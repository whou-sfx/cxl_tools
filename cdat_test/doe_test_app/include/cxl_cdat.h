/*
 * Copyright (C) 2021 Avery Design Systems, Inc.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the LICENSE file in the top-level directory.
 */

#ifndef CXL_CDAT_H
#define CXL_CDAT_H

#include "pcie_doe.h"
#include "cxl.h"

enum {
    CXL_DOE_COMPLIANCE             = 0,
    CXL_DOE_TABLE_ACCESS           = 2,
    CXL_DOE_MAX_PROTOCOL
};

#define CXL_DOE_PROTOCOL_COMPLIANCE ((CXL_DOE_COMPLIANCE << 16) | CXL_VENDOR_ID)
#define CXL_DOE_PROTOCOL_CDAT     ((CXL_DOE_TABLE_ACCESS << 16) | CXL_VENDOR_ID)

/*
 * DOE CDAT Table Protocol (CXL Spec)
 */
#define CXL_DOE_TAB_REQ 0
#define CXL_DOE_TAB_RSP 0
#define CXL_DOE_TAB_TYPE_CDAT 0
#define CXL_DOE_TAB_ENT_MAX 0xFFFF

/* Read Entry Request, 8.1.11.1 Table 134 */
struct cxl_cdat {
    DOEHeader header;
    uint8_t req_code;
    uint8_t table_type;
    uint16_t entry_handle;
} __attribute__((__packed__));

/*
 * CDAT Table Structure (CDAT Spec)
 */
#define CXL_CDAT_REV 1

/* Data object header */
struct cdat_table_header {
    uint32_t length;    /* Length of table in bytes, including this header */
    uint8_t revision;   /* ACPI Specification minor version number */
    uint8_t checksum;   /* To make sum of entire table == 0 */
    uint8_t reserved[6];
    uint32_t sequence;  /* ASCII table signature */
} __attribute__((__packed__));


/* Read Entry Response, 8.1.11.1 Table 135 */


/* Values for subtable type in CDAT structures */
enum cdat_type {
    CDAT_TYPE_DSMAS = 0,
    CDAT_TYPE_DSLBIS = 1,
    CDAT_TYPE_DSMSCIS = 2,
    CDAT_TYPE_DSIS = 3,
    CDAT_TYPE_DSEMTS = 4,
    CDAT_TYPE_SSLBIS = 5,
    CDAT_TYPE_MAX
};

struct cdat_sub_header {
    uint8_t type;
    uint8_t reserved;
    uint16_t length;
};

/*
 * The DOE CDAT read response contains a CDAT read entry (either the
 * CDAT header or a structure).
 */
union cdat_data {
	struct cdat_table_header header;
	struct cdat_sub_header entry;
}__attribute__((__packed__));

/* CDAT Structure Subtables */
struct cdat_dsmas {
    struct cdat_sub_header header;
    uint8_t DSMADhandle;
    uint8_t flags;
    uint16_t reserved2;
    uint64_t DPA_base;
    uint64_t DPA_length;
} __attribute__((__packed__));

struct cdat_dslbis {
    struct cdat_sub_header header;
    uint8_t handle;
    uint8_t flags;
    uint8_t data_type;
    uint8_t reserved2;
    uint64_t entry_base_unit;
    uint16_t entry[3];
    uint16_t reserved3;
} __attribute__((__packed__));

struct cdat_dsmscis {
    struct cdat_sub_header header;
    uint8_t DSMASH_handle;
    uint8_t reserved2[3];
    uint64_t memory_side_cache_size;
    uint32_t cache_attributes;
} __attribute__((__packed__));

struct cdat_dsis {
    struct cdat_sub_header header;
    uint8_t flags;
    uint8_t handle;
    uint16_t reserved2;
} __attribute__((__packed__));

struct cdat_dsemts {
    struct cdat_sub_header header;
    uint8_t DSMAS_handle;
    uint8_t EFI_memory_type_attr;
    uint16_t reserved2;
    uint64_t DPA_offset;
    uint64_t DPA_length;
} __attribute__((__packed__));

struct cdat_sslbe {
    uint16_t port_x_id;
    uint16_t port_y_id;
    uint16_t latency_bandwidth;
    uint16_t reserved;
} __attribute__((__packed__));

struct cdat_sslbis_header {
    struct cdat_sub_header header;
    uint8_t data_type;
    uint8_t reserved2[3];
    uint64_t entry_base_unit;
} __attribute__((__packed__));

void do_cdat_req(pcie_dev *dev, uint32_t idx);
void test_cdat(pcie_dev *dev);
#endif /* CXL_CDAT_H */
