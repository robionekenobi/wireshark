/** @file
 *
 * Copyright (c) 2003 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This software and documentation has been developed by Endace Technology Ltd.
 * along with the DAG PCI network capture cards. For further information please
 * visit https://www.endace.com/.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __W_ERF_RECORD_H__
#define __W_ERF_RECORD_H__

/*
 * Declarations and definitions for ERF records; for use by the ERF
 * file reader, code to handle LINKTYPE_ERF packets in pcap and
 * pcapng files, ERF metadata dissectors, and protocol dissectors
 * that register for particular ERF record types.
 */

/* Record type defines */
#define ERF_TYPE_LEGACY             0
#define ERF_TYPE_HDLC_POS           1
#define ERF_TYPE_ETH                2
#define ERF_TYPE_ATM                3
#define ERF_TYPE_AAL5               4
#define ERF_TYPE_MC_HDLC            5
#define ERF_TYPE_MC_RAW             6
#define ERF_TYPE_MC_ATM             7
#define ERF_TYPE_MC_RAW_CHANNEL     8
#define ERF_TYPE_MC_AAL5            9
#define ERF_TYPE_COLOR_HDLC_POS     10
#define ERF_TYPE_COLOR_ETH          11
#define ERF_TYPE_MC_AAL2            12
#define ERF_TYPE_IP_COUNTER         13
#define ERF_TYPE_TCP_FLOW_COUNTER   14
#define ERF_TYPE_DSM_COLOR_HDLC_POS 15
#define ERF_TYPE_DSM_COLOR_ETH      16
#define ERF_TYPE_COLOR_MC_HDLC_POS  17
#define ERF_TYPE_AAL2               18
#define ERF_TYPE_COLOR_HASH_POS     19
#define ERF_TYPE_COLOR_HASH_ETH     20
#define ERF_TYPE_INFINIBAND         21
#define ERF_TYPE_IPV4               22
#define ERF_TYPE_IPV6               23
#define ERF_TYPE_RAW_LINK           24
#define ERF_TYPE_INFINIBAND_LINK    25
/* XXX - what about 26? */
#define ERF_TYPE_META               27
#define ERF_TYPE_OPA_SNC            28
#define ERF_TYPE_OPA_9B             29

/* 28-31 reserved for future public ERF types */

/* Record types reserved for local and internal use */
#define ERF_TYPE_INTERNAL0          32
#define ERF_TYPE_INTERNAL1          33
#define ERF_TYPE_INTERNAL2          34
#define ERF_TYPE_INTERNAL3          35
#define ERF_TYPE_INTERNAL4          36
#define ERF_TYPE_INTERNAL5          37
#define ERF_TYPE_INTERNAL6          38
#define ERF_TYPE_INTERNAL7          39
#define ERF_TYPE_INTERNAL8          40
#define ERF_TYPE_INTERNAL9          41
#define ERF_TYPE_INTERNAL10         42
#define ERF_TYPE_INTERNAL11         43
#define ERF_TYPE_INTERNAL12         44
#define ERF_TYPE_INTERNAL13         45
#define ERF_TYPE_INTERNAL14         46
#define ERF_TYPE_INTERNAL15         47

/* Pad records */
#define ERF_TYPE_PAD                48

#define ERF_EXT_HDR_TYPE_CLASSIFICATION  3
#define ERF_EXT_HDR_TYPE_INTERCEPTID     4
#define ERF_EXT_HDR_TYPE_RAW_LINK        5
#define ERF_EXT_HDR_TYPE_BFS             6
#define ERF_EXT_HDR_TYPE_CHANNELISED    12
#define ERF_EXT_HDR_TYPE_SIGNATURE      14
#define ERF_EXT_HDR_TYPE_PKT_ID         15
#define ERF_EXT_HDR_TYPE_FLOW_ID        16
#define ERF_EXT_HDR_TYPE_HOST_ID        17
#define ERF_EXT_HDR_TYPE_ANCHOR_ID      18
#define ERF_EXT_HDR_TYPE_ENTROPY        19

