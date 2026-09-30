# Roadmap

## Operator workflow redesign (2026-09-23)

**Fact:** [Plan](OPERATOR_WORKFLOW_IMPLEMENTATION_PLAN.md) and
[concepts](../03-hmi/OPERATOR_WORKFLOW_SCREEN_CONCEPTS.md) are implemented as
paired browser-demo/native-LVGL presentation work. Current refinement is the
70/30 top-graph/bottom-control Manual forecast, lower-band state/elapsed,
left-side run controls, right-side labelled target/rate controls and native
observed elapsed presentation. Manual graph/control/run-action updates now
preserve the displayed graph in place and use the supplied full temperature
range; browser keyboard entry retains focus across simulated ticks; production controller/protocol reconciliation
and firmware reminders remain in the [controller backlog](CONTROLLER_INTEGRATION_BACKLOG.md).

## Client web demo planning (2026-09-18)

**Fact:** The [agent build plan](WEB_DEMO_CLIENT_BUILD_PLAN.md) records the source inspection and ordered P0-P7 implementation packets. The user confirmed 1:1 HMI fidelity with simulated controller responses. Implementation is not started. Delivery is confirmed as session-only programs with premade examples, an ordinary online webpage and English only.

## Completed presentation refinement

The [running readings refinement](RUNNING_READINGS_REFINEMENT_PLAN.md) now
shows curing elapsed and remaining together on one non-interactive line;
cooling elapsed and Est. remaining stay separate. The former time toggle is
removed, with unknown remaining kept explicit. The single-line readings,
digits-plus-letters keyboard and 48-pixel full-width light target are checked
in both browser and native UI. Native builds and 10/10 tests pass at 800x480,
800x640 and 1024x600; browser smoke passes. Host/browser only.

The foundation follows this ordered gate. A later phase may be prepared in
parallel, but it cannot claim completion before its prerequisites and evidence
exist.

| Phase | Outcome | Status on 2026-09-09 |
| --- | --- | --- |
| 1. Repository normalization | Inspect Git, normalize the Obsidian vault, add governance, establish documentation taxonomy | Complete |
| 2. Authoritative documentation | Encode requirements/invariants, V1 lessons, ADRs, questions, roadmap, and status | Complete foundation baseline |
| 3. Agent infrastructure | Add three narrow skills, six read-focused reviewers, `AGENTS.md`, and `PLANS.md`; validate non-overlap | Complete |
| 4. Zephyr 4.4.0 environment | Prepare isolated environment, document tooling, configure VS Code, verify | Complete for Windows `qemu_x86`; production board remains open |
| 5. Minimal Zephyr proof | Build a foundation-only Zephyr target and generate a compilation database | Complete |
| 6. Desktop LVGL foundation | Build and run a project-owned LVGL/SDL2 window and verify input/shareable/debug boundaries | Complete, including an MSVC/PDB `cppvsdbg` source stop |
| 7. Testing foundation | Prove portable host and Zephyr test paths and document exact commands | Complete |
| 8. Indexer V1 | Generate deterministic JSON and Markdown from repository/build metadata | Complete |
| 9. Foundation review | Run required broad reviewers and fix confirmed issues | Complete; no remaining blocker |
| 10. Foundation gate | Record files, environment changes, commands, results, risks, and questions, then stop | Complete; stopped before production work |
| 11. First UI vertical slice | Add a reusable fail-closed state-availability view, keep diagnostics separate, verify it visually/headlessly, and provide a one-command desktop operator workflow | Complete; no furnace semantics or actions introduced |
| 12. Host-only UI laboratory | Add deterministic semantic controller scenarios and an expanded no-scroll UI explorer that exercises shared model/presentation code without entering Zephyr | Graphite instrument-panel implementation is complete across all current routes. Default 800 x 480, 800 x 640, and 1024 x 600 viewport suites pass 10/10 with 165 actual native captures inspected and zero geometry errors; manual operator review and later controller-backed behavior remain pending |
| 13. Settings host-only presentation | Translate the approved Settings concept into the native LVGL laboratory with local edit feedback and bounded Maintenance previews | Complete for the host-only lab at 800 x 480: Settings routes and smoke/capture checks pass; controller-backed preferences, storage, clock, connections, deletion, and restart behavior remain gated |
| 14. Device host-only presentation | Translate the Device concept into native LVGL with generic catalogue-backed components/tasks, filters, details, notifications, history, info, and stale/unavailable states | Complete for the host-only lab: editable startup catalogue, native smoke, and zero-error geometry captures pass at 800 x 480, 800 x 640, and 1024 x 600; controller catalogue/counter integration remains gated |
| 15. Programs host-only styling | Translate the approved Programs style proposal into native LVGL while preserving library, detail, stage, graph-node, editor and Load-preview behavior | Complete for the host-only lab: native build, full smoke/geometry suites and zero-error captures pass at 800 x 480, 800 x 640, and 1024 x 600; operator and physical-panel review remain separate |
| 16. Touch keyboard host-only presentation | Translate the approved browser text/numeric keyboard layouts into native LVGL and route names, temperatures, rates and times through bounded local edit sessions | Complete for the host-only lab: native smoke covers text/page/Shift behavior, program and stage names, Settings rate entry and stage numeric routing; embedded keyboard integration and physical touch remain gated |

