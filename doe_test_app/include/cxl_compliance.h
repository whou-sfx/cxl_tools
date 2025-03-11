/*
 * CXL Compliance Structure
 *
 * Copyright (C) 2021 Avery Design Systems, Inc.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#ifndef CXL_COMPL_H
#define CXL_COMPL_H

#include <inttypes.h>
#include "pcie_doe.h"
#include "cxl.h"

/*
 * Reference:
 *   Compute Express Link (CXL) Specification, Rev. 2.0, Oct. 2020
 */
/* Compliance Mode Data Object Header - 14.16.4 Table 275 */
#define CXL_DOE_COMPLIANCE        0
#define CXL_DOE_PROTOCOL_COMPLIANCE ((CXL_DOE_COMPLIANCE << 16) | CXL_VENDOR_ID)

/* Compliance Mode Return Values - 14.16.4 Table 276 */
enum comp_status {
    CXL_COMP_MODE_RET_SUCC,
    CXL_COMP_MODE_RET_NOT_AUTH,
    CXL_COMP_MODE_RET_UNKNOWN_FAIL,
    CXL_COMP_MODE_RET_UNSUP_INJ_FUNC,
    CXL_COMP_MODE_RET_INTERNAL_ERR,
    CXL_COMP_MODE_RET_BUSY,
    CXL_COMP_MODE_RET_NOT_INIT,
};

/* Compliance Mode Types - 14.16.4 */
enum comp_type {
    CXL_COMP_MODE_CAP,
    CXL_COMP_MODE_STATUS,
    CXL_COMP_MODE_HALT,
    CXL_COMP_MODE_MULT_WR_STREAM,
    CXL_COMP_MODE_PRO_CON,
    CXL_COMP_MODE_BOGUS,
    CXL_COMP_MODE_INJ_POISON,
    CXL_COMP_MODE_INJ_CRC,
    CXL_COMP_MODE_INJ_FC,
    CXL_COMP_MODE_TOGGLE_CACHE,
    CXL_COMP_MODE_INJ_MAC,
    CXL_COMP_MODE_INS_UNEXP_MAC,
    CXL_COMP_MODE_INJ_VIRAL,
    CXL_COMP_MODE_INJ_ALMP,
    CXL_COMP_MODE_IGN_ALMP,
    CXL_COMP_MODE_INJ_BIT_ERR,
    CXL_COMP_MODE_INJ_MEDIA_POSION,
};

typedef struct compliance_req_header CompReqHeader;
typedef struct compliance_rsp_header CompRspHeader;

struct compliance_req_header {
    DOEHeader doe_header;
    uint8_t req_code;
    uint8_t version;
    uint16_t reserved;
} __attribute__((__packed__));

struct compliance_rsp_header {
    DOEHeader doe_header;
    uint8_t rsp_code;
    uint8_t version;
    uint8_t length;
} __attribute__((__packed__));

/* Special Patterns of response */
struct status_rsp {
    CompRspHeader header;
    uint8_t status;
} __attribute__((__packed__));

struct len_rsvd_rsp {
    /* The length field in header is reserved. */
    CompRspHeader header;
    uint8_t reserved[5];
} __attribute__((__packed__));

/* 14.16.4.1 Table 277 */
struct cxl_compliance_mode_cap {
    CompReqHeader header;
} __attribute__((__packed__));

/* 14.16.4.1 Table 278 */
struct cxl_compliance_mode_cap_rsp {
    CompRspHeader header;
    uint8_t status;
    uint64_t available_cap_bitmask;
    uint64_t enabled_cap_bitmask;
} __attribute__((__packed__));

/* 14.16.4.2 Table 279 */
struct cxl_compliance_mode_status {
    CompReqHeader header;
} __attribute__((__packed__));

/* 14.16.4.2 Table 280 */
struct cxl_compliance_mode_status_rsp {
    CompRspHeader header;
    uint32_t cap_bitfield;
    uint16_t cache_size;
    uint8_t cache_size_units;
} __attribute__((__packed__));

/* 14.16.4.3 Table 281 */
struct cxl_compliance_mode_halt {
    CompReqHeader header;
} __attribute__((__packed__));

/* 14.16.4.3 Table 282 */
#define cxl_compliance_mode_halt_rsp status_rsp

