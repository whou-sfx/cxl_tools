/*
 * Copyright (C) 2021 Avery Design Systems, Inc.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the LICENSE file in the top-level directory.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

#include "pcie.h"
#include "pcie_doe.h"
#include "driver/doe_api.h"

int doe_get_cap_by_prot(pcie_dev *dev, uint32_t prot)
{
    DOEcap *cap;
    DOEprot *p;

    for (cap = dev->doe_cap_head; cap; cap = cap->next) {
        for (p = cap->prot_head; p; p = p->next) {
            if (p->prot == prot) {
                return cap->cap;
            }
        }
    }

    /* fail */
    return 0;
}

void doe_exchange_object(pcie_dev *dev, uint32_t doe_cap, void *buf)
{
    *(uint32_t *)buf = doe_cap;
    ioctl(dev->cdev, DOE_MBOX_CMD, buf);
}

void doe_submit_object(pcie_dev *dev, uint32_t doe_cap, void *obj)
{
    uint32_t len = ((DOEHeader *)obj)->length;
    __doe_submit_object(dev, doe_cap, obj, len);
}

void __doe_submit_object(pcie_dev *dev, uint32_t doe_cap, void *obj, uint32_t len)
{
    uint32_t go = 0x80;
    int i;

    for (i = 0; i < (int)len; i++) {
        config_write(dev->pdev, doe_cap + PCIE_DOE_WR_DATA_MBOX,
                     *(uint32_t *)(obj + i * sizeof(uint32_t)));
    }

    /* Set GO */
    /*
    config_write(dev->pdev, dev->doe_cap + PCIE_DOE_CTRL,
                 PCIE_DOE_CTRL_GO);
    */
    pwrite(dev->pdev, &go, sizeof(uint32_t), doe_cap + PCIE_DOE_CTRL + 3);
}

void *doe_get_object(pcie_dev *dev, uint32_t doe_cap)
{
    uint32_t buf[PCI_DOE_MAX_DW_SIZE] = {0};
    uint32_t len, rd_cnt = 0;
    void *obj;

    /* Read Discovery response */
    while (doe_check_ready(dev, doe_cap)) {
        buf[rd_cnt++] = doe_read_mbox(dev, doe_cap);
    }

    if (rd_cnt * sizeof(uint32_t) < sizeof(DOEHeader)) {
        printf("mbox buffer size smaller than DOEHeader size 0x%0lx\n",
               sizeof(DOEHeader));
        return NULL;
    }

    len = ((DOEHeader *)buf)->length;

    if (len != rd_cnt) {
        printf("len 0x%0x does not match number of reads 0x%0x\n", len, rd_cnt);
        return NULL;
    }

    obj = malloc(len * sizeof(uint32_t));

    memcpy(obj, buf, len * sizeof(uint32_t));

    return obj;
}

uint32_t doe_read_mbox(pcie_dev *dev, uint32_t doe_cap)
{
    uint32_t data = 0;

    data = config_read(dev->pdev, doe_cap + PCIE_DOE_RD_DATA_MBOX);

    /* Write Discovery response success */
    config_write(dev->pdev, doe_cap + PCIE_DOE_RD_DATA_MBOX,
                 0x1/* arbitrary value */);

    return data;
}

void doe_abort(pcie_dev *dev, uint32_t doe_cap)
{
    /* Abort */
    config_write(dev->pdev, doe_cap + PCIE_DOE_CTRL,
                 PCIE_DOE_CTRL_ABORT);
}

void doe_wait(pcie_dev *dev, uint32_t doe_cap)
{
    /* Polling */
    while (!doe_check_ready(dev, doe_cap)) {
        printf("wait for 1 sec\n");
        sleep(1);
    }
}

bool doe_check_ready(pcie_dev *dev, uint32_t doe_cap)
{
    /* check status READY is set */
    return PCIE_DOE_STATUS_DO_RDY &
           config_read(dev->pdev, doe_cap + PCIE_DOE_STATUS);
}
