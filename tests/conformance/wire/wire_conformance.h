/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_WIRE_CONFORMANCE_H
#define FURNACE_HMI_WIRE_CONFORMANCE_H

/* Run every check; returns the number of failures, each reported once. */
int wire_conformance_run(void);

/* Supplied by the runner (host program or Ztest suite): one failed check. */
void wire_conformance_report(const char *check, const char *name);

#endif /* FURNACE_HMI_WIRE_CONFORMANCE_H */