## Next delivery gate

**Touch keyboard review:** The shared browser demo and host-only LVGL lab
include full-text and numeric entry layouts; [the plan](TOUCH_KEYBOARD_PLAN.md)
records field-aware keys, transaction semantics, native evidence and the
remaining embedded/physical-touch gate.

**2026-09-15 browser demo extension:** Home/Load/Running and stage editing are
available in the shared HTML; review the [demo plan](HOME_RUNNING_DEMO_PLAN.md).
Public website deployment and wider demo functionality remain later work.

**Programs operator review:** The [style proposal](../03-hmi/PROGRAMS_STYLE_PROPOSAL.md)
and [native styling plan](PROGRAMS_STYLE_PLAN.md) are implemented in the
host-only LVGL lab. Review the native presentation before physical-panel or
controller-backed integration; no domain, architecture, or protocol change is
proposed.

**Device production contract gate:** Review the [Device proposal](../03-hmi/DEVICE_SCREEN_PROPOSAL.md),
[implementation plan](DEVICE_IMPLEMENTATION_PLAN.md), and [proposed ADR-0011](../07-decisions/0011-component-service-catalogue.md).
The host-only native presentation is complete, but component catalogue ownership,
counter definitions, actual manufacturer references, controller service-state
transport, and service-history contracts remain proposed before production integration.

**2026-09-14 update:** The approved [Settings proposal](../03-hmi/SETTINGS_SCREEN_PROPOSAL.md)
has a native host-only LVGL presentation recorded in the [implementation plan](SETTINGS_IMPLEMENTATION_PLAN.md).
Production action contracts, persistence, clock, connectivity, and hardware
integration remain pending separate gates.

The [Graphite UI completion plan](GRAPHITE_UI_COMPLETION_PLAN.md) now has
implementation and rendered evidence for every current route/dialog at three
native sizes. See the [verification record](GRAPHITE_UI_VERIFICATION.md).
The next product gate is operator review and eventual physical-panel testing;
production command/control semantics remain a separate future slice.

The current step is the host-only UI laboratory plan in
[UI_LAB_VERTICAL_SLICE_PLAN.md](UI_LAB_VERTICAL_SLICE_PLAN.md). Its graphite
visual-system implementation is ready for manual operator review across the
supported host viewports. Production hardware, transport, wire format, and the
full question register remain deferred until their gates need them.

Any later command/control slice requires explicit domain semantics and review
under [PLANS.md](../../PLANS.md), while preserving the
[authority decision](../07-decisions/0001-authority-boundary.md).

## 2026-09-15 - HMI web demo publishing

**Fact:** Renamed the shared browser source to `hmi-web-demo.html` and added
a repeatable client adaptation to the Telamorph website build. Build and
headless browser interaction checks passed. See the
[web demo record](HMI_WEB_DEMO.md) for workflow, deployment result and limits.

**Deployment:** Attempted; blocked by expired Firebase login. Local preview
is available; live publishing requires `firebase.cmd login --reauth`.