/* ERF Header */
#define ERF_HDR_TYPE_MASK   0x7f
#define ERF_HDR_EHDR_MASK   0x80
#define ERF_HDR_FLAGS_MASK  0xff
#define ERF_HDR_CAP_MASK    0x43
#define ERF_HDR_CAP_LO_MASK 0x03
#define ERF_HDR_CAP_HI_MASK 0x40
#define ERF_HDR_VLEN_MASK   0x04
#define ERF_HDR_TRUNC_MASK  0x08
#define ERF_HDR_RXE_MASK    0x10
#define ERF_HDR_DSE_MASK    0x20
#define ERF_HDR_RES_MASK    0x80

/*
 * Calculate 3-bit ERF InterfaceID value from ERF Header flags byte
 */
#define erf_interface_id_from_flags(flags) (((((uint8_t)flags) & ERF_HDR_CAP_HI_MASK) >> 4 ) | (((uint8_t)flags) & ERF_HDR_CAP_LO_MASK))

/* Host ID and Anchor ID*/
#define ERF_EHDR_HOST_ID_MASK UINT64_C(0xffffffffffff)
#define ERF_EHDR_ANCHOR_ID_MASK UINT64_C(0xffffffffffff)
#define ERF_EHDR_MORE_EXTHDR_MASK UINT64_C(0x8000000000000000)
#define ERF_EHDR_ANCHOR_ID_DEFINITION_MASK UINT64_C(0x80000000000000)

#define ERF_EHDR_FLOW_ID_STACK_TYPE_MASK UINT64_C(0xff00000000)
#define ERF_EHDR_FLOW_ID_SOURCE_ID_MASK  UINT64_C(0xff000000000000)

/* ERF Provenance metadata */
#define ERF_META_SECTION_MASK 0xFF00
#define ERF_META_IS_SECTION(type) (type > 0 && (type & ERF_META_SECTION_MASK) == ERF_META_SECTION_MASK)
#define ERF_META_HOST_ID_IMPLICIT UINT64_MAX
#define ERF_ANCHOR_ID_IS_DEFINITION(anchor_id) ((uint64_t)anchor_id & ERF_EHDR_ANCHOR_ID_DEFINITION_MASK)
#define ERF_EHDR_SET_MORE_EXTHDR(ext_hdr) ((uint64_t)ext_hdr | ERF_EHDR_MORE_EXTHDR_MASK)

#define ERF_META_SECTION_CAPTURE     0xFF00
#define ERF_META_SECTION_HOST        0xFF01
#define ERF_META_SECTION_MODULE      0xFF02
#define ERF_META_SECTION_INTERFACE   0xFF03
#define ERF_META_SECTION_FLOW        0xFF04
#define ERF_META_SECTION_STATS       0xFF05
#define ERF_META_SECTION_INFO        0xFF06
#define ERF_META_SECTION_CONTEXT     0xFF07
#define ERF_META_SECTION_STREAM      0xFF08
#define ERF_META_SECTION_TRANSFORM   0xFF09
#define ERF_META_SECTION_DNS         0xFF0A
#define ERF_META_SECTION_SOURCE      0xFF0B
#define ERF_META_SECTION_NETWORK     0xFF0C
#define ERF_META_SECTION_ENDPOINT    0xFF0D
#define ERF_META_SECTION_INPUT       0xFF0E
#define ERF_META_SECTION_OUTPUT      0xFF0F

#define ERF_META_TAG_padding           0
#define ERF_META_TAG_comment           1
#define ERF_META_TAG_gen_time          2
#define ERF_META_TAG_parent_section    3
#define ERF_META_TAG_reset             4
#define ERF_META_TAG_event_time        5
#define ERF_META_TAG_host_id           6
#define ERF_META_TAG_attribute         7
#define ERF_META_TAG_fcs_len           8
#define ERF_META_TAG_mask_ipv4         9
#define ERF_META_TAG_mask_cidr         10

