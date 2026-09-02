/* SPDX-License-Identifier: Apache-2.0 */

#include <stdint.h>
#include <stdio.h>

#include <furnace_hmi/foundation.h>

int main(void)
{
    const uint32_t actual = furnace_hmi_foundation_probe_version();

    if (actual != FURNACE_HMI_FOUNDATION_PROBE_VERSION) {
        (void)fprintf(stderr, "foundation build probe mismatch: %lu\n", (unsigned long)actual);
        return 1;
    }

    return 0;
}
