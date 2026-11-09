# Protocol layering

[ADR-0003](../07-decisions/0003-protocol-layering.md) accepts separation of
concerns, not a final API or wire design.

## Responsibilities

1. **Application semantics** define operation meaning, authority, lifecycle,
   and state-recovery behavior without UART or frame details.
2. **Serialized representation** maps approved semantic data to explicit,
   versioned, bounded wire values without native C layout.
3. **Framing and integrity** delimit units, detect corruption, and carry only
   the metadata required by the chosen reliability model.
4. **Transport adapter** moves framed bytes over UART/RS-422 or a simulator
   loopback and reports transport facts without inventing application success.

Telemetry scheduling and request coordination use the layers but must not
become hidden inside a device driver. Likewise, the HMI model consumes
semantic state rather than serialized buffers.

## Dependency direction

Portable application and controller-client code may depend on semantic
interfaces. Codec/framer code depends on an approved schema. Transport
adapters depend on platform APIs. Higher layers must not depend on a concrete
UART instance, MCU vendor SDK, compiler packing, or SDL test transport.

## Source sharing

The HMI uses a pinned, checked copy of the controller's codec
([ADR-0009](../07-decisions/0009-protocol-source-sharing.md)). The copy is the
serialized-representation and framing layer only; the semantic layer above it
is the HMI's own, and the codec's structures do not reach the model.