#define ERF_META_TAG_org_name          11
#define ERF_META_TAG_name              12
#define ERF_META_TAG_descr             13
#define ERF_META_TAG_config            14
#define ERF_META_TAG_datapipe          15
#define ERF_META_TAG_app_name          16
#define ERF_META_TAG_os                17
#define ERF_META_TAG_hostname          18
#define ERF_META_TAG_user              19
#define ERF_META_TAG_model             20
#define ERF_META_TAG_fw_version        21
#define ERF_META_TAG_serial_no         22
#define ERF_META_TAG_ts_offset         23
#define ERF_META_TAG_ts_clock_freq     24
#define ERF_META_TAG_tzone             25
#define ERF_META_TAG_tzone_name        26
#define ERF_META_TAG_loc_lat           27
#define ERF_META_TAG_loc_long          28
#define ERF_META_TAG_snaplen           29
#define ERF_META_TAG_card_num          30
#define ERF_META_TAG_module_num        31
#define ERF_META_TAG_access_num        32
#define ERF_META_TAG_stream_num        33
#define ERF_META_TAG_loc_name          34
#define ERF_META_TAG_parent_file       35
#define ERF_META_TAG_filter            36
#define ERF_META_TAG_flow_hash_mode    37
#define ERF_META_TAG_tunneling_mode    38
#define ERF_META_TAG_npb_format        39
#define ERF_META_TAG_mem               40
#define ERF_META_TAG_datamine_id       41
#define ERF_META_TAG_rotfile_id        42
#define ERF_META_TAG_rotfile_name      43
#define ERF_META_TAG_dev_name          44
#define ERF_META_TAG_dev_path          45
#define ERF_META_TAG_loc_descr         46
#define ERF_META_TAG_app_version       47
#define ERF_META_TAG_cpu_affinity      48
#define ERF_META_TAG_cpu               49
#define ERF_META_TAG_cpu_phys_cores    50
#define ERF_META_TAG_cpu_numa_nodes    51
#define ERF_META_TAG_dag_attribute     52
#define ERF_META_TAG_dag_version       53
#define ERF_META_TAG_stream_flags      54
#define ERF_META_TAG_entropy_threshold 55
#define ERF_META_TAG_smart_trunc_default 56
#define ERF_META_TAG_ext_hdrs_added    57
#define ERF_META_TAG_ext_hdrs_removed  58
#define ERF_META_TAG_relative_snaplen  59
#define ERF_META_TAG_temperature       60
#define ERF_META_TAG_power             61
#define ERF_META_TAG_vendor            62
#define ERF_META_TAG_cpu_threads       63

#define ERF_META_TAG_if_num            64
#define ERF_META_TAG_if_vc             65
#define ERF_META_TAG_if_speed          66
#define ERF_META_TAG_if_ipv4           67
#define ERF_META_TAG_if_ipv6           68
#define ERF_META_TAG_if_mac            69
#define ERF_META_TAG_if_eui            70
#define ERF_META_TAG_if_ib_gid         71
#define ERF_META_TAG_if_ib_lid         72
#define ERF_META_TAG_if_wwn            73
#define ERF_META_TAG_if_fc_id          74
#define ERF_META_TAG_if_tx_speed       75
#define ERF_META_TAG_if_erf_type       76
#define ERF_META_TAG_if_link_type      77
#define ERF_META_TAG_if_sfp_type       78
#define ERF_META_TAG_if_rx_power       79
#define ERF_META_TAG_if_tx_power       80
#define ERF_META_TAG_if_link_status    81
#define ERF_META_TAG_if_phy_mode       82
#define ERF_META_TAG_if_port_type      83
#define ERF_META_TAG_if_rx_latency     84
#define ERF_META_TAG_tap_mode          85
#define ERF_META_TAG_tap_fail_mode     86
#define ERF_META_TAG_watchdog_expired  87
#define ERF_META_TAG_watchdog_interval 88

