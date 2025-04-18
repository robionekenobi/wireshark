/* packet-pw-satop.c
 * Routines for SAToP PW dissection as per RFC4553.
 * Copyright 2009, Dmitry Trebich, Artem Tamazov <artem.tamazov@tellabs.com>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * History:
 * ---------------------------------
 * 19.03.2009 initial implementation
 * 14.08.2009 added: support for IP/UDP demultiplexing
 * Not supported yet:
 * - Decoding of PW payload
 */

#include "config.h"

#include <epan/packet.h>
#include <epan/expert.h>

#include "packet-mpls.h"
#include "packet-pw-common.h"
#include "packet-rtp.h"

#define SIZEOF_RTP 12

void proto_register_pw_satop(void);
void proto_reg_handoff_pw_satop(void);

static int proto = -1;
static int ett_pw_satop;

static int hf_cw;
static int hf_cw_bits03;
static int hf_cw_l;
static int hf_cw_r;
static int hf_cw_rsv;
static int hf_cw_frg;
static int hf_cw_len;
static int hf_cw_seq;
static int hf_payload;
static int hf_payload_l;

static expert_field ei_cw_rsv;
static expert_field ei_payload_size_invalid_undecoded;
static expert_field ei_payload_size_invalid;
static expert_field ei_cw_frg;
static expert_field ei_cw_bits03;
static expert_field ei_cw_packet_size_too_small;

static dissector_handle_t pw_padding_handle;
static dissector_handle_t pw_satop_udp_handle;
static dissector_handle_t pw_satop_mpls_handle;

const char pwc_longname_pw_satop[] = "SAToP";
static const char shortname[] = "SAToP";

/* Preferences */
static bool pref_has_rtp_header;
static bool pref_heuristic_rtp_header = true;


