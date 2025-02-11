/*
 * Copyright (C) 2021 Avery Design Systems, Inc.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the LICENSE file in the top-level directory.
 */

#ifndef PCIE_H
#define PCIE_H

#include <inttypes.h>
#include <unistd.h>

#include <uapi/linux/pci_regs.h>
#include "pci_regs.h"

/* PCI DOE register defines 7.9.xx */
#define PCIE_EXT_CAP_OFFSET      0x100

/* Table 7-x2 */
#define PCI_DOE_PCI_SIG_VID     0x0001
#define PCI_SIG_DOE_DISCOVERY   0x00
#define PCI_SIG_DOE_CMA         0x01

typedef struct pcie_dev pcie_dev;
typedef struct DOEcap DOEcap;
typedef struct DVSECcap DVSECcap;
typedef struct DOEprot DOEprot;

struct DOEprot {
    uint32_t prot;

    DOEprot *next;
};

struct DOEcap {
    int cap;
    DOEprot *prot_head;

    DOEcap *next;
};

struct DVSECcap {
    int cap;
    uint16_t vendor_id;
    uint8_t revision;
    uint16_t length;
    uint16_t id;

    DVSECcap *next;
};

struct pcie_dev {
    int pdev;
    int cdev;

    int domain, bus, slot, func;

    int ext_cap;
    DOEcap *doe_cap_head;
    DVSECcap *dvsec_cap_head;
};

int init_cap_offset(pcie_dev *dev);
void config_write(int fd, uint32_t addr, uint32_t data);
uint32_t config_read(int fd, uint32_t addr);
#endif /* PCIE_H */
