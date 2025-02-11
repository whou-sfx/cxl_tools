/*
 * Copyright (C) 2021 Avery Design Systems, Inc.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the LICENSE file in the top-level directory.
 */

#ifndef PCIE_DOE_H
#define PCIE_DOE_H

// Table 7-x2
#define PCI_DOE_PCI_SIG_VID		 0x0001
#define  PCI_SIG_DOE_DISCOVERY	  0x00
#define  PCI_SIG_DOE_CMA			0x01

#define DATA_OBJ_BUILD_HEADER1(v, p)  ((p << 16) | v)
#define PCI_DOE_PROTOCOL_DISCOVERY  DATA_OBJ_BUILD_HEADER1(PCI_DOE_PCI_SIG_VID, PCI_SIG_DOE_DISCOVERY)
#define PCI_DOE_PROTOCOL_CMA		DATA_OBJ_BUILD_HEADER1(PCI_DOE_PCI_SIG_VID, PCI_SIG_DOE_CMA)

#define PCI_DOE_MAX_DW_SIZE (1 << 18)
#define PCI_DOE_PROTOCOL_MAX 256

/*******************************************************************************
 *
 * DOE Protocol - Data Object
 *
 ******************************************************************************/
typedef struct DOEHeader DOEHeader;
typedef struct doe_discovery doe_discovery;
typedef struct doe_discovery_rsp doe_discovery_rsp;

struct DOEHeader {
	uint16_t vendor_id;
	uint8_t doe_type;
	uint8_t reserved;
	struct {
		uint32_t length :18;
		uint32_t reserved2 :14;
	};
}__attribute__((__packed__));

struct doe_discovery {
	DOEHeader header;
	uint8_t index;
	uint8_t reserved[3];
}__attribute__((__packed__));

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
}__attribute__((__packed__));

#endif /* PCIE_DOE_H */
