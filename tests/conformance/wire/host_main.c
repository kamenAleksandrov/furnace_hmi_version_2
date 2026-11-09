/* SPDX-License-Identifier: Apache-2.0 */

/* The conformance checks as a CTest program: exit status 0 only when all pass. */

#include <stdio.h>

#include "wire_conformance.h"

void wire_conformance_report(const char *check, const char *name)
{
    printf("FAIL %s: %s\n", name, check);
}

int main(void)
{
    const int failures = wire_conformance_run();

    printf("wire conformance: %d failure(s)\n", failures);
    return (failures == 0) ? 0 : 1;
}
