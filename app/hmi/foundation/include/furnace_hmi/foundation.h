/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_FOUNDATION_H
#define FURNACE_HMI_FOUNDATION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FURNACE_HMI_FOUNDATION_PROBE_VERSION UINT32_C(1)

/**
 * Return the version of the portable build probe.
 *
 * This deliberately contains no furnace behavior. It is a compile-and-link
 * probe shared by the host and Zephyr foundations until real portable modules
 * are introduced behind reviewed interfaces. It is not an ABI compatibility
 * promise for separately deployed binaries.
 */
uint32_t furnace_hmi_foundation_probe_version(void);

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_FOUNDATION_H */
