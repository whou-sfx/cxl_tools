/*
 * Copyright (C) 2021 Avery Design Systems, Inc.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the LICENSE file in the top-level directory.
 */

#ifndef DOE_DISCOVERY_H
#define DOE_DISCOVERY_H

#include "pcie_doe.h"

typedef struct doe_discovery doe_discovery;
typedef struct doe_discovery_rsp doe_discovery_rsp;

struct doe_discovery {
    DOEHeader header;
    uint8_t index;
    uint8_t reserved[3];
} __attribute__((__packed__));

struct doe_discovery_rsp {
    DOEHeader header;
    union {
        struct {
            uint16_t vendor_id;
            uint8_t doe_type;
            uint8_t next_index;
        };
        uint32_t data;
    };
} __attribute__((__packed__));

int doe_discovery_one(pcie_dev *dev, uint32_t doe_cap, uint32_t idx,
                      doe_discovery_rsp *rsp);
void doe_discovery_all(pcie_dev *dev);
void test_discovery(pcie_dev *dev);

#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#endif
