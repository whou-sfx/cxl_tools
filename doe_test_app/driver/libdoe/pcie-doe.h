/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Data Object Exchange was added to the PCI spec as an ECN to 5.0.
 *
 * Copyright (C) 2021 Huawei
 *     Jonathan Cameron <Jonathan.Cameron@huawei.com>
 */

#include <linux/completion.h>
#include <linux/mutex.h>

#ifndef LINUX_PCIE_DOE_H
#define LINUX_PCIE_DOE_H
/**
 * struct pcie_doe - State to support use of DOE mailbox
 * @lock: Ensure users of the mailbox are serialized
 * @cap_offset: Config space offset to base of DOE capability.
 * @pdev: PCI device that hosts this DOE.
 * @c: Completion used for interrupt handling.
 * @use_int: Flage to indicate if interrupts rather than polling used.
 */
struct pcie_doe {
	struct mutex lock;
	int cap_offset;
	struct pci_dev *pdev;
	struct completion c;
	bool use_int;
};

void pcie_doe_fini(struct pci_dev *pdev);
int pcie_doe_init(struct pcie_doe *doe, struct pci_dev *dev, int doe_offset,
		  bool use_int);
int pcie_doe_exchange(struct pcie_doe *doe, u32 *request, size_t request_sz,
		      u32 *response, size_t response_sz);
int pcie_doe_protocol_check(struct pcie_doe *doe, u16 vid, u8 protocol);
#endif
