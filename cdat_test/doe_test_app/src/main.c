/*
 * Copyright (C) 2021 Avery Design Systems, Inc.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the LICENSE file in the top-level directory.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>

#include "utils.h"
#include "pcie_doe.h"

#include "doe_test.h"
#include "doe_discovery.h"
#include "cxl_cdat.h"
#include "cxl_compliance.h"

#ifndef PROGNAME
#define PROGNAME "test.exe"
#endif

typedef void (*Testcase)(pcie_dev *);

/* The commented test_list is not implemented
 * in Avery BFM CXL Device yet */
static Testcase test_list[] = {
    test_discovery,
    test_cdat,
    /*
    test_error,
    test_invalid_len,
    test_invalid_protocol,
    test_abort,
    test_not_align,
    test_compliance,
    */
};

/* Ref: pciutils/lib/filter.c */
/* Slot filter syntax: [[[domain]:][bus]:][slot][.[func]] */
static char *pci_filter_parse_slot(pcie_dev *f, char *str)
{
    char *colon = strrchr(str, ':');
    char *dot = strchr((colon ? colon + 1 : str), '.');
    char *mid = str;
    char *e, *bus, *colon2;

    if (colon) {
        *colon++ = 0;
        mid = colon;
        colon2 = strchr(str, ':');

        if (colon2) {
            *colon2++ = 0;
            bus = colon2;
            if (str[0] && strcmp(str, "*")) {
                long int x = strtol(str, &e, 16);
                if ((e && *e) || (x < 0 || x > 0x7fffffff)) {
                    return "Invalid domain number";
                }
                f->domain = x;
            }
        } else
            bus = str;

        if (bus[0] && strcmp(bus, "*")) {
            long int x = strtol(bus, &e, 16);
            if ((e && *e) || (x < 0 || x > 0xff)) {
                return "Invalid bus number";
            }
            f->bus = x;
        }
    }

    if (dot) {
        *dot++ = 0;
    }

    if (mid[0] && strcmp(mid, "*")) {
        long int x = strtol(mid, &e, 16);
        if ((e && *e) || (x < 0 || x > 0x1f)) {
            return "Invalid slot number";
        }
        f->slot = x;
    }

    if (dot && dot[0] && strcmp(dot, "*")) {
        long int x = strtol(dot, &e, 16);
        if ((e && *e) || (x < 0 || x > 7)) {
            return "Invalid function number";
        }
        f->func = x;
    }
    return NULL;
}

static void usage(void)
{
    printf("Usage: " PROGNAME " -s [[[[<domain>]:]<bus>]:][<device>][.[<func>]]\n");
}

int main(int argc, char **argv)
{
    pcie_dev dev = {0};
    int i, cmd_opt = 0;
    char filename[41], *err;
    DVSECcap *dvsec;
    bool found = false;

    cmd_opt = getopt(argc, argv, "hs:");

    switch (cmd_opt) {
    case 's':
        err = pci_filter_parse_slot(&dev, optarg);
        if (err) {
            printf("%s\n", err);
            return -1;
        }
        sprintf(filename, "/sys/bus/pci/devices/%04x:%02x:%02x.%01x/config",
                dev.domain, dev.bus, dev.slot, dev.func);
        break;
    case 'h':
        usage();
        return 0;
    default:
        usage();
        return -1;
    }

    dev.pdev = open(filename, O_RDWR);
    if (dev.pdev < 0) {
        printf("Fail to open %s\n", filename);
        return -1;
    }

    init_cap_offset(&dev);

    /* check cap */
    if (!dev.doe_cap_head->cap) {
        printf("DOE not found\n");
        return -1;
    }

    for (dvsec = dev.dvsec_cap_head; dvsec; dvsec = dvsec->next) {
        found |= (dvsec->vendor_id == CXL_VENDOR_ID && dvsec->id == 0x0);
    }
    if (!found) {
        printf("CXL DVSEC #0 not found\n");
        return -1;
    }

    dev.cdev = open("/dev/doe0", O_RDWR | O_SYNC);

    if (dev.cdev < 0) {
        printf("Failed to open /dev/doe0: %s!\n", strerror(errno));
        printf("Try loading DOE driver first.\n");
        return errno;
    }

    /* test */
    for (i = 0; i < ARRAY_SIZE(test_list); i++) {
        test_list[i](&dev);
    }

    close(dev.cdev);
    close(dev.pdev);
    return 0;
}
