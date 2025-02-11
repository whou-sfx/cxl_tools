/*
 * Copyright (C) 2021 Avery Design Systems, Inc.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the LICENSE file in the top-level directory.
 */

#ifndef CXL_H
#define CXL_H

#define CXL_VENDOR_ID 0x1e98

/* CXL DVSEC ID */
#define PCIE_DVSEC_CXL_DEV          0x0
#define PCIE_DVSEC_NON_CXL_FUNC_MAP 0x2
#define PCIE_DVSEC_EXT_PORT         0x3
#define PCIE_DVSEC_GPF_PORT         0x4
#define PCIE_DVSEC_GPF_DEV          0x5
#define PCIE_DVSEC_FLEX_BUS_PORT    0x7
#define PCIE_DVSEC_REG_LOC          0x8
#define PCIE_DVSEC_MLD              0x9
#define PCIE_DVSEC_TEST_CAP         0xA

#endif /* CXL_H */
