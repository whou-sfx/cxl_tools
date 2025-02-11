#ifndef PCI_REGS_H
#define PCI_REGS_H

#define PCI_EXT_CAP_ID_DOE	0x2E	/* Data Object Exchange */
#undef PCI_EXT_CAP_ID_MAX
#define PCI_EXT_CAP_ID_MAX	PCI_EXT_CAP_ID_DOE

/* Data Object Exchange */
#define PCI_DOE_CAP		0x04	/* DOE Capabilities Register */
#define  PCI_DOE_CAP_INT			0x00000001  /* Interrupt Support */
#define  PCI_DOE_CAP_IRQ			0x00000ffe  /* Interrupt Message Number */
#define PCI_DOE_CTRL		0x08	/* DOE Control Register */
#define  PCI_DOE_CTRL_ABORT			0x00000001  /* DOE Abort */
#define  PCI_DOE_CTRL_INT_EN			0x00000002  /* DOE Interrupt Enable */
#define  PCI_DOE_CTRL_GO			0x80000000  /* DOE Go */
#define PCI_DOE_STATUS		0x0C	/* DOE Status Register */
#define  PCI_DOE_STATUS_BUSY			0x00000001  /* DOE Busy */
#define  PCI_DOE_STATUS_INT_STATUS		0x00000002  /* DOE Interrupt Status */
#define  PCI_DOE_STATUS_ERROR			0x00000004  /* DOE Error */
#define  PCI_DOE_STATUS_DATA_OBJECT_READY	0x80000000  /* Data Object Ready */
#define PCI_DOE_WRITE		0x10	/* DOE Write Data Mailbox Register */
#define PCI_DOE_READ		0x14	/* DOE Read Data Mailbox Register */

/* DOE Data Object - note not actually registers */
#define PCI_DOE_DATA_OBJECT_HEADER_1_VID	0x0000FFFF
#define PCI_DOE_DATA_OBJECT_HEADER_1_TYPE	0x00FF0000
#define PCI_DOE_DATA_OBJECT_HEADER_2_LENGTH	0x0003FFFF

#define PCI_DOE_DATA_OBJECT_DISC_REQ_3_INDEX	0x000000FF
#define PCI_DOE_DATA_OBJECT_DISC_RSP_3_VID	0x0000FFFF
#define PCI_DOE_DATA_OBJECT_DISC_RSP_3_PROTOCOL	0x00FF0000
#define PCI_DOE_DATA_OBJECT_DISC_RSP_3_NEXT_INDEX 0xFF000000

#endif /* PCI_REGS_H */
