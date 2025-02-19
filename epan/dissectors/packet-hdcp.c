/* packet-hdcp.c
 * Routines for HDCP dissection
 * Copyright 2011-2014, Martin Kaiser <martin@kaiser.cx>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * This dissector supports HDCP (version 1) over I2C. For now, only the
 * most common protocol messages are recognized.
 *
 * The specification of the version 1 protocol can be found at
 * http://www.digital-cp.com/files/static_page_files/5C3DC13B-9F6B-D82E-D77D8ACA08A448BF/HDCP Specification Rev1_4.pdf
 */

#include "config.h"

#include <epan/packet.h>
#include <epan/ptvcursor.h>
void proto_register_hdcp(void);

static int proto_hdcp;

static wmem_tree_t *transactions;

static int ett_hdcp;

static int hf_hdcp_reg;
static int hf_hdcp_resp_in;
static int hf_hdcp_resp_to;
static int hf_hdcp_a_ksv;
static int hf_hdcp_b_ksv;
static int hf_hdcp_an;
static int hf_hdcp_hdmi_reserved;
static int hf_hdcp_repeater;
static int hf_hdcp_ksv_fifo;
static int hf_hdcp_fast_trans;
static int hf_hdcp_features;
static int hf_hdcp_fast_reauth;
static int hf_hdcp_hdmi_mode;
static int hf_hdcp_max_casc_exc;
static int hf_hdcp_depth;
static int hf_hdcp_max_devs_exc;
static int hf_hdcp_downstream;
static int hf_hdcp_link_vfy;

#define REG_BKSV    0x0
#define REG_AKSV    0x10
#define REG_AN      0x18
#define REG_BCAPS   0x40
#define REG_BSTATUS 0x41

typedef struct _hdcp_transaction_t {
    uint32_t rqst_frame;
    uint32_t resp_frame;
    uint8_t rqst_type;
} hdcp_transaction_t;

static const value_string hdcp_reg[] = {
    { REG_BKSV, "B_ksv" },
    { REG_AKSV, "A_ksv" },
    { REG_AN, "An" },
    { REG_BCAPS, "B_caps"},
    { REG_BSTATUS, "B_status"},
    { 0, NULL }
};


/* the input tvb contains an HDCP message without the leading address byte
   (the address byte is handled by the HDMI dissector)
   the caller must set the direction in pinfo */
