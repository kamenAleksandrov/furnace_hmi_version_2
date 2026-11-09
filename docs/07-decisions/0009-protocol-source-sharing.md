# ADR-0009: Select protocol source-sharing model

- Status: Accepted (product owner, 2026-11-10); reviews recorded below
- Date: 2026-08-24; decided 2026-11-10
- Owners: product owner; the controller repository owns the copied code
- Governing requirements: PROTO-004, PROTO-005, TEST-004

## Context

The HMI and Control CPU need compatible protocol semantics and wire behavior,
but may use different repositories, toolchains, release cadences, and resource
constraints. Sharing all C source can reduce duplication yet increase coupling;
independent implementations improve diversity but risk semantic drift. In
2026-11 the controller has a codec proven by golden vectors (ADR-0007).

## Options considered

- **A. A pinned copy of the controller's codec**, checked against its source.
- **B. An independent HMI codec**, proven by the same golden vectors.
- **C. The controller repository as a build dependency** of the HMI.

## Decision

**A, the product owner's choice:**

- the `.c` and `.h` files of the controller's `core/wire` and `core/crc` are
  copied into `app/hmi/protocol/wire` at one named controller revision, the
  same revision whose wire-format document ADR-0007 follows; the HMI supplies
  its own build files, since the controller's link Zephyr-specific targets;
- the copied files are not edited in the HMI and are excluded from the HMI's
  formatting and lint ownership; they keep their SPDX headers (both
  repositories are Apache-2.0), and a manifest records the source repository,
  revision, and a hash of every file;
- a check fails when the copy differs from the manifest, and, given a
  controller checkout, when the copy differs from that revision;
- the controller's golden vectors are copied with it and run in the HMI's own
  tests, in the HMI's desktop MSVC build, its host GCC build and its Zephyr
  builds;
- a change to the codec, including a fix for an MSVC or GCC warning, is made in
  the controller repository first and synced into the HMI as one change.

**Why:** one implementation already proven by the controller's vectors, with
little code to maintain. **Cost:** a defect in the codec affects both sides at
once; the golden vectors are the guard, and independence is not claimed. The
portability of the copied C17 code to MSVC is unproven until Phase 21's M21-1;
if it fails, the fix goes to the controller.

## Consequences

- No codec structure, error type or native layout becomes the HMI's model
  (ADR-0003): the client converts to plain HMI data.
- Release pinning and cross-repository review follow the sync: each sync is a
  reviewed HMI change naming the controller revision.

## Historical: the 2026-08-24 deferral

The deferral listed a shared package, generated artifacts, independent codecs
with shared vectors, and hybrids, pending repository and release constraints.
Those constraints are now known (Phase 21 plan) and answered above.

## Review record

2026-11-10, with the Phase 21 plan (AI reviewers acting in the HMI's review
roles; the product owner decided):

- **Protocol and architect review:** the copy fits the `app/hmi/<module>`
  layout; findings applied: copy only `.c` and `.h` with HMI build files (the
  controller's CMake links Zephyr); prove the codec under the desktop MSVC
  build; exclude it from HMI lint ownership; keep the headers; licensing
  compatible.
- **Decision challenger:** findings applied: one controller revision for the
  documents and the copy; the compilers covered; the upstream-first path for
  warnings.
- **Unresolved:** none.
