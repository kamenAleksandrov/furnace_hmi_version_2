# Device screen proposal

Status: **Proposal for operator review**, 2026-09-14. No production controller
behavior or manufacturer service interval is established by this document.

## Intent, evidence, and ownership

**User mandate:** Extend the existing browser concept with Device. Show total
program runtime, total powered-on time, fan runtime, component service
intervals, maintenance notifications, errors, and useful worker information.
Components must be easy for the developer to add in code; workers see only
configured components and cannot use this page to damage or misconfigure the
device.

**Facts:** Controller ownership of counters, time, faults, service conditions,
and service acknowledgement records is established by REQ-FUN-001, 010, 011,
016; ARCH-INV-001, 002, 004, 007; and PROTO-008, 010 in the
[requirements](../00-project/REQUIREMENTS.md). REQ-NFR-004, 006, 009 and
[ADR-0005](../07-decisions/0005-lvgl-single-owner.md) constrain responsiveness,
validity, portability, and LVGL ownership.
[ADR-0001](../07-decisions/0001-authority-boundary.md) preserves controller
authority. The current native Device lab shows a furnace visual, readings,
and service placeholder; a per-component service catalogue is not implemented.
The current Settings lab has since gained native implementation; this task
extends the browser concept and planning documents only.

**Recommendation:** Device is an inspection workspace. Its initial actions are
View details, filter, page, and read approved guidance/history. It has no
interval editing, counter reset, fault clear, actuator test, calibration,
service-complete action, or hidden administrative unlock. Settings Maintenance
remains the separate home of its already proposed reset/delete/restart flows.
Read-only UI alone is not a safety mechanism: the controller must still enforce
permissions and protections against all clients.

**Assumption to confirm:** Workers need to identify work and refer it to service
personnel, not certify repair completion. Recording service completion later
needs a separately defined controller-authorized workflow and audit record.
No new role/PIN system is introduced by this proposal.

## Page structure and style

Use the same graphite shell, header/state indicator, 8 px rhythm, 6 px corners,
48 px minimum controls, and bottom navigation as Settings. Device and Settings
become clickable destinations in the [same HTML](../images/theme/hmi-web-demo.html).
Representative browser captures: [Overview](../images/theme/device-overview-concept-800x480.png)
and [Component detail](../images/theme/device-component-concept-800x480.png).
Home remains a visual reference; Programs now links to the separate
[styling proposal](PROGRAMS_STYLE_PROPOSAL.md) in the same concept. Preserve unsaved Settings prompts
when switching to Device. Device opens Overview and retains category/filter
state locally while navigating. Reference viewport is 800 x 480; also inspect
800 x 640 and 1024 x 600.

| Left category | Worker content |
| --- | --- |
| Overview | Three principal runtime readings, service attention summary, and current errors with links |
| Components | Configured parts, next task/status, All / Needs attention filter, and component details |
| Notifications | Active faults/warnings and service reminders, separated by source/type, with All / Service / Errors filters |
| Service history | Controller-recorded inspections, replacements, and service-policy acknowledgements, clearly distinguished |
| Device info | Device identity, HMI/controller firmware, connection and snapshot validity, catalogue revision, and support reference when configured |

Lists use three touch rows per page at the short viewport with fixed Previous /
Next controls and counts. Large catalogues never require a single huge page or
rebuilding every row. Details use short pages/dialogs; production procedure
text is bounded and paginated, not an unbounded scrolling manual. Layout
validation must include long localized labels and large counters.

Overview attention cards link to the relevant filtered list. Do not use a
single green "Device healthy" label to summarize unknown or unmonitored parts.
"No current reported errors" is narrower and honest. A schedule showing time
remaining is not evidence that a component cannot fail.

## Counter definitions

| Counter | Proposed meaning / open detail |
| --- | --- |
| Total operational time | Accumulated time the furnace is powered on, including idle and active operation. Label **Powered-on time** in the UI. Identify what hardware state proves powered-on; controller uptime is not automatically equivalent if power domains differ |
| Total program runtime | Accumulated time running accepted Programs. Excludes Manual. Proposed policy excludes paused time; confirm whether the user instead wants total occupied Program time including pauses |
| Fan runtime | Accumulated fan running time, including permitted cooldown outside a Program. If only a command is available, label **Fan commanded-on time**; do not imply measured rotation |

