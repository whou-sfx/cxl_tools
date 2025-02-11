/*
 * Copyright (C) 2021 Avery Design Systems, Inc.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the LICENSE file in the top-level directory.
 */

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include "utils.h"
#include "cxl_cdat.h"

uint32_t buf[PCI_DOE_MAX_DW_SIZE + 1] = {0};
uint32_t resp_buf[PCI_DOE_MAX_DW_SIZE + 1] = {0};


void do_cdat_req(pcie_dev *dev, uint32_t idx)
{
    int doe_cap;
    struct cxl_cdat req = {
        .header = {
            .vendor_id = CXL_VENDOR_ID,
            .doe_type = CXL_DOE_TABLE_ACCESS,
            .length = DIV_ROUND_UP(sizeof(req), sizeof(uint32_t)),
        },
        .req_code = CXL_DOE_TAB_REQ,
        .table_type = CXL_DOE_TAB_TYPE_CDAT,
        .entry_handle = idx,
    };

    //doe_cap = doe_get_cap_by_prot(dev,
            //DATA_OBJ_BUILD_HEADER1(CXL_VENDOR_ID, CXL_DOE_TABLE_ACCESS));

    doe_cap = 0xd00;
    memcpy(buf + 1, &req, req.header.length * sizeof(uint32_t));
    doe_exchange_object(dev, doe_cap, buf);
}

void test_cdat(pcie_dev *dev)
{
    int i;
    uint32_t idx = 0, len;
    struct rsp_header {
        struct cxl_cdat_rsp hdr;
        struct cdat_sub_header sub_hdr;
    };
    struct rsp_header *rsp_hdr;

    rsp_hdr = (struct rsp_header *)buf;

    while (idx != CXL_DOE_TAB_ENT_MAX) {
        do_cdat_req(dev, idx);
        if (idx == 0) {
            printf("CDAT table header:\n");
        } else {
            switch (rsp_hdr->sub_hdr.type) {
            case CDAT_TYPE_DSMAS:
                printf("DSMAS:\n");
                break;
            case CDAT_TYPE_DSLBIS:
                printf("DSLBIS:\n");
                break;
            case CDAT_TYPE_DSMSCIS:
                printf("DSMSCIS:\n");
                break;
            case CDAT_TYPE_DSIS:
                printf("DSIS:\n");
                break;
            case CDAT_TYPE_DSEMTS:
                printf("DSEMTS:\n");
                break;
            case CDAT_TYPE_SSLBIS:
                printf("SSLBIS:\n");
                break;
            }
        }
        i = sizeof(struct cxl_cdat_rsp) / 4;
        idx = rsp_hdr->hdr.entry_handle;
        len = rsp_hdr->hdr.header.length;

        printf("next ent: %02x\n", idx);
        for (; i < len; i++) {
            printf("\tbuf[%02x] = %08x\n", i, buf[i]);
        }
    }
}
