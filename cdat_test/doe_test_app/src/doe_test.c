/*
 * Copyright (C) 2021 Avery Design Systems, Inc.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the LICENSE file in the top-level directory.
 */

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>

#include "pcie_doe.h"
#include "doe_discovery.h"

/*
 * Tests in this file are not the normal exchanges for DOE.
 * Should not use doe_exchange_object().
 */

void test_invalid_len(pcie_dev *dev)
{
    doe_discovery req = {
        .header = {
            .vendor_id = PCI_DOE_PCI_SIG_VID,
            .doe_type = PCI_SIG_DOE_DISCOVERY,
            .length = 1,
        },
        .index = 0,
    };

    __doe_submit_object(dev, dev->doe_cap_head->cap, &req, DIV_ROUND_UP(sizeof(req), sizeof(uint32_t)));

    assert(!doe_check_ready(dev, dev->doe_cap_head->cap));
}

void test_invalid_protocol(pcie_dev *dev)
{
    doe_abort(dev, dev->doe_cap_head->cap);
    doe_discovery req = {
        .header = {
            .vendor_id = 0x1234,
            .doe_type = PCI_SIG_DOE_DISCOVERY,
            .length = 1,
        },
        .index = 0,
    };

    doe_submit_object(dev, dev->doe_cap_head->cap, &req);

    assert(!doe_check_ready(dev, dev->doe_cap_head->cap));
}

void test_abort(pcie_dev *dev)
{
    uint32_t buf;
    doe_discovery req = {
        .header = {
            .vendor_id = PCI_DOE_PCI_SIG_VID,
            .doe_type = PCI_SIG_DOE_DISCOVERY,
            .length = DIV_ROUND_UP(sizeof(doe_discovery), 4),
        },
        .index = 0,
    };

    doe_submit_object(dev, dev->doe_cap_head->cap, &req);

    doe_wait(dev, dev->doe_cap_head->cap);

    buf = doe_read_mbox(dev, dev->doe_cap_head->cap);
    printf("buf: %x\n", buf);
    doe_abort(dev, dev->doe_cap_head->cap);

    assert(!doe_check_ready(dev, dev->doe_cap_head->cap));
}

void test_error(pcie_dev *dev)
{
    uint32_t st;
    doe_discovery_rsp *rsp;
    doe_discovery req = {
        .header = {
            .vendor_id = PCI_DOE_PCI_SIG_VID,
            .doe_type = PCI_SIG_DOE_DISCOVERY,
            .length = DIV_ROUND_UP(sizeof(doe_discovery), 4),
        },
        .index = 0,
    };
    doe_abort(dev, dev->doe_cap_head->cap);

    doe_submit_object(dev, dev->doe_cap_head->cap, &req);

    doe_wait(dev, dev->doe_cap_head->cap);

    rsp = doe_get_object(dev, dev->doe_cap_head->cap);
    printf("rsp.vendor_id = %x, rsp.doe_type = %x, rsp.length = %x, rsp.idx = %x\n",
           rsp->header.vendor_id, rsp->header.doe_type, rsp->header.length,
           rsp->next_index);
    free(rsp);
    rsp = NULL;

    /* Err before invalid read */
    st = config_read(dev->pdev, dev->doe_cap_head->cap + PCIE_DOE_STATUS);
    printf("error b4: %x\n", st & PCIE_DOE_STATUS_ERR);

    /* Additional invalid read */
    doe_read_mbox(dev, dev->doe_cap_head->cap);

    /* Err after invalid read */
    st = config_read(dev->pdev, dev->doe_cap_head->cap + PCIE_DOE_STATUS);
    printf("error after: %x\n", st & PCIE_DOE_STATUS_ERR);

    /* Submit request again */
    doe_submit_object(dev, dev->doe_cap_head->cap, &req);
    rsp = doe_get_object(dev, dev->doe_cap_head->cap);
    printf("rsp == NULL ? %s\n", (rsp == NULL) ? "True" : "False");

    doe_abort(dev, dev->doe_cap_head->cap);

    /* Err after abort */
    st = config_read(dev->pdev, dev->doe_cap_head->cap + PCIE_DOE_STATUS);
    printf("error abort: %x\n", st & PCIE_DOE_STATUS_ERR);
}

void test_not_align(pcie_dev *dev)
{
    uint32_t data, addr, size;
    size = 1;
    addr = dev->doe_cap_head->cap + PCIE_DOE_CTRL;
    data = 0x52;

    pwrite(dev->pdev, &data, size, addr);
    data = 0;
    pread(dev->pdev, &data, size, addr);

    printf("[read] addr: %03x, data: %08x\n", addr, data);

    data = 0x53;
    pwrite(dev->pdev, &data, size, addr);
    data = 0;
    pread(dev->pdev, &data, size, addr);

    printf("[read] addr: %03x, data: %08x\n", addr, data);
}
