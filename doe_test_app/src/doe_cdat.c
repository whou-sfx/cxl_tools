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
uint32_t  cdat_tbl[PCI_DOE_MAX_DW_SIZE + 1] = {0};


void do_cdat_req(pcie_dev *dev, uint32_t idx)
{
    int doe_cap;
    struct cxl_cdat req = {
        .doe_hdr = {
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
    memcpy(buf + 1, &req, req.doe_hdr.length * sizeof(uint32_t));
    doe_exchange_object(dev, doe_cap, buf);
}

static unsigned char cdat_checksum(void *buf, size_t size)
{
    unsigned char sum, *data = buf;
    size_t i;

    for (sum = 0, i = 0; i < size; i++)
       sum += data[i];
    return sum;
}

void test_cdat(pcie_dev *dev)
{
    int i;
    uint32_t idx = 0, len;
    int tbl_size = 0;
    static int tbl_offset = 0;
    struct rsp_header {
        struct cxl_cdat_rsp hdr;
        union cdat_data     data;
    };
    struct rsp_header *rsp_hdr;

    rsp_hdr = (struct rsp_header *)buf;

    while (idx != CXL_DOE_TAB_ENT_MAX) {
        int payload = 0;
        do_cdat_req(dev, idx);
        if (idx == 0) {
            tbl_offset = 0;
            memset(cdat_tbl, 0x00, sizeof(cdat_tbl));
            tbl_size = rsp_hdr->data.tbl_hdr.length;
            printf("CDAT table header(len %d):\n", tbl_size);
        } else {
            switch (rsp_hdr->data.tbl_entry_hdr.type) {
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
        payload = (rsp_hdr->hdr.doe_hdr.length * sizeof(uint32_t)) - sizeof(struct cxl_cdat);
	printf("doe_hdr_len %ld, cxl_cdat_hdr %ld, payloalen %d\n", (rsp_hdr->hdr.doe_hdr.length * sizeof(uint32_t)), sizeof(struct cxl_cdat), payload);
        memcpy(((uint8_t *)(cdat_tbl)+tbl_offset), (void *) &(rsp_hdr->data.tbl_hdr), payload);
        tbl_offset += payload;

        i = sizeof(struct cxl_cdat_rsp) / 4;
        idx = rsp_hdr->hdr.entry_handle;
        len = rsp_hdr->hdr.doe_hdr.length;
        for (; i < len; i++) {
            printf("\tbuf[%02x] = %08x\n", i, buf[i]);
        }

        printf("next ent: %02x\n", idx);
    }

    /*dump the cdat table*/ 
    {
        struct cdat_table_header *tbl_hdr = (struct cdat_table_header *)cdat_tbl;
        printf("hdr len %d\n", tbl_hdr->length);
        printf("hdr rev %d\n", tbl_hdr->revision);
        printf("hdr checksum: 0x%x\n", tbl_hdr->checksum);
        printf("hdr seq 0x%x\n", tbl_hdr->sequence);
    
        for ( i = 0; i < tbl_size/sizeof(uint32_t); i++) {
                printf("\tcdat_tbl[%02x] = %08x\n", i, cdat_tbl[i]);
            }

    }
    if (cdat_checksum(cdat_tbl, tbl_size)) {
        printf("ERR: cdat tbl checksum uncorrect\n");
    } else {
	    printf("cdat tbl checksum pass\n");
    } 
}
