/*
 * Copyright (C) 2021 Avery Design Systems, Inc.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the LICENSE file in the top-level directory.
 */

#ifndef PCIE_DOE_H
#define PCIE_DOE_H

#include <stdbool.h>
#include "pcie.h"

#define DATA_OBJ_BUILD_HEADER1(v, p)  ((p << 16) | v)
#define PCI_DOE_PROTOCOL_DISCOVERY \
    DATA_OBJ_BUILD_HEADER1(PCI_DOE_PCI_SIG_VID, PCI_SIG_DOE_DISCOVERY)
#define PCI_DOE_PROTOCOL_CMA \
    DATA_OBJ_BUILD_HEADER1(PCI_DOE_PCI_SIG_VID, PCI_SIG_DOE_CMA)

/* Table 7-x3 */
#define DOE_DISCOVERY_IDX_MASK      0x000000ff

#define PCI_DOE_MAX_DW_SIZE (1 << 18)
#define PCI_DOE_PROTOCOL_MAX 256

/*******************************************************************************
 *
 * DOE Protocol - Data Object
 *
 ******************************************************************************/
typedef struct DOEHeader DOEHeader;

struct DOEHeader {
    uint16_t vendor_id;
    uint8_t doe_type;
    uint8_t reserved;
    struct {
        uint32_t length:18;
        uint32_t reserved2:14;
    };
} __attribute__((__packed__));

void doe_exchange_object(pcie_dev *dev, uint32_t doe_cap, void* buf);
void doe_submit_object(pcie_dev *dev, uint32_t doe_cap, void* obj);
void __doe_submit_object(pcie_dev *dev, uint32_t doe_cap, void* obj, uint32_t len);
uint32_t doe_read_mbox(pcie_dev *dev, uint32_t doe_cap);
bool doe_check_ready(pcie_dev *dev, uint32_t doe_cap);
void doe_wait(pcie_dev *dev, uint32_t doe_cap);
void doe_abort(pcie_dev *dev, uint32_t doe_cap);
void *doe_get_object(pcie_dev *dev, uint32_t doe_cap);
int doe_get_cap_by_prot(pcie_dev *dev, uint32_t prot);
#endif /* PCIE_DOE_H */