/* 14.16.4.4 Table 283 */
struct cxl_compliance_mode_multiple_write_streaming {
    CompReqHeader header;
    uint8_t protocol;
    uint8_t virtual_addr;
    uint8_t self_checking;
    uint8_t verify_read_semantics;
    uint8_t num_inc;
    uint8_t num_sets;
    uint8_t num_loops;
    uint8_t reserved2;
    uint64_t start_addr;
    uint64_t write_addr;
    uint64_t writeback_addr;
    uint64_t byte_mask;
    uint32_t addr_incr;
    uint32_t set_offset;
    uint32_t pattern_p;
    uint32_t inc_pattern_b;
} __attribute__((__packed__));

/* 14.16.4.4 Table 284 */
#define cxl_compliance_mode_multiple_write_streaming_rsp status_rsp

/* 14.16.4.5 Table 285 */
struct cxl_compliance_mode_producer_consumer {
    CompReqHeader header;
    uint8_t protocol;
    uint8_t num_inc;
    uint8_t num_sets;
    uint8_t num_loops;
    uint8_t write_semantics;
    uint8_t reserved2[3];
    uint64_t start_addr;
    uint64_t byte_mask;
    uint32_t addr_incr;
    uint32_t set_offset;
    uint32_t pattern;
} __attribute__((__packed__));

/* 14.16.4.5 Table 286 */
#define cxl_compliance_mode_producer_consumer_rsp status_rsp

/* 14.16.4.6 Table 287 */
struct cxl_compliance_mode_inject_bogus_writes {
    CompReqHeader header;
    uint8_t count;
    uint8_t reserved;
    uint32_t pattern;
} __attribute__((__packed__));

/* 14.16.4.6 Table 288 */
#define cxl_compliance_mode_inject_bogus_writes_rsp status_rsp

/* 14.16.4.7 Table 289 */
struct cxl_compliance_mode_inject_poison {
    CompReqHeader header;
    uint8_t protocol;
} __attribute__((__packed__));

/* 14.16.4.7 Table 290 */
#define cxl_compliance_mode_inject_poison_rsp status_rsp

/* 14.16.4.8 Table 291 */
struct cxl_compliance_mode_inject_crc {
    CompReqHeader header;
    uint8_t num_bits_flip;
    uint8_t num_flits_inj;
} __attribute__((__packed__));

/* 14.16.4.8 Table 292 */
#define cxl_compliance_mode_inject_crc_rsp status_rsp

/* 14.16.4.9 Table 293 */
struct cxl_compliance_mode_inject_flow_control {
    CompReqHeader header;
    uint8_t inj_flow_control;
} __attribute__((__packed__));

/* 14.16.4.9 Table 294 */
#define cxl_compliance_mode_inject_flow_control_rsp status_rsp

/* 14.16.4.10 Table 295 */
struct cxl_compliance_mode_toggle_cache_flush {
    CompReqHeader header;
    uint8_t cache_flush_control;
} __attribute__((__packed__));

/* 14.16.4.10 Table 296 */
#define cxl_compliance_mode_toggle_cache_flush_rsp status_rsp

/* 14.16.4.11 Table 297 */
struct cxl_compliance_mode_inject_mac_delay {
    CompReqHeader header;
    uint8_t enable;
    uint8_t mode;
    uint8_t delay;
} __attribute__((__packed__));

/* 14.16.4.11 Table 298 */
#define cxl_compliance_mode_inject_mac_delay_rsp status_rsp

/* 14.16.4.12 Table 299 */
struct cxl_compliance_mode_insert_unexp_mac {
    CompReqHeader header;
    uint8_t opcode;
    uint8_t mode;
} __attribute__((__packed__));

/* 14.16.4.12 Table 300 */
#define cxl_compliance_mode_insert_unexp_mac_rsp status_rsp

/* 14.16.4.13 Table 301 */
struct cxl_compliance_mode_inject_viral {
    CompReqHeader header;
    uint8_t protocol;
} __attribute__((__packed__));

/* 14.16.4.13 Table 302 */
#define cxl_compliance_mode_inject_viral_rsp status_rsp

/* 14.16.4.14 Table 303 */
struct cxl_compliance_mode_inject_almp {
    CompReqHeader header;
    uint8_t opcode;
    uint8_t reserved2[3];
} __attribute__((__packed__));

