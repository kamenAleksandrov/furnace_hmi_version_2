/* SPDX-License-Identifier: Apache-2.0 */

/* The controller client scenarios as a CTest program: exit status 0 only when all pass. */

#include <stdio.h>

#include "controller_client_tests.h"

void controller_client_tests_report(const char *check, const char *scenario)
{
    printf("FAIL %s: %s\n", scenario, check);
}

int main(void)
{
    const int failures = controller_client_tests_run();

    printf("controller client: %d failure(s)\n", failures);
    return (failures == 0) ? 0 : 1;
}