#define ERF_META_TAG_src_ipv4          128
#define ERF_META_TAG_dest_ipv4         129
#define ERF_META_TAG_src_ipv6          130
#define ERF_META_TAG_dest_ipv6         131
#define ERF_META_TAG_src_mac           132
#define ERF_META_TAG_dest_mac          133
#define ERF_META_TAG_src_eui           134
#define ERF_META_TAG_dest_eui          135
#define ERF_META_TAG_src_ib_gid        136
#define ERF_META_TAG_dest_ib_gid       137
#define ERF_META_TAG_src_ib_lid        138
#define ERF_META_TAG_dest_ib_lid       139
#define ERF_META_TAG_src_wwn           140
#define ERF_META_TAG_dest_wwn          141
#define ERF_META_TAG_src_fc_id         142
#define ERF_META_TAG_dest_fc_id        143
#define ERF_META_TAG_src_port          144
#define ERF_META_TAG_dest_port         145
#define ERF_META_TAG_ip_proto          146
#define ERF_META_TAG_flow_hash         147
#define ERF_META_TAG_filter_match      148
#define ERF_META_TAG_filter_match_name 149
#define ERF_META_TAG_error_flags       150
#define ERF_META_TAG_initiator_pkts    151
#define ERF_META_TAG_responder_pkts    152
#define ERF_META_TAG_initiator_bytes   153
#define ERF_META_TAG_responder_bytes   154
#define ERF_META_TAG_initiator_min_entropy 155
#define ERF_META_TAG_responder_min_entropy 156
#define ERF_META_TAG_initiator_avg_entropy 157
#define ERF_META_TAG_responder_avg_entropy 158
#define ERF_META_TAG_initiator_max_entropy 159
#define ERF_META_TAG_responder_max_entropy 160
#define ERF_META_TAG_dpi_application   161
#define ERF_META_TAG_dpi_confidence    162
#define ERF_META_TAG_dpi_state         163
#define ERF_META_TAG_dpi_protocol_stack 164
#define ERF_META_TAG_flow_state        165
#define ERF_META_TAG_vlan_id           166
#define ERF_META_TAG_mpls_label        167
#define ERF_META_TAG_vlan_pcp          168
#define ERF_META_TAG_mpls_tc           169
#define ERF_META_TAG_dscp              170
#define ERF_META_TAG_initiator_mpls_label 171
#define ERF_META_TAG_responder_mpls_label 172
#define ERF_META_TAG_initiator_mpls_tc 173
#define ERF_META_TAG_responder_mpls_tc 174
#define ERF_META_TAG_initiator_ipv4    175
#define ERF_META_TAG_responder_ipv4    176
#define ERF_META_TAG_initiator_ipv6    177
#define ERF_META_TAG_responder_ipv6    178
#define ERF_META_TAG_initiator_mac     179
#define ERF_META_TAG_responder_mac     180
#define ERF_META_TAG_initiator_port    181
#define ERF_META_TAG_responder_port    182
#define ERF_META_TAG_initiator_retx    183
#define ERF_META_TAG_responder_retx    184
#define ERF_META_TAG_initiator_zwin    185
#define ERF_META_TAG_responder_zwin    186
#define ERF_META_TAG_initiator_tcp_flags 187
#define ERF_META_TAG_responder_tcp_flags 188
#define ERF_META_TAG_tcp_irtt          189

#define ERF_META_TAG_start_time        193
#define ERF_META_TAG_end_time          194
#define ERF_META_TAG_stat_if_drop      195
#define ERF_META_TAG_stat_frames       196
#define ERF_META_TAG_stat_bytes        197
#define ERF_META_TAG_stat_cap          198
#define ERF_META_TAG_stat_cap_bytes    199
#define ERF_META_TAG_stat_os_drop      200
#define ERF_META_TAG_stat_ds_lctr      201
#define ERF_META_TAG_stat_filter_match 202
#define ERF_META_TAG_stat_filter_drop  203
#define ERF_META_TAG_stat_too_short    204
#define ERF_META_TAG_stat_too_long     205
#define ERF_META_TAG_stat_rx_error     206
#define ERF_META_TAG_stat_fcs_error    207
#define ERF_META_TAG_stat_aborted      208
#define ERF_META_TAG_stat_proto_error  209
#define ERF_META_TAG_stat_b1_error     210
#define ERF_META_TAG_stat_b2_error     211
#define ERF_META_TAG_stat_b3_error     212
#define ERF_META_TAG_stat_rei_error    213
#define ERF_META_TAG_stat_drop         214
#define ERF_META_TAG_stat_buf_drop     215
#define ERF_META_TAG_stream_drop       216
#define ERF_META_TAG_stream_buf_drop   217
#define ERF_META_TAG_pkt_drop          218
#define ERF_META_TAG_record_drop       219
#define ERF_META_TAG_bandwidth         220
#define ERF_META_TAG_duration          221
#define ERF_META_TAG_top_index         222
#define ERF_META_TAG_concurrent_flows  223
#define ERF_META_TAG_active_flows      224
#define ERF_META_TAG_created_flows     225
#define ERF_META_TAG_deleted_flows     226
#define ERF_META_TAG_active_endpoints  227
#define ERF_META_TAG_tx_pkts           228
#define ERF_META_TAG_tx_bytes          229
#define ERF_META_TAG_rx_bandwidth      230
#define ERF_META_TAG_tx_bandwidth      231
#define ERF_META_TAG_records           232
#define ERF_META_TAG_record_bytes      233
#define ERF_META_TAG_pkt_drop_bytes    234
#define ERF_META_TAG_record_drop_bytes 235
#define ERF_META_TAG_drop_bandwidth    236
#define ERF_META_TAG_retx_pkts         237
#define ERF_META_TAG_zwin_pkts         238

