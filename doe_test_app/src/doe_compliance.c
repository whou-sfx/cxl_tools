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
#include "cxl.h"
#include "cxl_compliance.h"

uint32_t comp_buf[PCI_DOE_MAX_DW_SIZE + 1] = {0};

static void do_compliance_req(pcie_dev *dev, uint32_t idx)
{
    uint32_t req_len, doe_cap;
    CompReq req;

    switch (idx) {
    case CXL_COMP_MODE_CAP:
        req_len = sizeof(struct cxl_compliance_mode_cap);
        break;
    case CXL_COMP_MODE_BOGUS:
        req_len = sizeof(struct cxl_compliance_mode_inject_bogus_writes);
        req.inject_bogus_writes.count = 0x12;
        req.inject_bogus_writes.pattern = 0x3456;
        break;
    case CXL_COMP_MODE_STATUS:
    case CXL_COMP_MODE_HALT:
    case CXL_COMP_MODE_MULT_WR_STREAM:
    case CXL_COMP_MODE_PRO_CON:
    case CXL_COMP_MODE_INJ_POISON:
    case CXL_COMP_MODE_INJ_CRC:
    case CXL_COMP_MODE_INJ_FC:
    case CXL_COMP_MODE_TOGGLE_CACHE:
    case CXL_COMP_MODE_INJ_MAC:
    case CXL_COMP_MODE_INS_UNEXP_MAC:
    case CXL_COMP_MODE_INJ_VIRAL:
        req_len = sizeof(struct cxl_compliance_mode_inject_viral);
        req.inject_viral.protocol = 2;
        break;
    case CXL_COMP_MODE_INJ_ALMP:
    case CXL_COMP_MODE_IGN_ALMP:
    case CXL_COMP_MODE_INJ_BIT_ERR:
    case CXL_COMP_MODE_INJ_MEDIA_POSION:
        req_len = sizeof(struct cxl_compliance_mode_inject_media_posion);
        req.inject_media_posion.protol = 0x2;  /*0x2 for cxl.mem*/
        req.inject_media_posion.action = 0;  /* 0 for inject; 1 for clear*/
        req.inject_media_posion.dpa = 0x20000;
        req.inject_media_posion.data = 0x0;
        break;
    default:
        break;
    }

    req = (CompReq) {
        .header = {
            .doe_header = {
                .vendor_id = CXL_VENDOR_ID,
                .doe_type = CXL_DOE_COMPLIANCE,
                .length = DIV_ROUND_UP(req_len, sizeof(uint32_t)),
            },
            .req_code = idx,
            .version = 0xcc,
        },
    };

    doe_cap = 0xd00;
    /** doe_cap = doe_get_cap_by_prot(dev, */
    /**         DATA_OBJ_BUILD_HEADER1(CXL_VENDOR_ID, CXL_DOE_COMPLIANCE)); */
    memcpy(comp_buf + 1, &req, req.header.doe_header.length * sizeof(uint32_t));
    doe_exchange_object(dev, doe_cap, comp_buf);
}

/* TODO */
void test_compliance(pcie_dev *dev)
{
    int i;
    CompRspHeader *rsp_hdr;

    memset(comp_buf, 0x00, sizeof(comp_buf));
    rsp_hdr = (CompRspHeader *)comp_buf;

    printf("Compaliance Query Cap\n");
    do_compliance_req(dev, CXL_COMP_MODE_CAP);

    printf("VID = %x\n", rsp_hdr->doe_header.vendor_id);
    printf("DOE Type = %x\n", rsp_hdr->doe_header.doe_type);
    printf("Len(DW) = %x\n", rsp_hdr->doe_header.length);

    printf("resp_code = %x\n", rsp_hdr->rsp_code);
    printf("Ver = %x\n", rsp_hdr->version);
    printf("Len(B) = %x\n", rsp_hdr->length);

    i = DIV_ROUND_UP(sizeof(CompRspHeader), 4);
    for (; i < rsp_hdr->doe_header.length; i++) {
        printf("\tcomp_buf[%02x] = %08x\n", i, comp_buf[i]);
    }


    printf("Compliance Inject Viral\n");
    do_compliance_req(dev, CXL_COMP_MODE_INJ_VIRAL);

    printf("VID = %x\n", rsp_hdr->doe_header.vendor_id);
    printf("DOE Type = %x\n", rsp_hdr->doe_header.doe_type);
    printf("Len(DW) = %x\n", rsp_hdr->doe_header.length);

    printf("Type = %x\n", rsp_hdr->rsp_code);
    printf("Ver = %x\n", rsp_hdr->version);
    printf("Len(B) = %x\n", rsp_hdr->length);

    i = DIV_ROUND_UP(sizeof(CompRspHeader), 4);
    for (; i < rsp_hdr->doe_header.length; i++) {
        printf("\tcomp_buf[%02x] = %08x\n", i, comp_buf[i]);
    }

    printf("Compliance Inject Media Error\n");
    do_compliance_req(dev, CXL_COMP_MODE_INJ_MEDIA_POSION);

    printf("VID = %x\n", rsp_hdr->doe_header.vendor_id);
    printf("DOE Type = %x\n", rsp_hdr->doe_header.doe_type);
    printf("Len(DW) = %x\n", rsp_hdr->doe_header.length);

    printf("Type = %x\n", rsp_hdr->rsp_code);
    printf("Ver = %x\n", rsp_hdr->version);
    printf("Len(B) = %x\n", rsp_hdr->length);

    i = DIV_ROUND_UP(sizeof(CompRspHeader), 4);
    for (; i < rsp_hdr->doe_header.length; i++) {
        printf("\tcomp_buf[%02x] = %08x\n", i, comp_buf[i]);
    }

}
