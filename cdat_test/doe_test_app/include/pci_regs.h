/*
 * Copyright (C) 2021 Avery Design Systems, Inc.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the LICENSE file in the top-level directory.
 */

/* Supplement to standard header linux/pci_regs.h */
#ifndef PCI_REGS_H
#define PCI_REGS_H

#define PCI_CAP_ID(header)      (header & 0x000000ff)
#define PCI_CAP_NEXT(header)    ((header >> 8) & 0xff)

#define PCI_EXT_CAP_ID_DVSEC    0x23    /* Designated Vendor-Specific */
#define PCI_EXT_CAP_ID_DOE      0x2e    /* Data Object Exchange */

/* DOE Capabilities Register */
#define PCIE_DOE_CAP            0x04
#define  PCIE_DOE_CAP_INTR_SUPP 0x00000001
/* DOE Control Register  */
#define PCIE_DOE_CTRL           0x08
#define  PCIE_DOE_CTRL_ABORT    0x00000001
#define  PCIE_DOE_CTRL_INTR_EN  0x00000002
#define  PCIE_DOE_CTRL_GO       0x80000000
/* DOE Status Register  */
#define PCIE_DOE_STATUS         0x0c
#define  PCIE_DOE_STATUS_BUSY   0x00000001
#define  PCIE_DOE_STATUS_INTR   0x00000002
#define  PCIE_DOE_STATUS_ERR    0x00000004
#define  PCIE_DOE_STATUS_DO_RDY 0x80000000
/* DOE Write Data Mailbox Register  */
#define PCIE_DOE_WR_DATA_MBOX   0x10
/* DOE Read Data Mailbox Register  */
#define PCIE_DOE_RD_DATA_MBOX   0x14

/* PCIe Designated Vendor-Specific Capability */
#ifndef PCI_DVSEC_HEADER1
#define PCI_DVSEC_HEADER1       4    /* Designated Vendor-Specific Header 1 */
#endif
#ifndef PCI_DVSEC_HEADER2
#define PCI_DVSEC_HEADER2       8    /* Designated Vendor-Specific Header 2 */
#endif

/* DVSEC */
#define PCI_EXT_DVSEC_VEN_ID(header1)   (header1 & 0x0000ffff)
#define PCI_EXT_DVSEC_REV(header1)      ((header1 >> 16) & 0xf)
#define PCI_EXT_DVSEC_LEN(header1)      ((header1 >> 20) & 0xffc)
#define PCI_EXT_DVSEC_ID(header2)       (header2 & 0x0000ffff)