/* 14.16.4.14 Table 304 */
#define cxl_compliance_mode_inject_almp_rsp len_rsvd_rsp

/* 14.16.4.15 Table 305 */
struct cxl_compliance_mode_ignore_almp {
    CompReqHeader header;
    uint8_t opcode;
    uint8_t reserved2[3];
} __attribute__((__packed__));

/* 14.16.4.15 Table 306 */
#define cxl_compliance_mode_ignore_almp_rsp len_rsvd_rsp

/* 14.16.4.16 Table 307 */
struct cxl_compliance_mode_inject_bit_error {
    CompReqHeader header;
    uint8_t opcode;
} __attribute__((__packed__));

/*14.16.3.17 Inject meida posion*/
struct cxl_compliance_mode_inject_media_posion {
    CompReqHeader header;

    uint8_t protol;
    uint8_t rsvd2;
    uint8_t action; /*0 inject, 1 clear*/
    uint8_t rsvd3;

    uint64_t dpa;
    uint64_t data;

} __attribute__((__packed__));


/* 14.16.4.16 Table 308 */
#define cxl_compliance_mode_inject_bit_error_rsp len_rsvd_rsp

typedef struct ComplianceObject ComplianceObject;

typedef union doe_u {
    CompReqHeader header;

    struct cxl_compliance_mode_cap cap;
    struct cxl_compliance_mode_status status;
    struct cxl_compliance_mode_halt halt;
    struct cxl_compliance_mode_multiple_write_streaming
        multiple_write_streaming;
    struct cxl_compliance_mode_producer_consumer producer_consumer;
    struct cxl_compliance_mode_inject_bogus_writes
        inject_bogus_writes;
    struct cxl_compliance_mode_inject_poison inject_poison;
    struct cxl_compliance_mode_inject_crc inject_crc;
    struct cxl_compliance_mode_inject_flow_control
        inject_flow_control;
    struct cxl_compliance_mode_toggle_cache_flush
        toggle_cache_flush;
    struct cxl_compliance_mode_inject_mac_delay inject_mac_delay;
    struct cxl_compliance_mode_insert_unexp_mac insert_unexp_mac;
    struct cxl_compliance_mode_inject_viral inject_viral;
    struct cxl_compliance_mode_inject_almp inject_almp;
    struct cxl_compliance_mode_ignore_almp ignore_almp;
    struct cxl_compliance_mode_inject_bit_error ignore_bit_error;
    struct cxl_compliance_mode_inject_media_posion inject_media_posion;
} CompReq;

typedef union doe_rsp_u {
    CompRspHeader header;

    struct cxl_compliance_mode_cap_rsp cap_rsp;
    struct cxl_compliance_mode_status_rsp status_rsp;
    struct cxl_compliance_mode_halt_rsp halt_rsp;
    struct cxl_compliance_mode_multiple_write_streaming_rsp
        multiple_write_streaming_rsp;
    struct cxl_compliance_mode_producer_consumer_rsp producer_consumer_rsp;
    struct cxl_compliance_mode_inject_bogus_writes_rsp
        inject_bogus_writes_rsp;
    struct cxl_compliance_mode_inject_poison_rsp inject_poison_rsp;
    struct cxl_compliance_mode_inject_crc_rsp inject_crc_rsp;
    struct cxl_compliance_mode_inject_flow_control_rsp
        inject_flow_control_rsp;
    struct cxl_compliance_mode_toggle_cache_flush_rsp
        toggle_cache_flush_rsp;
    struct cxl_compliance_mode_inject_mac_delay_rsp inject_mac_delay_rsp;
    struct cxl_compliance_mode_insert_unexp_mac_rsp insert_unexp_mac_rsp;
    struct cxl_compliance_mode_inject_viral inject_viral_rsp;
    struct cxl_compliance_mode_inject_almp_rsp inject_almp_rsp;
    struct cxl_compliance_mode_ignore_almp_rsp ignore_almp_rsp;
    struct cxl_compliance_mode_inject_bit_error_rsp ignore_bit_error_rsp;
} CompRsp;

struct ComplianceObject {
    CompRsp response;
} __attribute__((__packed__));

void test_compliance(pcie_dev *dev);
#endif /* CXL_COMPL_H */
