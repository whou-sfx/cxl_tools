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
#include <stdlib.h>
#include <assert.h>

#include "doe_discovery.h"

int doe_discovery_one(pcie_dev *dev, uint32_t doe_cap,
                      uint32_t idx, doe_discovery_rsp *rsp)
{
    uint32_t buf[PCI_DOE_MAX_DW_SIZE + 1], rsp_len;
    doe_discovery req = {
        .header = {
            .vendor_id = PCI_DOE_PCI_SIG_VID,
            .doe_type = PCI_SIG_DOE_DISCOVERY,
            .length = DIV_ROUND_UP(sizeof(doe_discovery), sizeof(uint32_t)),
        },
        .index = idx,
    };

    memcpy(buf + 1, &req, req.header.length * sizeof(uint32_t));
    doe_exchange_object(dev, doe_cap, buf);

    rsp_len = ((DOEHeader *)buf)->length;
    memcpy(rsp, buf, rsp_len * sizeof(uint32_t));

    return 0;
}

void doe_discovery_all(pcie_dev *dev)
{
    uint32_t idx;
    doe_discovery_rsp rsp = {0};
    int rc = 0;
    DOEcap *doe_cap = dev->doe_cap_head;
    DOEprot **prot;

    for (; doe_cap; doe_cap = doe_cap->next) {
        idx = 0;
        prot = &doe_cap->prot_head;

        do {
            rc = doe_discovery_one(dev, doe_cap->cap, idx, &rsp);

            if (rc) {
                break;
            }

            *prot = malloc(sizeof(DOEprot));
            (*prot)->prot = DATA_OBJ_BUILD_HEADER1(rsp.vendor_id, rsp.doe_type);

#if 0
            if (idx == 0) {
                assert((*prot)->prot == PCI_DOE_PROTOCOL_DISCOVERY);
            }
#endif

            prot = &(*prot)->next;

            idx = rsp.next_index;
        } while (idx);
    }
}

void test_discovery(pcie_dev *dev)
{
    DOEcap *doe_cap;
    DOEprot *prot;

    doe_discovery_all(dev);

    for (doe_cap = dev->doe_cap_head; doe_cap; doe_cap = doe_cap->next) {
        printf("cap off = %x\n", doe_cap->cap);
        for (prot = doe_cap->prot_head; prot; prot = prot->next) {
            printf("\tprotocol = %08x\n", prot->prot);
        }
    }
}
