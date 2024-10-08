/* packet-dcerpc-ubikvote.c
 *
 * Routines for DCE DFS Ubik Voting  routines.
 * Copyright 2002, Jaime Fournier <Jaime.Fournier@hush.com>
 * This information is based off the released idl files from opengroup.
 * ftp://ftp.opengroup.org/pub/dce122/dce/src/file.tar.gz file/ncsubik/ubikvote_proc.idl
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"


#include <epan/packet.h>
#include "packet-dcerpc.h"

void proto_register_ubikvote (void);
void proto_reg_handoff_ubikvote (void);

static int proto_ubikvote;
static int hf_ubikvote_opnum;


static int ett_ubikvote;


static e_guid_t uuid_ubikvote = { 0x4d37f2dd, 0xed43, 0x0003, { 0x02, 0xc0, 0x37, 0xcf, 0x1e, 0x00, 0x00, 0x00 } };
static uint16_t ver_ubikvote = 4;


static const dcerpc_sub_dissector ubikvote_dissectors[] = {
	{ 0, "Beacon",              NULL, NULL},
	{ 1, "Debug",               NULL, NULL},
	{ 2, "SDebug",              NULL, NULL},
	{ 3, "GetServerInterfaces", NULL, NULL},
	{ 4, "GetSyncSite",         NULL, NULL},
	{ 5, "DebugV2",             NULL, NULL},
	{ 6, "SDebugV2",            NULL, NULL},
	{ 7, "GetSyncSiteIdentity", NULL, NULL},
	{ 0, NULL, NULL, NULL }
};

void
proto_register_ubikvote (void)
{
	static hf_register_info hf[] = {
	  { &hf_ubikvote_opnum,
	    { "Operation", "ubikvote.opnum", FT_UINT16, BASE_DEC,
	      NULL, 0x0, NULL, HFILL }}
	};

	static int *ett[] = {
		&ett_ubikvote,
	};
	proto_ubikvote = proto_register_protocol ("DCE DFS FLDB UBIKVOTE", "UBIKVOTE", "ubikvote");
	proto_register_field_array (proto_ubikvote, hf, array_length (hf));
	proto_register_subtree_array (ett, array_length (ett));
}

void
proto_reg_handoff_ubikvote (void)
{
	/* Register the protocol as dcerpc */
	dcerpc_init_uuid (proto_ubikvote, ett_ubikvote, &uuid_ubikvote, ver_ubikvote, ubikvote_dissectors, hf_ubikvote_opnum);
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