static int
dissect_hdcp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    uint8_t reg;
    proto_item *pi;
    ptvcursor_t *cursor;
    proto_tree *hdcp_tree;
    hdcp_transaction_t *hdcp_trans;
    proto_item *it;
    uint64_t a_ksv, b_ksv;

    /* XXX check if the packet is really HDCP? */

    col_set_str(pinfo->cinfo, COL_PROTOCOL, "HDCP");
    col_clear(pinfo->cinfo, COL_INFO);

    pi = proto_tree_add_protocol_format(tree, proto_hdcp,
            tvb, 0, tvb_reported_length(tvb), "HDCP");
    hdcp_tree = proto_item_add_subtree(pi, ett_hdcp);

    cursor = ptvcursor_new(pinfo->pool, hdcp_tree, tvb, 0);

    if (pinfo->p2p_dir==P2P_DIR_SENT) {
        /* transmitter sends data to the receiver */

        reg = tvb_get_uint8(tvb, ptvcursor_current_offset(cursor));
        /* all values in HDCP are little endian */
        ptvcursor_add(cursor, hf_hdcp_reg, 1, ENC_LITTLE_ENDIAN);

        if (tvb_reported_length_remaining(tvb,
                    ptvcursor_current_offset(cursor)) == 0) {
            /* transmitter requests the content of a register */
            col_append_sep_fstr(pinfo->cinfo, COL_INFO, NULL, "request %s",
                    val_to_str(reg, hdcp_reg, "unknown (0x%x)"));

            if (PINFO_FD_VISITED(pinfo)) {
                /* we've already dissected the receiver's response */
                hdcp_trans = (hdcp_transaction_t *)wmem_tree_lookup32(
                        transactions, pinfo->num);
                if (hdcp_trans && hdcp_trans->rqst_frame==pinfo->num &&
                        hdcp_trans->resp_frame!=0) {

                   it = proto_tree_add_uint_format(hdcp_tree, hf_hdcp_resp_in,
                           NULL, 0, 0, hdcp_trans->resp_frame,
                           "Request to get the content of register %s, "
                           "response in frame %d",
                           val_to_str_const(hdcp_trans->rqst_type,
                               hdcp_reg, "unknown (0x%x)"),
                           hdcp_trans->resp_frame);
                    proto_item_set_generated(it);
                }
            }
            else {
                /* we've not yet dissected the response */
                hdcp_trans = wmem_new(wmem_file_scope(), hdcp_transaction_t);
                hdcp_trans->rqst_frame = pinfo->num;
                hdcp_trans->resp_frame = 0;
                hdcp_trans->rqst_type = reg;
                wmem_tree_insert32(transactions,
                        hdcp_trans->rqst_frame, (void *)hdcp_trans);
            }
        }
        else {
            /* transmitter actually sends protocol data */
            col_append_sep_fstr(pinfo->cinfo, COL_INFO, NULL, "send %s",
                    val_to_str(reg, hdcp_reg, "unknown (0x%x)"));
            switch (reg) {
                case REG_AKSV:
                    a_ksv = tvb_get_letoh40(tvb,
                                ptvcursor_current_offset(cursor));
                    proto_tree_add_uint64_format(hdcp_tree, hf_hdcp_a_ksv,
                            tvb, ptvcursor_current_offset(cursor), 5,
                            a_ksv, "A_ksv 0x%010" PRIx64, a_ksv);
                    ptvcursor_advance(cursor, 5);
                    break;
                case REG_AN:
                    ptvcursor_add(cursor, hf_hdcp_an, 8, ENC_LITTLE_ENDIAN);
                    break;
                default:
                    break;
            }
        }
    }
    else {
        /* transmitter reads from receiver */

        hdcp_trans = (hdcp_transaction_t *)wmem_tree_lookup32_le(
                transactions, pinfo->num);
        if (hdcp_trans) {
            if (hdcp_trans->resp_frame==0) {
                /* there's a pending request, this packet is the response */
                hdcp_trans->resp_frame = pinfo->num;
            }

            if (hdcp_trans->resp_frame== pinfo->num) {
                /* we found the request that corresponds to our response */
                col_append_sep_fstr(pinfo->cinfo, COL_INFO, NULL, "send %s",
                        val_to_str_const(hdcp_trans->rqst_type,
                            hdcp_reg, "unknown (0x%x)"));
                it = proto_tree_add_uint_format(hdcp_tree, hf_hdcp_resp_to,
                        NULL, 0, 0, hdcp_trans->rqst_frame,
                        "Response to frame %d (content of register %s)",
                        hdcp_trans->rqst_frame,
                        val_to_str_const(hdcp_trans->rqst_type,
                            hdcp_reg, "unknown (0x%x)"));
                proto_item_set_generated(it);
                switch (hdcp_trans->rqst_type) {
                    case REG_BKSV:
                        b_ksv = tvb_get_letoh40(tvb,
                                ptvcursor_current_offset(cursor));
                        proto_tree_add_uint64_format(hdcp_tree, hf_hdcp_b_ksv,
                                tvb, ptvcursor_current_offset(cursor), 5,
                                b_ksv, "B_ksv 0x%010" PRIx64,
                                b_ksv);
                        ptvcursor_advance(cursor, 5);
                        break;
                    case REG_BCAPS:
                        ptvcursor_add_no_advance(cursor,
                                hf_hdcp_hdmi_reserved, 1, ENC_LITTLE_ENDIAN);
                        ptvcursor_add_no_advance(cursor,
                                hf_hdcp_repeater, 1, ENC_LITTLE_ENDIAN);
                        ptvcursor_add_no_advance(cursor,
                                hf_hdcp_ksv_fifo, 1, ENC_LITTLE_ENDIAN);
                        ptvcursor_add_no_advance(cursor,
                                hf_hdcp_fast_trans, 1, ENC_LITTLE_ENDIAN);
                        ptvcursor_add_no_advance(cursor,
                                hf_hdcp_features, 1, ENC_LITTLE_ENDIAN);
                        ptvcursor_add_no_advance(cursor,
                                hf_hdcp_fast_reauth, 1, ENC_LITTLE_ENDIAN);
                        break;
                    case REG_BSTATUS:
                        ptvcursor_add_no_advance(cursor,
                                hf_hdcp_hdmi_mode, 2, ENC_LITTLE_ENDIAN);
                        ptvcursor_add_no_advance(cursor,
                                hf_hdcp_max_casc_exc, 2, ENC_LITTLE_ENDIAN);
                        ptvcursor_add_no_advance(cursor,
                                hf_hdcp_depth, 2, ENC_LITTLE_ENDIAN);
                        ptvcursor_add_no_advance(cursor,
                                hf_hdcp_max_devs_exc, 2, ENC_LITTLE_ENDIAN);
                        ptvcursor_add_no_advance(cursor,
                                hf_hdcp_downstream, 2, ENC_LITTLE_ENDIAN);
                        break;
                }
            }
        }

        if (!hdcp_trans || hdcp_trans->resp_frame!=pinfo->num) {
            /* the packet isn't a response to a request from the
             * transmitter; it must be a link verification */
            if (tvb_reported_length_remaining(
                        tvb, ptvcursor_current_offset(cursor)) == 2) {
                col_append_sep_str(pinfo->cinfo, COL_INFO, NULL,
                        "send link verification Ri'");
                ptvcursor_add_no_advance(cursor,
                        hf_hdcp_link_vfy, 2, ENC_LITTLE_ENDIAN);
            }
        }
    }

    ptvcursor_free(cursor);
    return tvb_reported_length(tvb);
}