Future useful counters, only when exposed: Manual runtime, heater energized
time, completed/aborted program counts, fan starts, contactor cycles, and uptime
since the last controller boot (distinct from lifetime powered-on time).
Avoid displaying synthetic estimates of energy consumption or remaining
component life as measurements.

Controller counters are monotonic, persistent, sufficiently wide, and tied to
device identity and counter generation. HMI timers do not accumulate these
totals. Counter definitions state inclusion/exclusion rules, units, provenance,
and expected precision. Power-loss persistence may have bounded uncertainty;
define and expose it rather than promising exact lifetime totals.

Component replacement starts a new **installed instance** and service baseline;
it does not erase furnace lifetime totals or the old part's history. Display
installed-part age, since-last-service usage, and lifetime totals as different
values. Counter discontinuity/generation mismatch makes a service comparison
unknown until reconciled; never calculate a negative remaining lifetime.

## Extensible component catalogue

**Recommendation:** one developer-owned declarative catalogue per product
configuration, with generic component/task rendering. Do not add a new screen,
switch statement, or widget callback for each part. Proposal and alternatives
are in [ADR-0011](../07-decisions/0011-component-service-catalogue.md).

Each configured component has a stable component ID, installed-instance ID,
localized display name, optional part number/location, and a bounded task list.
Each task has a stable task ID, action label, interval basis/units, interval or
explicit condition-only status, warning lead, manufacturer reference/revision,
approved procedure reference, and catalogue revision. Installation/service
baselines, counters, next-due results, applicability, permissions, and service
history are controller-owned runtime observations, not editable catalogue UI.

Illustrative configuration shape (not a wire schema or production C API):

```text
component: cabinet_fan
  display_name_key: component.cabinet_fan
  product_variant: target-config-selected
  part_number: approved-BOM-reference
  tasks:
    - id: inspect_fan
      action_key: service.inspect_fan
      triggers:
        - basis: component_runtime
          counter_id: cabinet_fan_rotating_seconds
          interval: approved-manufacturer-value
        - basis: calendar
          interval: approved-manufacturer-calendar-period
      due_when: any_trigger_due
      warning_lead: approved-policy
      source: manufacturer-document-and-revision
      procedure_key: approved-fan-inspection
```

Adding a part: select it in the product catalogue, give it stable IDs and
localization, supply verified manufacturer/BOM/procedure references, bind an
existing counter or add a controller counter provider, and validate the
configuration. Build/publish it through the owning product configuration path;
the worker cannot add parts or edit these records. Hardware enablement, pin
mapping and drivers remain in Zephyr devicetree/Kconfig/board/adapters, not the
portable catalogue or renderer.

The controller's current catalogue/policy is authoritative. Avoid independent
HMI/controller interval tables that can disagree. A build-owned manifest may
generate controller policy and HMI localization metadata, or the controller
may expose bounded catalogue metadata; exact deployment/serialization is open.
In either case the HMI displays controller-reported due state and never
authorizes continued operation from a locally calculated schedule.

Initial catalogue entries worth considering include fans, filters, heating
elements, thermocouples, contactors, SSRs, power supplies, seals, terminals,
wiring, and protective devices. These are candidate part categories, not a
claim that this furnace contains them or that each has a replacement interval.
Every configured electrical component can have an inspection/condition task;
do not invent scheduled replacement for parts whose manufacturer specifies
none. Do not treat an electrical endurance rating as a maintenance interval.

The HTML's `deviceCatalogue` demonstrates adding entries without renderer
changes. All example part names, intervals, counters, dates, notices and records
are synthetic. No manufacturer values or actual BOM were supplied or researched.

## Service intervals and due states

Support component-runtime, powered-on-time, Program-runtime, calendar, switching
cycles, and condition-based tasks when their sources exist. Multiple triggers
can use **whichever comes first**, evaluated by the controller. If one trigger
is known due and another is unknown, keep Due; if none is due but a required
trigger is unknown, do not claim Up to date. A calendar policy must define
calendar months versus elapsed days, baseline, leap dates, and invalid or
changed wall-clock behavior. Hours are not interchangeable with calendar age.