static
void dissect_pw_satop(tvbuff_t * tvb_original
					,packet_info * pinfo
					,proto_tree * tree
					,pwc_demux_type_t demux)
{
	int min_packet_size_this_dissector = PWC_SIZEOF_CW;
	int encaps_size;
	int       packet_size;
	int       rtp_header_offset;
	int       cw_offset;
	int       payload_size;
	int       padding_size;
	int properties;
	uint16_t  sn;
	bool      has_rtp_header;

	enum {
		PAY_NO_IDEA = 0
		,PAY_LIKE_E1
		,PAY_LIKE_T1
		,PAY_LIKE_E3_T3
		,PAY_LIKE_OCTET_ALIGNED_T1
	} payload_properties;

	switch (demux)
	{
	case PWC_DEMUX_MPLS:
		if (dissect_try_cw_first_nibble(tvb_original, pinfo, tree))
		{
			return;
		}
		break;
	case PWC_DEMUX_UDP:
		break;
	default:
		DISSECTOR_ASSERT_NOT_REACHED();
		return;
	}

	packet_size = tvb_reported_length_remaining(tvb_original, 0);
	if (pref_has_rtp_header) {
		min_packet_size_this_dissector += SIZEOF_RTP;
	}

	if (packet_size < min_packet_size_this_dissector)
	{
		proto_item  *item;
		item = proto_tree_add_item(tree, proto, tvb_original, 0, -1, ENC_NA);
		expert_add_info_format(pinfo, item, &ei_cw_packet_size_too_small,
				       "PW packet size (%d) is too small to carry sensible information"
				       ,(int)packet_size);
		col_set_str(pinfo->cinfo, COL_PROTOCOL, shortname);
		col_set_str(pinfo->cinfo, COL_INFO, "Malformed: PW packet is too small");
		return;
	}

	switch (demux)
	{
	case PWC_DEMUX_MPLS:
		rtp_header_offset = PWC_SIZEOF_CW;
		sn = tvb_get_uint16(tvb_original, 2, ENC_BIG_ENDIAN);
		break;
	case PWC_DEMUX_UDP:
		rtp_header_offset = 0;
		sn = tvb_get_uint16(tvb_original, SIZEOF_RTP + 2, ENC_BIG_ENDIAN);
		break;
	default:
		DISSECTOR_ASSERT_NOT_REACHED();
		return;
	}

	if ((pref_has_rtp_header) ||
		((pref_heuristic_rtp_header) &&
			/* Check for RTP version 2, the other fields must be zero */
			(tvb_get_uint8(tvb_original, rtp_header_offset) == 0x80) &&
			/* Check the marker is zero. Unfortunately PT is not always from the dynamic range */
			((tvb_get_uint8(tvb_original, rtp_header_offset + 1) & 0x80) == 0) &&
			/* The sequence numbers from cw and RTP header must match */
			(tvb_get_ntohs(tvb_original, rtp_header_offset + 2) == sn)))
	{
		switch (demux)
		{
		case PWC_DEMUX_MPLS:
			cw_offset = 0;
			break;
		case PWC_DEMUX_UDP:
			cw_offset = SIZEOF_RTP;
			break;
		default:
			DISSECTOR_ASSERT_NOT_REACHED();
			return;
		}
		encaps_size = PWC_SIZEOF_CW + SIZEOF_RTP;
		has_rtp_header = true;
	} else {
		cw_offset = 0;
		encaps_size = PWC_SIZEOF_CW;
		has_rtp_header = false;
	}

	/* check how "good" is this packet */
	/* also decide payload length from packet size, CW and optional RTP header */
	properties = 0;
	if (0 != (tvb_get_uint8(tvb_original, cw_offset) & 0xf0 /*bits03*/))
	{
		properties |= PWC_CW_BAD_BITS03;
	}
	if (0 != (tvb_get_uint8(tvb_original, cw_offset) & 0x03 /*rsv*/))
	{
		properties |= PWC_CW_BAD_RSV;
	}
	if (0 != (tvb_get_uint8(tvb_original, cw_offset + 1) & 0xc0 /*frag*/))
	{
		properties |= PWC_CW_BAD_FRAG;
	}
	{
		/* RFC4553:
		 * [...MAY be used to carry the length of the SAToP
		 * packet (defined as the size of the SAToP header + the payload
		 * size) if it is less than 64 bytes, and MUST be set to zero
		 * otherwise... ]
		 *
		 * Note that this differs from RFC4385's definition of length:
		 * [ If the MPLS payload is less than 64 bytes, the length field
		 * MUST be set to the length of the PW payload...]
		 *
		 * We will use RFC4553's definition here.
		 */
		int  cw_len;
		int payload_size_from_packet;

		cw_len = tvb_get_uint8(tvb_original, cw_offset + 1) & 0x3f;
		payload_size_from_packet = packet_size - encaps_size;
		if (cw_len != 0)
		{
			int payload_size_from_cw;
			payload_size_from_cw = cw_len - encaps_size;
			/*
			 * Assumptions for error case,
			 * will be overwritten if no errors found:
			 */
			payload_size = payload_size_from_packet;
			padding_size = 0;

			if (payload_size_from_cw < 0)
			{
				properties |= PWC_CW_BAD_PAYLEN_LT_0;
			}
			else if (payload_size_from_cw > payload_size_from_packet)
			{
				properties |= PWC_CW_BAD_PAYLEN_GT_PACKET;
			}
			else if (payload_size_from_packet >= 64)
			{
				properties |= PWC_CW_BAD_LEN_MUST_BE_0;
			}
			else /* ok */
			{
				payload_size = payload_size_from_cw;
				padding_size = payload_size_from_packet - payload_size_from_cw; /* >=0 */
			}
		}
		else
		{
			payload_size = payload_size_from_packet;
			padding_size = 0;
		}
	}
	if (payload_size == 0)
	{
		/*
		 * As CW.L it indicates that PW payload is invalid, dissector should
		 * not blame packets with bad payload (including "bad" or "strange" SIZE of
		 * payload) when L bit is set.
		 */
		if (0 == (tvb_get_uint8(tvb_original, cw_offset) & 0x08 /*L bit*/))
		{
			properties |= PWC_PAY_SIZE_BAD;
		}
	}

	/* guess about payload type */
	if (payload_size == 256)
	{
		payload_properties = PAY_LIKE_E1;
	}
	else if (payload_size == 192)
	{
		payload_properties = PAY_LIKE_T1;
	}
	else if (payload_size == 1024)
	{
		payload_properties = PAY_LIKE_E3_T3;
	}
	else if ((payload_size != 0) && (payload_size % 25 == 0))
	{
		payload_properties = PAY_LIKE_OCTET_ALIGNED_T1;
	}
	else
	{
		payload_properties = PAY_NO_IDEA; /*we do not have any ideas about payload type*/
	}

	/* fill up columns*/
	col_set_str(pinfo->cinfo, COL_PROTOCOL, shortname);
	col_clear(pinfo->cinfo, COL_INFO);
	if (properties & PWC_ANYOF_CW_BAD)
	{
		col_set_str(pinfo->cinfo, COL_INFO, "CW:Bad, ");
	}

	if (properties & PWC_PAY_SIZE_BAD)
	{
		col_append_str(pinfo->cinfo, COL_INFO, "Payload size:0 (Bad)");
	}
	else
	{
		col_append_fstr(pinfo->cinfo, COL_INFO, "TDM octets:%d", (int)payload_size);
	}

	if (padding_size != 0)
	{
		col_append_fstr(pinfo->cinfo, COL_INFO, ", Padding:%d", (int)padding_size);
	}


	{
		proto_item* item;
		item = proto_tree_add_item(tree, proto, tvb_original, 0, -1, ENC_NA);
		pwc_item_append_cw(item, tvb_get_ntohl(tvb_original, cw_offset), true);
		pwc_item_append_text_n_items(item, (int)payload_size, "octet");
		{
			proto_tree* tree2;
			tree2 = proto_item_add_subtree(item, ett_pw_satop);
			if (has_rtp_header && demux == PWC_DEMUX_UDP)
			{
				dissect_rtp_shim_header(tvb_original, 0, pinfo, tree2, NULL);
			}

			{
				tvbuff_t* tvb;
				proto_item* item2;
				tvb = tvb_new_subset_length(tvb_original, cw_offset, PWC_SIZEOF_CW);
				item2 = proto_tree_add_item(tree2, hf_cw, tvb, 0, -1, ENC_NA);
				pwc_item_append_cw(item2, tvb_get_ntohl(tvb, 0), false);
				{
					proto_tree* tree3;
					tree3 = proto_item_add_subtree(item2, ett_pw_satop);
					{
						proto_item* item3;
						if (properties & PWC_CW_BAD_BITS03) /*display only if value is wrong*/
						{
							item3 = proto_tree_add_item(tree3, hf_cw_bits03, tvb, 0, 1, ENC_BIG_ENDIAN);
							expert_add_info(pinfo, item3, &ei_cw_bits03);
						}

						proto_tree_add_item(tree3, hf_cw_l, tvb, 0, 1, ENC_BIG_ENDIAN);
						proto_tree_add_item(tree3, hf_cw_r, tvb, 0, 1, ENC_BIG_ENDIAN);

						item3 = proto_tree_add_item(tree3, hf_cw_rsv, tvb, 0, 1, ENC_BIG_ENDIAN);
						if (properties & PWC_CW_BAD_RSV)
						{
							expert_add_info(pinfo, item3, &ei_cw_rsv);
						}

						item3 = proto_tree_add_item(tree3, hf_cw_frg, tvb, 1, 1, ENC_BIG_ENDIAN);
						if (properties & PWC_CW_BAD_FRAG)
						{
							expert_add_info(pinfo, item3, &ei_cw_frg);
						}

						item3 = proto_tree_add_item(tree3, hf_cw_len, tvb, 1, 1, ENC_BIG_ENDIAN);
						if (properties & PWC_CW_BAD_PAYLEN_LT_0)
						{
							expert_add_info_format(pinfo, item3, &ei_payload_size_invalid,
								"Bad Length: too small, must be > %d",
								(int)encaps_size);
						}
						if (properties & PWC_CW_BAD_PAYLEN_GT_PACKET)
						{
							expert_add_info_format(pinfo, item3, &ei_payload_size_invalid,
								"Bad Length: must be <= than PSN packet size (%d)",
								(int)packet_size);
						}
						if (properties & PWC_CW_BAD_LEN_MUST_BE_0)
						{
							expert_add_info_format(pinfo, item3, &ei_payload_size_invalid,
								"Bad Length: must be 0 if SAToP packet size (%d) is > 64",
								(int)packet_size);
						}

						proto_tree_add_item(tree3, hf_cw_seq, tvb, 2, 2, ENC_BIG_ENDIAN);
					}
				}
			}

			if (has_rtp_header && demux != PWC_DEMUX_UDP)
			{
				dissect_rtp_shim_header(tvb_original, PWC_SIZEOF_CW, pinfo, tree2, NULL);
			}
		}

		/* payload */
		if (properties & PWC_PAY_SIZE_BAD)
		{
			expert_add_info_format(pinfo, item, &ei_payload_size_invalid,
				"SAToP payload: none found. Size of payload must be <> 0");
		}
		else if (payload_size == 0)
		{
			expert_add_info(pinfo, item, &ei_payload_size_invalid_undecoded);
		}
		else
		{

			proto_tree* tree2;
			tree2 = proto_item_add_subtree(item, ett_pw_satop);
			{
				proto_item* item2;
				tvbuff_t* tvb;
				tvb = tvb_new_subset_length(tvb_original, encaps_size, payload_size);
				item2 = proto_tree_add_item(tree2, hf_payload, tvb, 0, -1, ENC_NA);
				pwc_item_append_text_n_items(item2, (int)payload_size, "octet");
				{
					proto_tree* tree3;
					const char* s;
					switch(payload_properties)
					{
					case PAY_LIKE_E1:
						s = " (looks like E1)";
						break;
					case PAY_LIKE_T1:
						s = " (looks like T1)";
						break;
					case PAY_LIKE_E3_T3:
						s = " (looks like E3/T3)";
						break;
					case PAY_LIKE_OCTET_ALIGNED_T1:
						s = " (looks like octet-aligned T1)";
						break;
					case PAY_NO_IDEA:
					default:
						s = "";
						break;
					}
					proto_item_append_text(item2, "%s", s);
					tree3 = proto_item_add_subtree(item2, ett_pw_satop);
					call_data_dissector(tvb, pinfo, tree3);
					item2 = proto_tree_add_int(tree3, hf_payload_l, tvb, 0, 0
						,(int)payload_size); /* allow filtering */
					proto_item_set_hidden(item2);
				}
			}
		}

		/* padding */
		if (padding_size > 0)
		{
			proto_tree* tree2;
			tree2 = proto_item_add_subtree(item, ett_pw_satop);
			{
				tvbuff_t* tvb;
				tvb = tvb_new_subset_length_caplen(tvb_original, PWC_SIZEOF_CW + payload_size, padding_size, -1);
				call_dissector(pw_padding_handle, tvb, pinfo, tree2);
			}
		}
	}
	return;
}


