/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_CONTROLLER_CLIENT_TESTS_H
#define FURNACE_HMI_CONTROLLER_CLIENT_TESTS_H

/* Run every scenario; returns the number of failures, each reported once. */
int controller_client_tests_run(void);

/* The observation mapped to the operator screens' model (controller_dashboard_tests.c). */
int controller_dashboard_tests_run(void);

/* Supplied by the runner (host program or Ztest suite): one failed check. */
void controller_client_tests_report(const char *check, const char *scenario);

#endif /* FURNACE_HMI_CONTROLLER_CLIENT_TESTS_H */
