/*
 * Copyright (C) 2021 Avery Design Systems, Inc.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the LICENSE file in the top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include "pcie.h"
#include "cxl.h"

int init_cap_offset(pcie_dev *dev)
{
    uint32_t cap_offset, reg_val, reg_val2;
    DOEcap **doe = &dev->doe_cap_head;
    DVSECcap **dvsec = &dev->dvsec_cap_head;

    printf("Reading config space of PCI device\n");

    for (cap_offset = config_read(dev->pdev, PCI_CAPABILITY_LIST);
         cap_offset; cap_offset = PCI_CAP_NEXT(reg_val)) {
        reg_val = config_read(dev->pdev, cap_offset);

        switch (PCI_CAP_ID(reg_val)) {
        case PCI_CAP_ID_EXP:
            dev->ext_cap = cap_offset;
            break;
        default:
            break;
        }
    }

    for (cap_offset = PCIE_EXT_CAP_OFFSET; cap_offset;
         cap_offset = PCI_EXT_CAP_NEXT(reg_val)) {
        reg_val = config_read(dev->pdev, cap_offset);

        switch (PCI_EXT_CAP_ID(reg_val)) {
        case PCI_EXT_CAP_ID_DVSEC:
            *dvsec = malloc(sizeof(DVSECcap));
            (*dvsec)->cap = cap_offset;

            reg_val2 = config_read(dev->pdev, cap_offset + PCI_DVSEC_HEADER1);
            (*dvsec)->vendor_id = PCI_EXT_DVSEC_VEN_ID(reg_val2);
            (*dvsec)->revision = PCI_EXT_DVSEC_REV(reg_val2);
            (*dvsec)->length = PCI_EXT_DVSEC_LEN(reg_val2);

            reg_val2 = config_read(dev->pdev, cap_offset + PCI_DVSEC_HEADER2);
            (*dvsec)->id = PCI_EXT_DVSEC_ID(reg_val2);

            dvsec = &(*dvsec)->next;
            break;
        case PCI_EXT_CAP_ID_DOE:
            *doe = malloc(sizeof(DOEcap));
            (*doe)->cap = cap_offset;
            doe = &(*doe)->next;
            break;
        default:
            break;
        }
    }

    return 0;
}

void config_write(int fd, uint32_t addr, uint32_t data)
{
    pwrite(fd, &data, sizeof(uint32_t), addr);
    /* printf("[write] addr: %03x, data: %08x\n", addr, data); */
}

uint32_t config_read(int fd, uint32_t addr)
{
    uint32_t data;

    pread(fd, &data, sizeof(uint32_t), addr);
    /* printf("[read] addr: %03x, data: %08x\n", addr, data); */

    return data;
}
