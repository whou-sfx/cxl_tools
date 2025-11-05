/*
 * Copyright (C) 2021 Avery Design Systems, Inc.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the LICENSE file in the top-level directory.
 */

#define DEBUG
#include <linux/sched/clock.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <linux/idr.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <uapi/linux/pci_regs.h>
#include <linux/vmalloc.h>

#include "libdoe/pci_regs.h"
#include "libdoe/pcie-doe.h"
#include "doe.h"
#include "doe_api.h"

#define PCI_CLASS_MEMORY_CXL	0x0502
#define CXL_MEMORY_PROGIF 0x10
#define PCIE_EXT_CAP_OFFSET 0x100

struct doe_node {
	struct pcie_doe doe;
	struct doe_node *next;
};

struct doe_dev {
	struct pci_dev *pdev;
	struct cdev cdev;
	struct doe_node *doe_head;
};

static int doe_major;
static struct class *doe_class = NULL;

/* Discovery in kernel space */
static void do_doe_discovery(struct doe_dev *ddev) {
	int ret = -2;
    int idx = 0;
    doe_discovery_rsp response = { 0 };
    do {
        idx = response.next_index;
        doe_discovery request = {
            .header = {
                .vendor_id = PCI_DOE_PCI_SIG_VID,
                .doe_type = PCI_SIG_DOE_DISCOVERY,
                .length = DIV_ROUND_UP(sizeof(request), sizeof(uint32_t)),
            },
            .index = idx,
        };

        ret = pcie_doe_exchange(&ddev->doe_head->doe, (u32 *)&request, sizeof(request),
            (u32 *)&response, sizeof(response));
        dev_info(&ddev->pdev->dev, "ret = %x, len %x, vid %x, type %x, next idx %x\n",
            ret, response.header.length, response.vendor_id, response.doe_type,
            response.next_index);
    } while (response.next_index != 0);
}

static long doe_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	u32 *req_buf, *rsp_buf;
	int req_size = (PCI_DOE_MAX_DW_SIZE + 1) * sizeof(u32),
		rsp_size = PCI_DOE_MAX_DW_SIZE * sizeof(u32);
	struct inode *inode;
	struct doe_dev *doe_dev;
	struct doe_node *doe_node = NULL;
	DOEHeader *doe_hdr;

	switch (cmd) {
	case DOE_MBOX_CMD:
		/* Add one DW for cap_offset. Users should maintain the mappings for the
		 * DOE cap offsets and their protocols */
		req_buf = kmalloc(req_size, GFP_KERNEL);
		if (!req_buf) {
			return -ENOMEM;
		}
		rsp_buf = kmalloc(rsp_size, GFP_KERNEL);
		if (!rsp_buf) {
			kfree(req_buf);
			return -ENOMEM;
		}
		inode = file_inode(file);
		doe_dev = container_of(inode->i_cdev, typeof(*doe_dev), cdev);

		if (copy_from_user(req_buf, (void __user *)arg, req_size)) {
			kfree(req_buf);
			kfree(rsp_buf);
			return -EFAULT;
		}
		dev_info(&doe_dev->pdev->dev, "buf: %08x %08x %08x %08x %08x %08x, req_size=%x\n", 
				req_buf[0], req_buf[1], req_buf[2], req_buf[3], req_buf[4], req_buf[5], req_size);
		/* Find the struct doe_node corresponding to the offset info from user */
		for (doe_node = doe_dev->doe_head; doe_node;
			doe_node = doe_node->next) {
			dev_info(&doe_dev->pdev->dev, "cap_offset=%x\n", doe_node->doe.cap_offset);
			if (doe_node->doe.cap_offset == req_buf[0])
				break;
		}
		if (doe_node == NULL) {
			printk (KERN_NOTICE "can't find the required capability 0x%x", req_buf[0]);
			kfree(req_buf);
			kfree(rsp_buf);
			return -ENOTTY;
		}

		doe_hdr = (DOEHeader *)(req_buf + 1);
		// Add size check if needed
		pcie_doe_exchange(&doe_node->doe, req_buf + 1,
			doe_hdr->length * sizeof(u32), rsp_buf, rsp_size);

		doe_hdr = (DOEHeader *)rsp_buf;
		dev_info(&doe_dev->pdev->dev, "vendor %x, type %x, len %zx\n",  // Change %x to %zx for size_t compatibility
                 doe_hdr->vendor_id, doe_hdr->doe_type, (size_t)doe_hdr->length * sizeof(u32));  // Cast to size_t

        if (copy_to_user((void __user *)arg, rsp_buf, doe_hdr->length * sizeof(u32))) {
            kfree(req_buf);
            kfree(rsp_buf);
            return -EFAULT;  // Handle the error
        }

        kfree(req_buf);
        kfree(rsp_buf);
        return 0;
	default:
		return -ENOTTY;
	}
}

static const struct file_operations doe_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = doe_ioctl,
	.compat_ioctl = compat_ptr_ioctl,
	.llseek = noop_llseek,
};

static int doe_create_cdev(struct doe_dev *doe_dev)
{
	struct cdev *cdev = &doe_dev->cdev;
	int devno, rc = 0;

	cdev_init(cdev, &doe_fops);
	devno = MKDEV(doe_major, 0);  // Assuming minor is 0 as before

	rc = cdev_add(cdev, devno, 1);
	if (rc) {
		goto err_cdev;
	}

	// Use the existing doe_class created in doe_init
	if (doe_class) {
		device_create(doe_class, NULL, devno, NULL, "doe%d", 0);  // Create device under the existing class
	} else {
		printk(KERN_ERR "doe_class is not created!\n");
		rc = -ENODEV;  // Handle error if class isn't initialized
		goto err_device_create;
	}

	return 0;

err_device_create:
	cdev_del(cdev);
err_cdev:
	unregister_chrdev_region(devno, 1);
	return rc;
}