static
int dissect_pw_satop_mpls( tvbuff_t * tvb_original, packet_info * pinfo, proto_tree * tree, void* data _U_)
{
	dissect_pw_satop(tvb_original,pinfo,tree,PWC_DEMUX_MPLS);
	return tvb_captured_length(tvb_original);
}


static
int dissect_pw_satop_udp( tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree, void* data _U_)
{
	dissect_pw_satop(tvb,pinfo,tree,PWC_DEMUX_UDP);
	return tvb_captured_length(tvb);
}


void proto_register_pw_satop(void)
{
	static hf_register_info hf[] = {
		{ &hf_cw	,{"Control Word"		,"pwsatop.cw"
				,FT_NONE			,BASE_NONE		,NULL
				,0				,NULL			,HFILL }},

		{&hf_cw_bits03,{"Bits 0 to 3"			,"pwsatop.cw.bits03"
				,FT_UINT8			,BASE_DEC		,NULL
				,0xf0				,NULL			,HFILL }},

		{&hf_cw_l,	{"L bit: TDM payload state"	,"pwsatop.cw.lbit"
				,FT_UINT8			,BASE_DEC		,VALS(pwc_vals_cw_l_bit)
				,0x08				,NULL			,HFILL }},

		{&hf_cw_r,	{"R bit: Local CE-bound IWF"	,"pwsatop.cw.rbit"
				,FT_UINT8			,BASE_DEC		,VALS(pwc_vals_cw_r_bit)
				,0x04				,NULL			,HFILL }},

		{&hf_cw_rsv,	{"Reserved"			,"pwsatop.cw.rsv"
				,FT_UINT8			,BASE_DEC		,NULL
				,0x03				,NULL			,HFILL }},

		{&hf_cw_frg,	{"Fragmentation"		,"pwsatop.cw.frag"
				,FT_UINT8			,BASE_DEC		,VALS(pwc_vals_cw_frag)
				,0xc0				,NULL			,HFILL }},

		{&hf_cw_len,	{"Length"			,"pwsatop.cw.length"
				,FT_UINT8			,BASE_DEC		,NULL
				,0x3f				,NULL			,HFILL }},

		{&hf_cw_seq,	{"Sequence number"		,"pwsatop.cw.seqno"
				,FT_UINT16			,BASE_DEC		,NULL
				,0				,NULL			,HFILL }},

		{&hf_payload	,{"TDM payload"			,"pwsatop.payload"
				,FT_BYTES			,BASE_NONE		,NULL
				,0				,NULL			,HFILL }},

		{&hf_payload_l	,{"TDM payload length"		,"pwsatop.payload.len"
				,FT_INT32			,BASE_DEC		,NULL
				,0				,NULL			,HFILL }}
	};

	static int *ett_array[] = {
		&ett_pw_satop
	};
	static ei_register_info ei[] = {
		{ &ei_cw_packet_size_too_small, { "pwsatop.packet_size_too_small", PI_MALFORMED, PI_ERROR, "PW packet size is too small to carry sensible information", EXPFILL }},
		{ &ei_cw_bits03, { "pwsatop.cw.bits03.not_zero", PI_MALFORMED, PI_ERROR, "Bits 0..3 of Control Word must be 0", EXPFILL }},
		{ &ei_cw_rsv, { "pwsatop.cw.rsv.not_zero", PI_MALFORMED, PI_ERROR, "RSV bits of Control Word must be 0", EXPFILL }},
		{ &ei_cw_frg, { "pwsatop.cw.frag.not_allowed", PI_MALFORMED, PI_ERROR, "Fragmentation of payload is not allowed for SAToP", EXPFILL }},
		{ &ei_payload_size_invalid, { "pwsatop.payload.size_invalid", PI_MALFORMED, PI_ERROR, "Bad Length: too small", EXPFILL }},
		{ &ei_payload_size_invalid_undecoded, { "pwsatop.payload.undecoded", PI_UNDECODED, PI_NOTE, "SAToP payload: omitted to conserve bandwidth", EXPFILL }},
	};

	module_t *pwsatop_module;
	expert_module_t* expert_pwsatop;

	proto = proto_register_protocol(pwc_longname_pw_satop, shortname, "pwsatopcw");
	proto_register_field_array(proto, hf, array_length(hf));
	proto_register_subtree_array(ett_array, array_length(ett_array));
	expert_pwsatop = expert_register_protocol(proto);
	expert_register_field_array(expert_pwsatop, ei, array_length(ei));
	pwsatop_module = prefs_register_protocol(proto, NULL);
	prefs_register_bool_preference(pwsatop_module, "rtp_header", "RTP header in SAToP header",
					"Whether or not the RTP header is present in the SAToP header.", &pref_has_rtp_header);
	prefs_register_bool_preference(pwsatop_module, "rtp_header_heuristic", "Try to find RTP header in SAToP header",
					"Heuristically determine if an RTP header is present in the SAToP header.", &pref_heuristic_rtp_header);
	pw_satop_mpls_handle = register_dissector("pw_satop_mpls", dissect_pw_satop_mpls, proto);
	pw_satop_udp_handle = register_dissector("pw_satop_udp", dissect_pw_satop_udp, proto);
}

void proto_reg_handoff_pw_satop(void)
{
	pw_padding_handle = find_dissector_add_dependency("pw_padding", proto);

	/* For Decode As */
	dissector_add_for_decode_as("mpls.label", pw_satop_mpls_handle);
	dissector_add_for_decode_as("mpls.pfn", pw_satop_mpls_handle);
	dissector_add_for_decode_as_with_preference("udp.port", pw_satop_udp_handle);
}

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: t
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 noexpandtab:
 * :indentSize=8:tabSize=8:noTabs=false:
 */
