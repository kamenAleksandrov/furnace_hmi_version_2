/* SPDX-License-Identifier: Apache-2.0 */

/* Program names (Phase 21 M21-7a, WI-H10): the wire format's strict UTF-8 and the code point limit. */

#include <furnace_hmi/controller_library.h>

/* The length of the sequence led by lead, and the smallest code point it may encode; 0 when none. */
static uint32_t sequence_length(uint8_t lead, uint32_t *minimum, uint32_t *bits)
{
    *minimum = 0U;
    *bits = 0U;
    if (lead < 0x80U) {
        *minimum = 0U;
        *bits = lead;
        return 1U;
    }
    if (lead >= 0xC2U && lead < 0xE0U) {
        *minimum = 0x80U;
        *bits = lead & 0x1FU;
        return 2U;
    }
    if (lead >= 0xE0U && lead < 0xF0U) {
        *minimum = 0x800U;
        *bits = lead & 0x0FU;
        return 3U;
    }
    if (lead >= 0xF0U && lead <= 0xF4U) {
        *minimum = 0x10000U;
        *bits = lead & 0x07U;
        return 4U;
    }
    return 0U;
}

bool furnace_hmi_controller_name_valid(const char *bytes, uint32_t length,
                                       uint32_t max_code_points)
{
    const uint8_t *s = (const uint8_t *)bytes;
    uint32_t code_points = 0U;
    uint32_t i = 0U;

    while (i < length) {
        uint32_t minimum;
        uint32_t cp;
        const uint32_t n = sequence_length(s[i], &minimum, &cp);

        if (n == 0U || s[i] == 0U || n > length - i) {
            return false;
        }
        for (uint32_t k = 1U; k < n; k++) {
            if ((s[i + k] & 0xC0U) != 0x80U) {
                return false;
            }
            cp = (cp << 6) | (s[i + k] & 0x3FU);
        }
        if (cp < minimum || cp > 0x10FFFFU || (cp >= 0xD800U && cp <= 0xDFFFU)) {
            return false;
        }
        i += n;
        code_points++;
    }
    return code_points <= max_code_points;
}
