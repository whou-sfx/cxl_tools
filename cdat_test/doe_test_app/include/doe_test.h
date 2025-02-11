/*
 * Copyright (C) 2021 Avery Design Systems, Inc.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the LICENSE file in the top-level directory.
 */

#ifndef DOE_TEST_H
#define DOE_TEST_H

void test_invalid_len(pcie_dev *dev);
void test_invalid_protocol(pcie_dev *dev);
void test_abort(pcie_dev *dev);
void test_error(pcie_dev *dev);
void test_not_align(pcie_dev *dev);
#endif /* DOE_TEST_H */