/* PCIe CXL Designated Vendor-Specific Capabilities, Control, Status */
#define PCI_CXL_CAP             0x0a    /* CXL Capability Register */
#define  PCI_CXL_CAP_CACHE      0x0001  /* CXL.cache Protocol Support */
#define  PCI_CXL_CAP_IO         0x0002  /* CXL.io Protocol Support */
#define  PCI_CXL_CAP_MEM        0x0004  /* CXL.mem Protocol Support */
#define  PCI_CXL_CAP_MEM_HWINIT 0x0008  /* CXL.mem Initalizes with HW/FW Support */
#define  PCI_CXL_CAP_HDM_CNT(x) (((x) & (3 << 4)) >> 4)    /* CXL Number of HDM ranges */
#define  PCI_CXL_CAP_VIRAL      0x4000  /* CXL Viral Handling Support */
#define PCI_CXL_CTRL            0x0c    /* CXL Control Register */
#define  PCI_CXL_CTRL_CACHE     0x0001  /* CXL.cache Protocol Enable */
#define  PCI_CXL_CTRL_IO        0x0002  /* CXL.io Protocol Enable */
#define  PCI_CXL_CTRL_MEM       0x0004  /* CXL.mem Protocol Enable */
#define  PCI_CXL_CTRL_CACHE_SF_COV(x)   (((x) & (0x1f << 3)) >> 3) /* Snoop Filter Coverage */
#define  PCI_CXL_CTRL_CACHE_SF_GRAN(x)  (((x) & (0x7 << 8)) >> 8) /* Snoop Filter Granularity */
#define  PCI_CXL_CTRL_CACHE_CLN 0x0800  /* CXL.cache Performance Hint on Clean Evictions */
#define  PCI_CXL_CTRL_VIRAL     0x4000  /* CXL Viral Handling Enable */
#define PCI_CXL_STATUS          0x0e    /* CXL Status Register */
#define  PCI_CXL_STATUS_VIRAL   0x4000  /* CXL Viral Handling Status */
#define PCI_CXL_CTRL2           0x10    /* CXL Control2 Register */
#define PCI_CXL_STATUS2         0x12    /* CXL Status2 Register */
#define PCI_CXL_LOCK            0x14    /* CXL Lock Register */
#define PCI_CXL_CAP2            0x16    /* CXL Capability2 Register */
#define PCI_CXL_RNG1_SIZE_HI    0x18    /* CXL Range 1 Size High Register */
#define PCI_CXL_RNG1_SIZE_LO    0x1c    /* CXL Range 1 Size Low Register */
#define  PCI_CXL_RNG1_SIZE_LO_MEM_VALID 0x0001    /* CXL Memory Info Valid */
#define  PCI_CXL_RNG1_SIZE_LO_MEM_ACT   0x0002    /* CXL Memory Active */
#define  PCI_CXL_RNG1_SIZE_LO_MED_TYPE(x)   (((x) & (0x7 << 2)) >> 2)   /* CXL Media Type */
#define  PCI_CXL_RNG1_SIZE_LO_MEM_CLASS(x)  (((x) & (0x7 << 5)) >> 5)   /* CXL Memory Class */
#define  PCI_CXL_RNG1_SIZE_LO_INTERLEAVE(x) (((x) & (0x1f << 8)) >> 8)  /* CXL Desired Interleave */
#define  PCI_CXL_RNG1_SIZE_LO_MEM_ACT_TO(x) (((x) & (0x7 << 13)) >> 13) /* CXL Memory Active Timeout */
#define  PCI_CXL_RNG1_SIZE_LO_SIZE(x)       ((x) & (0xf << 28))         /* CXL Memory Size Low */
#define PCI_CXL_RNG1_BASE_HI    0x20    /* CXL Range 1 Base High Register */
#define PCI_CXL_RNG1_BASE_LO    0x24    /* CXL Range 1 Base Low Register */
#define  PCI_CXL_RNG1_BASE_LO_BASE(x)   ((x) & (0xf << 28))    /* CXL Memory Base Low */
#define PCI_CXL_RNG2_SIZE_HI    0x28    /* CXL Range 2 Size High Register */
#define PCI_CXL_RNG2_SIZE_LO    0x2c    /* CXL Range 2 Size Low Register */
#define  PCI_CXL_RNG2_SIZE_LO_MEM_VALID  0x0001    /* CXL Memory Info Valid */
#define  PCI_CXL_RNG2_SIZE_LO_MEM_ACT    0x0002    /* CXL Memory Active */
#define  PCI_CXL_RNG2_SIZE_LO_MED_TYPE(x)   (((x) & (0x7 << 2)) >> 2)   /* CXL Media Type */
#define  PCI_CXL_RNG2_SIZE_LO_MEM_CLASS(x)  (((x) & (0x7 << 5)) >> 5)   /* CXL Memory Class */
#define  PCI_CXL_RNG2_SIZE_LO_INTERLEAVE(x) (((x) & (0x1f << 8)) >> 8)  /* CXL Desired Interleave */
#define  PCI_CXL_RNG2_SIZE_LO_MEM_ACT_TO(x) (((x) & (0x7 << 13)) >> 13) /* CXL Memory Active Timeout */
#define  PCI_CXL_RNG2_SIZE_LO_SIZE(x)       ((x) & (0xf << 28))         /* CXL Memory Size Low */
#define PCI_CXL_RNG2_BASE_HI    0x30    /* CXL Range 2 Base High Register */
#define PCI_CXL_RNG2_BASE_LO    0x34    /* CXL Range 2 Base Low Register */
#define  PCI_CXL_RNG2_BASE_LO_BASE(x)       ((x) & (0xf << 28))         /* CXL Memory Base Low */

#endif /* PCI_REGS_H */