#define ERF_META_TAG_ns_host_ipv4      256
#define ERF_META_TAG_ns_host_ipv6      257
#define ERF_META_TAG_ns_host_mac       258
#define ERF_META_TAG_ns_host_eui       259
#define ERF_META_TAG_ns_host_ib_gid    260
#define ERF_META_TAG_ns_host_ib_lid    261
#define ERF_META_TAG_ns_host_wwn       262
#define ERF_META_TAG_ns_host_fc_id     263
#define ERF_META_TAG_ns_dns_ipv4       264
#define ERF_META_TAG_ns_dns_ipv6       265

#define ERF_META_TAG_exthdr            321
#define ERF_META_TAG_pcap_ng_block     322
#define ERF_META_TAG_asn1              323
#define ERF_META_TAG_section_ref       324

#define ERF_META_TAG_clk_source             384
#define ERF_META_TAG_clk_state              385
#define ERF_META_TAG_clk_threshold          386
#define ERF_META_TAG_clk_correction         387
#define ERF_META_TAG_clk_failures           388
#define ERF_META_TAG_clk_resyncs            389
#define ERF_META_TAG_clk_phase_error        390
#define ERF_META_TAG_clk_input_pulses       391
#define ERF_META_TAG_clk_rejected_pulses    392
#define ERF_META_TAG_clk_phc_index          393
#define ERF_META_TAG_clk_phc_offset         394
#define ERF_META_TAG_clk_timebase           395
#define ERF_META_TAG_clk_descr              396
#define ERF_META_TAG_clk_out_source         397
#define ERF_META_TAG_clk_link_mode          398
#define ERF_META_TAG_ptp_domain_num         399
#define ERF_META_TAG_ptp_steps_removed      400
#define ERF_META_TAG_ptp_offset_from_master 401
#define ERF_META_TAG_ptp_mean_path_delay    402
#define ERF_META_TAG_ptp_parent_identity    403
#define ERF_META_TAG_ptp_parent_port_num    404
#define ERF_META_TAG_ptp_gm_identity        405
#define ERF_META_TAG_ptp_gm_clock_quality   406
#define ERF_META_TAG_ptp_current_utc_offset 407
#define ERF_META_TAG_ptp_time_properties    408
#define ERF_META_TAG_ptp_time_source        409
#define ERF_META_TAG_ptp_clock_identity     410
#define ERF_META_TAG_ptp_port_num           411
#define ERF_META_TAG_ptp_port_state         412
#define ERF_META_TAG_ptp_delay_mechanism    413
#define ERF_META_TAG_clk_port_proto         414
#define ERF_META_TAG_ntp_status             415
#define ERF_META_TAG_ntp_stratum            416
#define ERF_META_TAG_ntp_rootdelay          417
#define ERF_META_TAG_ntp_rootdisp           418
#define ERF_META_TAG_ntp_offset             419
#define ERF_META_TAG_ntp_frequency          420
#define ERF_META_TAG_ntp_sys_jitter         421
#define ERF_META_TAG_ntp_peer_remote        422
#define ERF_META_TAG_ntp_peer_refid         423

 /*
  * The timestamp is 64bit unsigned fixed point little-endian value with
  * 32 bits for second and 32 bits for fraction.
  */
typedef uint64_t erf_timestamp_t;

typedef struct erf_record {
	erf_timestamp_t	ts;
	uint8_t		type;
	uint8_t		flags;
	uint16_t	rlen;
	uint16_t	lctr;
	uint16_t	wlen;
} erf_header_t;

typedef struct erf_mc_hdr {
	uint32_t	mc;
} erf_mc_header_t;

typedef struct erf_aal2_hdr {
	uint32_t	aal2;
} erf_aal2_header_t;

typedef struct erf_eth_hdr {
	uint8_t offset;
	uint8_t pad;
} erf_eth_header_t;

union erf_subhdr {
  struct erf_mc_hdr mc_hdr;
  struct erf_aal2_hdr aal2_hdr;
  struct erf_eth_hdr eth_hdr;
};

#endif /* __W_ERF_RECORD_H__ */

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