static int doe_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int rc;
	struct doe_dev *ddev = NULL;
	u32 cap_offset, reg_val;
	struct doe_node **dnp;
	bool use_int = false;

	printk("doe_probe\n");
	rc = pcim_enable_device(pdev);
	pci_set_master(pdev);
	if (rc) {
		dev_info(&pdev->dev, "pcim_enable_device failed\n");
		return rc;
	}

	ddev = kzalloc(sizeof(struct doe_dev), GFP_KERNEL);
	if (!ddev) {
		dev_info(&pdev->dev, "kzalloc failed\n");
		return -ENOMEM;
	}
	ddev->pdev = pdev;
	dnp = &ddev->doe_head;

	rc = pci_alloc_irq_vectors(pdev, 1, 4, PCI_IRQ_MSI);
	if (rc > 0) {
		dev_info(&pdev->dev, "allocated %d irqs\n", rc);
		use_int = true;
	} else {
		dev_info(&pdev->dev, "alloc %d irqs failed\n", rc);
		kfree(ddev);
		return -1;
	}

	for (cap_offset = PCIE_EXT_CAP_OFFSET; cap_offset; cap_offset = PCI_EXT_CAP_NEXT(reg_val)) {
		pci_read_config_dword(pdev, cap_offset, &reg_val);

		if (PCI_EXT_CAP_ID(reg_val) == PCI_EXT_CAP_ID_DOE) {
			dev_info(&pdev->dev, "cap = %x\n", cap_offset);
			struct doe_node *new_node = vmalloc(sizeof(struct doe_node));
			if (!new_node) {
				dev_info(&pdev->dev, "vmalloc failed\n");
				// Free previously allocated nodes if needed, but for simplicity, continue
				continue;  // Or handle error more robustly
			}
			*dnp = new_node;
			pcie_doe_init(&(*dnp)->doe, pdev, cap_offset, use_int);
			dnp = &(*dnp)->next;
		}
	}
	*dnp = NULL;  // Ensure the list ends properly

	dev_set_drvdata(&pdev->dev, ddev);
	rc = doe_create_cdev(ddev);  // Check return value
	if (rc) {
		dev_info(&pdev->dev, "doe_create_cdev failed\n");
		// Add cleanup code here
	}

	if (ddev->doe_head) {  // Check before calling
		do_doe_discovery(ddev);
	} else {
		dev_info(&pdev->dev, "No DOE capabilities found\n");
	}

	return 0;
}

static void doe_remove(struct pci_dev *pdev)
{
	struct doe_dev *ddev;
	struct doe_node *current_node, *next_node;

	ddev = dev_get_drvdata(&pdev->dev);
	if (ddev) {
		current_node = ddev->doe_head;
		while (current_node) {
			next_node = current_node->next;
			pcie_doe_fini(pdev);  // This may depend on IRQs being freed first
			vfree(current_node);
			current_node = next_node;
		}

		if (ddev->cdev.dev) {
			device_destroy(doe_class, MKDEV(doe_major, 0));  // As per previous fixes
		}

		kfree(ddev);
        //pci_set_drvdata(&pdev->dev, NULL);
	}

	// Reset DOE control register*/
	pci_write_config_dword(pdev, ddev->doe_head->doe.cap_offset + PCI_DOE_CTRL, 0);
}

static const struct pci_device_id doe_pci_tbl[] = {
	/* PCI class code for CXL.mem Type-3 Devices */
	{ PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID,
	  PCI_CLASS_MEMORY_CXL, 0xffffff, 0 },
	{ PCI_DEVICE_CLASS((PCI_CLASS_MEMORY_CXL << 8 | CXL_MEMORY_PROGIF), ~0)},
	{ /* terminate list */ },
};
MODULE_DEVICE_TABLE(pci, doe_pci_tbl);

static struct pci_driver doe_driver = {
	.name			= KBUILD_MODNAME,
	.id_table		= doe_pci_tbl,
	.probe			= doe_probe,
	.remove			= doe_remove,
};

static __init int doe_init(void)
{
	int rc;
	dev_t devt;

	printk("doe_init\n");
	rc = alloc_chrdev_region(&devt, 0, 1, "doe");
	if (rc) {
		printk("alloc_chrdev_region failed, rc=%x\n", rc);
		return rc;
	}

	doe_major = MAJOR(devt);

	// Create the class here, once during module initialization
	doe_class = class_create("doe");
	if (IS_ERR(doe_class)) {
		printk("class_create failed: %ld\n", PTR_ERR(doe_class));
		rc = PTR_ERR(doe_class);
		goto err_class_create;
	}

	rc = pci_register_driver(&doe_driver);
	if (rc) {
		printk("pci_register_driver failed, rc=%x\n", rc);
		goto err_driver;
	}

	return 0;

err_class_create:
	class_destroy(doe_class);  // Cleanup if class creation fails
err_driver:
	unregister_chrdev_region(MKDEV(doe_major, 0), 1);
	return rc;
}

static __exit void doe_exit(void)
{
    // Unregister the driver first to clean up devices via doe_remove
    pci_unregister_driver(&doe_driver);

    // Now destroy the class after devices are handled
    if (doe_class) {
        class_destroy(doe_class);
    }

    unregister_chrdev_region(MKDEV(doe_major, 0), 1);
}

MODULE_LICENSE("GPL v2");
module_init(doe_init);
module_exit(doe_exit);