| Status | Presentation |
| --- | --- |
| Up to date | No configured task currently due; show next threshold/date |
| Due soon | Show task and remaining usage/date, using controller warning policy |
| Due / Overdue | Show task and due point / overdue amount; do not truncate overdue to zero |
| Condition-based | Show inspection basis; no fake countdown or percentage |
| Unknown / Not configured | Explain missing counter, clock, baseline, interval, or metadata |
| Stale | Mark last observed data and age; do not present prior state as current |

Component summary shows its most urgent task; details retain every task and
its independent baseline/history. Fixed maintenance thresholds are configured
policy, not a diagnosis that something is broken. Distinguish **Inspection due**
from **Fault reported** and **Replacement required**. Only a controller or
approved source may assert the latter.

No service progress bar is a remaining-life guarantee. Prefer explicit
"220 fan hours remaining" / "Due on ..." / "30 h overdue" with basis labels.
The controller alone determines whether a service condition blocks a proposed
run or offers its existing one-time acknowledgement. Per-part reminders do
not silently multiply or reset the accepted REQ-FUN-016 acknowledgement policy.

## Notifications, guidance, and history

Order active faults before warnings, then overdue/due/soon service work, with
stable identity and timestamps rather than reshuffling on every telemetry tick.
Separate active errors from resolved/history records. Show component, task or
stable error code, first/last occurrence and validity, controller impact if
known, and an approved worker-level next step. Recurring events are grouped
only when stable event identity/rules permit it; preserve underlying history.

Opening or closing details does not acknowledge a fault, clear an error,
reset an interval, record service completion, or grant a service waiver.
Initial Device is read-only. Existing controller-issued service-acknowledgement
records can be viewed in Service history; label them **Acknowledgement**, not
**Repair completed**. Any future "report an issue" or "request service" action
needs a real endpoint and delivery status; defer it rather than faking a ticket.

Guidance comes from approved, versioned procedures. Worker notices identify the
affected part and how to refer work to service personnel. Do not expose live
electrical troubleshooting instructions, test actuators, override interlocks,
or invent fault recovery procedures from a code alone. Unknown codes retain
the exact code and source with "Guidance unavailable". Fault and service policy
remain controller/hardware responsibilities.

Service history should show component/task, event kind, controller timestamp,
relevant usage baseline, procedure/reference, and authorized recorder when
available. Inspection, replacement, and acknowledgement remain distinct.
History is controller-persisted and read-only here. A future correction should
append an audit correction, not silently rewrite old evidence. Record retention,
export and pagination semantics remain open.

Device info contains supported identity/version/catalogue fields with their
owner and validity, not made-up serial numbers. Optional support details and
manual reference are product-configured. No credentials or private pairing
material are exposed. Later diagnostic export is useful if a bounded,
redacted export path is actually available; it is not part of the first slice.

## Staleness, discovery, and navigation

Show last-known counters with Stale + snapshot age on link loss; initial absence
shows Unavailable, never zeros. Do not keep ticking counters or counting down
service dates from the browser/HMI clock. Session change invalidates capability
and catalogue observations; refresh coherent identity/catalogue/state before
claiming current values. Unknown configured parts remain visible with Unknown;
parts not in the configured catalogue are absent. An empty catalogue explicitly
says no components are configured, not that all parts are healthy.

Pages identify loading, unavailable, stale, empty, and retrieval-failed states.
A paged catalogue/history read is revision-bound; avoid merging pages across
catalogue changes. Persistent IDs maintain selection and retired-instance
history. A removed selected component yields a clear return-to-list state.

**Existing rule:** REQ-FUN-003 contains active/paused Programs on Running until
normal Stop, so Device is ordinarily reached outside that workflow (Manual
retains navigation). This proposal does not silently change it. Necessary
fault/notification summaries remain visible in the running context without
obstructing normal Stop. A new read-only Device route during Program execution
would need a separate explicit navigation decision.

## Open choices for implementation

- Include paused Program time in the named runtime total, or expose it separately?
- Which hardware signal defines furnace powered-on and confirms each fan running?
- Actual component/BOM list, manufacturer intervals, warning leads, procedures,
  counter sources, and electrical-component applicability.
- Catalogue ownership/deployment, maximum counts/string sizes, identity and
  migration rules, and controller per-task service policy.
- Who may record completed maintenance outside this worker page, and how are
  replacements, service baselines, audit corrections, and global service gates related?

These do not block the clearly synthetic browser design.