void
proto_register_hdcp(void)
{
    static hf_register_info hf[] = {
        { &hf_hdcp_reg,
            { "Register offset", "hdcp.reg", FT_UINT8, BASE_HEX,
                VALS(hdcp_reg), 0, NULL, HFILL } },
        { &hf_hdcp_resp_in,
            { "Response In", "hdcp.resp_in", FT_FRAMENUM, BASE_NONE, FRAMENUM_TYPE(FT_FRAMENUM_RESPONSE), 0x0,
                "The response to this request is in this frame", HFILL }},
        { &hf_hdcp_resp_to,
            { "Response To", "hdcp.resp_to", FT_FRAMENUM, BASE_NONE, FRAMENUM_TYPE(FT_FRAMENUM_REQUEST), 0x0,
                "This is the response to the request in this frame", HFILL }},
        { &hf_hdcp_a_ksv,
            { "Transmitter's key selection vector", "hdcp.a_ksv", FT_UINT40,
                BASE_HEX, NULL, 0, NULL, HFILL } },
        { &hf_hdcp_b_ksv,
            { "Receiver's key selection vector", "hdcp.b_ksv", FT_UINT64,
                BASE_HEX, NULL, 0, NULL, HFILL } },
        { &hf_hdcp_an,
            { "Random number for the session", "hdcp.an", FT_UINT64,
                BASE_HEX, NULL, 0, NULL, HFILL } },
        { &hf_hdcp_hdmi_reserved,
            { "HDMI reserved", "hdcp.hdmi_reserved", FT_UINT8, BASE_DEC,
                NULL, 0x80, NULL, HFILL } },
        { &hf_hdcp_repeater,
            { "Repeater", "hdcp.repeater", FT_UINT8, BASE_DEC,
                NULL, 0x40, NULL, HFILL } },
        { &hf_hdcp_ksv_fifo,
            { "KSV fifo ready", "hdcp.ksv_fifo", FT_UINT8, BASE_DEC,
                NULL, 0x20, NULL, HFILL } },
        { &hf_hdcp_fast_trans,
            { "Support for 400KHz transfers", "hdcp.fast_trans",
                FT_UINT8, BASE_DEC, NULL, 0x10, NULL, HFILL } },
        { &hf_hdcp_features,
            { "Support for additional features", "hdcp.features",
                FT_UINT8, BASE_DEC, NULL, 0x02, NULL, HFILL } },
        { &hf_hdcp_fast_reauth,
            { "Support for fast re-authentication", "hdcp.fast_reauth",
                FT_UINT8, BASE_DEC, NULL, 0x01, NULL, HFILL } },
        { &hf_hdcp_hdmi_mode,
            { "HDMI mode", "hdcp.hdmi_mode",
                FT_UINT16, BASE_DEC, NULL, 0x1000, NULL, HFILL } },
        { &hf_hdcp_max_casc_exc,
            { "Maximum cascading depth exceeded", "hdcp.max_casc_exc",
                FT_UINT16, BASE_DEC, NULL, 0x0800, NULL, HFILL } },
        { &hf_hdcp_depth,
            { "Repeater cascade depth", "hdcp.depth",
                FT_UINT16, BASE_DEC, NULL, 0x0700, NULL, HFILL } },
        { &hf_hdcp_max_devs_exc,
            { "Maximum number of devices exceeded", "hdcp.max_devs_exc",
                FT_UINT16, BASE_DEC, NULL, 0x0080, NULL, HFILL } },
        { &hf_hdcp_downstream,
            { "Number of downstream receivers", "hdcp.downstream",
                FT_UINT16, BASE_DEC, NULL, 0x007F, NULL, HFILL } },
        { &hf_hdcp_link_vfy,
            { "Link verification response Ri'", "hdcp.link_vfy",
                FT_UINT16, BASE_HEX, NULL, 0, NULL, HFILL } }
    };

    static int *ett[] = {
        &ett_hdcp
    };


    proto_hdcp = proto_register_protocol("High bandwidth Digital Content Protection", "HDCP", "hdcp");

    proto_register_field_array(proto_hdcp, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

    register_dissector("hdcp", dissect_hdcp, proto_hdcp);

    transactions = wmem_tree_new_autoreset(wmem_epan_scope(), wmem_file_scope());
}

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
