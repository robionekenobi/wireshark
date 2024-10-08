/* packet-pkinit.c
 * Routines for PKINIT packet dissection
 *  Ronnie Sahlberg 2004
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <epan/packet.h>
#include <epan/oids.h>
#include <epan/asn1.h>
#include <epan/proto_data.h>
#include <wsutil/array.h>

#include "packet-ber.h"
#include "packet-pkinit.h"
#include "packet-cms.h"
#include "packet-pkix1explicit.h"
#include "packet-kerberos.h"

#define PNAME  "PKINIT"
#define PSNAME "PKInit"
#define PFNAME "pkinit"

void proto_register_pkinit(void);
void proto_reg_handoff_pkinit(void);

/* Initialize the protocol and registered fields */
static int proto_pkinit;
#include "packet-pkinit-hf.c"

/* Initialize the subtree pointers */
#include "packet-pkinit-ett.c"

static int dissect_KerberosV5Spec2_KerberosTime(bool implicit_tag _U_, tvbuff_t *tvb, int offset,  asn1_ctx_t *actx, proto_tree *tree, int hf_index _U_);
static int dissect_KerberosV5Spec2_Realm(bool implicit_tag _U_, tvbuff_t *tvb, int offset,  asn1_ctx_t *actx, proto_tree *tree, int hf_index _U_);
static int dissect_KerberosV5Spec2_PrincipalName(bool implicit_tag _U_, tvbuff_t *tvb, int offset,  asn1_ctx_t *actx, proto_tree *tree, int hf_index _U_);
static int dissect_pkinit_PKAuthenticator_Win2k(bool implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);

#include "packet-pkinit-fn.c"

static int
dissect_KerberosV5Spec2_KerberosTime(bool implicit_tag _U_, tvbuff_t *tvb, int offset, asn1_ctx_t *actx, proto_tree *tree, int hf_index _U_) {
  offset = dissect_krb5_ctime(tree, tvb, offset, actx);
  return offset;
}

static int
dissect_KerberosV5Spec2_Realm(bool implicit_tag _U_, tvbuff_t *tvb, int offset, asn1_ctx_t *actx, proto_tree *tree, int hf_index _U_) {
  offset = dissect_krb5_realm(tree, tvb, offset, actx);
  return offset;
}

static int
dissect_KerberosV5Spec2_PrincipalName(bool implicit_tag _U_, tvbuff_t *tvb, int offset, asn1_ctx_t *actx, proto_tree *tree, int hf_index _U_) {
  offset = dissect_krb5_cname(tree, tvb, offset, actx);
  return offset;
}


/*--- proto_register_pkinit ----------------------------------------------*/
void proto_register_pkinit(void) {

  /* List of fields */
  static hf_register_info hf[] = {
#include "packet-pkinit-hfarr.c"
  };

  /* List of subtrees */
  static int *ett[] = {
#include "packet-pkinit-ettarr.c"
  };

  /* Register protocol */
  proto_pkinit = proto_register_protocol(PNAME, PSNAME, PFNAME);

  /* Register fields and subtrees */
  proto_register_field_array(proto_pkinit, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

}


/*--- proto_reg_handoff_pkinit -------------------------------------------*/
void proto_reg_handoff_pkinit(void) {
#include "packet-pkinit-dis-tab.c"

    /* It would seem better to get these from REGISTER declarations in
       pkinit.cnf rather than putting them in the template this way,
       but I had trouble with that, and other existing examples are
       done this way. [res Fri Aug 2 23:55:30 2024]

       RFC-8636 "PKINIT Algorithm Agility"
    */
    oid_add_from_string("id-pkinit-kdf-ah-sha1"   , "1.3.6.1.5.2.3.6.1");
    oid_add_from_string("id-pkinit-kdf-ah-sha256" , "1.3.6.1.5.2.3.6.2");
    oid_add_from_string("id-pkinit-kdf-ah-sha512" , "1.3.6.1.5.2.3.6.3");
    oid_add_from_string("id-pkinit-kdf-ah-sha384" , "1.3.6.1.5.2.3.6.4");
}
