/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Message layouts of the controller client protocol (module core/wire, Phase
 * 20b). One function per message moves each field in table order, in both
 * directions, so the encoder and the decoder cannot disagree about a layout
 * (ADR-0018, COMM-04). Each primitive is an explicit little-endian encoding;
 * no structure is ever copied. The tables of wire format §7 are the definition,
 * and tests/core/wire checks this file against the golden vectors they generate.
 */

#include <string.h>

#include <furnace/wire_messages.h>

struct wire_io {
	uint8_t *buf;
	size_t cap;
	size_t pos;
	bool encoding;
	bool strict; /* a client message: only defined values (wire format §5) */
	bool failed;
};

/* The next `size` bytes of the buffer, or NULL after marking the failure. */
static uint8_t *io_take(struct wire_io *io, size_t size)
{
	uint8_t *at;

	if (io->failed || (size > (io->cap - io->pos))) {
		io->failed = true;
		return NULL;
	}
	at = &io->buf[io->pos];
	io->pos += size;
	return at;
}

/* An unsigned value of `size` bytes, little-endian. */
static void io_uint(struct wire_io *io, uint64_t *value, size_t size)
{
	uint8_t *at = io_take(io, size);

	if (at == NULL) {
		return;
	}
	if (io->encoding) {
		for (size_t i = 0U; i < size; i++) {
			at[i] = (uint8_t)(*value >> (8U * i));
		}
	} else {
		uint64_t v = 0U;

		for (size_t i = 0U; i < size; i++) {
			v |= (uint64_t)at[i] << (8U * i);
		}
		*value = v;
	}
}

static void io_u8(struct wire_io *io, uint8_t *value)
{
	uint64_t v = *value;

	io_uint(io, &v, 1U);
	if (!io->encoding) {
		*value = (uint8_t)v;
	}
}

static void io_u16(struct wire_io *io, uint16_t *value)
{
	uint64_t v = *value;

	io_uint(io, &v, 2U);
	if (!io->encoding) {
		*value = (uint16_t)v;
	}
}

static void io_u32(struct wire_io *io, uint32_t *value)
{
	uint64_t v = *value;

	io_uint(io, &v, 4U);
	if (!io->encoding) {
		*value = (uint32_t)v;
	}
}

static void io_id128(struct wire_io *io, uint8_t *value)
{
	uint8_t *at = io_take(io, FURNACE_WIRE_SESSION_ID_SIZE);

	if (at == NULL) {
		return;
	}
	if (io->encoding) {
		memcpy(at, value, FURNACE_WIRE_SESSION_ID_SIZE);
	} else {
		memcpy(value, at, FURNACE_WIRE_SESSION_ID_SIZE);
	}
}

static void io_u64(struct wire_io *io, uint64_t *value)
{
	io_uint(io, value, 8U);
}

static void io_i32(struct wire_io *io, int32_t *value)
{
	uint32_t v = (uint32_t)*value;

	io_u32(io, &v);
	if (!io->encoding) {
		*value = (v <= (uint32_t)INT32_MAX) ? (int32_t)v : (-(int32_t)(UINT32_MAX - v) - 1);
	}
}

static void io_i64(struct wire_io *io, int64_t *value)
{
	uint64_t v = (uint64_t)*value;

	io_u64(io, &v);
	if (!io->encoding) {
		*value = (v <= (uint64_t)INT64_MAX) ? (int64_t)v : (-(int64_t)(UINT64_MAX - v) - 1);
	}
}

/* One byte, 0 or 1; another value is malformed in every message. */
static void io_bool(struct wire_io *io, bool *value)
{
	uint8_t v = *value ? 1U : 0U;

	io_u8(io, &v);
	if (v > 1U) {
		io->failed = true;
	}
	if (!io->encoding) {
		*value = (v == 1U);
	}
}

/* An enumeration with defined values `low`…`high`, checked in client messages. */
static void io_enum8(struct wire_io *io, uint8_t *value, uint8_t low, uint8_t high)
{
	io_u8(io, value);
	if (io->strict && ((*value < low) || (*value > high))) {
		io->failed = true;
	}
}

/* A bit set with defined bits `mask`, checked in client messages. */
static void io_bits32(struct wire_io *io, uint32_t *value, uint32_t mask)
{
	io_u32(io, value);
	if (io->strict && ((*value & ~mask) != 0U)) {
		io->failed = true;
	}
}

/* Strict UTF-8 (wire format §7.1): no overlong form, surrogate or NUL. */
static bool utf8_is_strict(const uint8_t *s, size_t len)
{
	size_t i = 0U;

	while (i < len) {
		const uint8_t lead = s[i];
		uint32_t cp;
		size_t extra;
		uint32_t minimum;

		if ((lead == 0U) || ((lead >= 0x80U) && (lead < 0xC2U)) || (lead > 0xF4U)) {
			return false;
		}
		if (lead < 0x80U) {
			i++;
			continue;
		}
		if (lead < 0xE0U) {
			extra = 1U;
			minimum = 0x80U;
			cp = lead & 0x1FU;
		} else if (lead < 0xF0U) {
			extra = 2U;
			minimum = 0x800U;
			cp = lead & 0x0FU;
		} else {
			extra = 3U;
			minimum = 0x10000U;
			cp = lead & 0x07U;
		}
		if (extra >= (len - i)) {
			return false;
		}
		for (size_t k = 1U; k <= extra; k++) {
			if ((s[i + k] & 0xC0U) != 0x80U) {
				return false;
			}
			cp = (cp << 6) | (s[i + k] & 0x3FU);
		}
		if ((cp < minimum) || (cp > 0x10FFFFU) || ((cp >= 0xD800U) && (cp <= 0xDFFFU))) {
			return false;
		}
		i += extra + 1U;
	}
	return true;
}

/* A `u8` length and that many bytes; `s` holds bound + 1 bytes, NUL-terminated. */
static void io_utf8(struct wire_io *io, char *s, uint8_t bound)
{
	uint8_t len = 0U;
	uint8_t *at;

	if (io->encoding) {
		while ((len < bound) && (s[len] != '\0')) {
			len++;
		}
		if (s[len] != '\0') {
			io->failed = true;
			return;
		}
	}
	io_u8(io, &len);
	if (io->failed || (len > bound)) {
		io->failed = true;
		return;
	}
	at = io_take(io, len);
	if (at == NULL) {
		return;
	}
	if (io->encoding) {
		if (!utf8_is_strict((const uint8_t *)s, len)) {
			io->failed = true;
			return;
		}
		memcpy(at, s, len);
	} else {
		if (!utf8_is_strict(at, len)) {
			io->failed = true;
			return;
		}
		memcpy(s, at, len);
		s[len] = '\0';
	}
}

/* An array count; a value above the bound never reaches the element loop. */
static void io_count(struct wire_io *io, uint8_t *count, uint8_t bound)
{
	io_u8(io, count);
	if (*count > bound) {
		io->failed = true;
	}
}

static void io_measured(struct wire_io *io, struct furnace_wire_measured *m)
{
	io_i32(io, &m->value);
	io_u8(io, &m->validity);
	if (m->validity == 0U) {
		io->failed = true; /* never valid, in any message (wire format §4) */
	}
	io_u32(io, &m->age_ms);
}

/* The unused fields of a stage are zero in a client message (wire format §7.1). */
static void io_stage_rule(struct wire_io *io, const struct furnace_wire_stage *m)
{
	const bool heating = m->kind == 1U;

	if (io->strict &&
	    (heating ? (m->duration_minutes != 0U) : ((m->target != 0) || (m->rate != 0)))) {
		io->failed = true;
	}
}

static void io_envelope(struct wire_io *io, struct furnace_wire_envelope *m)
{
	io_id128(io, m->session_id);
	io_u32(io, &m->client_session_id);
	io_u32(io, &m->request_id);
}

static void io_stage(struct wire_io *io, struct furnace_wire_stage *m)
{
	io_enum8(io, &m->kind, 1U, 2U);
	io_i32(io, &m->target);
	io_i32(io, &m->rate);
	io_u32(io, &m->duration_minutes);
	io_stage_rule(io, m);
}

static void io_skipped_stage(struct wire_io *io, struct furnace_wire_skipped_stage *m)
{
	io_u8(io, &m->index);
	io_u8(io, &m->kind);
	io_i32(io, &m->target);
	io_u32(io, &m->hold_minutes);
}

static void io_fault_entry(struct wire_io *io, struct furnace_wire_fault_entry *m)
{
	io_u8(io, &m->fault_id);
	io_u8(io, &m->source);
	io_u8(io, &m->class);
	io_bool(io, &m->condition_active);
	io_bool(io, &m->latched);
	io_bool(io, &m->acknowledged);
	io_u8(io, &m->recovery);
	io_u32(io, &m->occurrences);
	io_u64(io, &m->first_at);
	io_u64(io, &m->last_change);
	io_i64(io, &m->value);
	io_u8(io, &m->category);
	io_bool(io, &m->continuable);
	io_bool(io, &m->continued);
	io_u32(io, &m->first_boot);
	io_u32(io, &m->last_boot);
}

static void io_library_entry(struct wire_io *io, struct furnace_wire_library_entry *m)
{
	io_u32(io, &m->program_id);
	io_u32(io, &m->program_revision);
	io_utf8(io, m->name, 124U);
	io_u8(io, &m->stage_count);
	io_i32(io, &m->max_target);
	io_u32(io, &m->curing_estimate_s);
	io_i32(io, &m->planning_reference);
	io_bool(io, &m->damaged);
}

static void io_health_subsystem(struct wire_io *io, struct furnace_wire_health_subsystem *m)
{
	io_u8(io, &m->subsystem);
	io_u8(io, &m->state);
	io_u32(io, &m->detail);
}

static void io_event_param(struct wire_io *io, struct furnace_wire_event_param *m)
{
	io_u8(io, &m->type);
	io_u32(io, &m->value);
}

static void io_history_entry(struct wire_io *io, struct furnace_wire_history_entry *m)
{
	io_u32(io, &m->log_sequence);
	io_u32(io, &m->boot_id);
	io_u32(io, &m->sequence);
	io_u64(io, &m->mono_ms);
	io_u8(io, &m->wall_state);
	io_i64(io, &m->wall_ms);
	io_u16(io, &m->event_id);
	io_u8(io, &m->severity);
	io_u8(io, &m->category);
	io_count(io, &m->params_count, 8U);
	for (uint8_t i = 0U; (i < m->params_count) && !io->failed; i++) {
		io_event_param(io, &m->params[i]);
	}
}

static void io_fault_status(struct wire_io *io, struct furnace_wire_fault_status *m)
{
	io_bool(io, &m->heating_inhibited);
	io_bool(io, &m->resume_allowed);
	io_u8(io, &m->latched_class);
	io_u32(io, &m->inhibit_ids);
	io_u32(io, &m->continuable_ids);
	io_u8(io, &m->unacknowledged_count);
	io_u32(io, &m->last_sequence);
	io_u32(io, &m->history_dropped);
}

static void io_start_status(struct wire_io *io, struct furnace_wire_start_status *m)
{
	io_u32(io, &m->transaction_id);
	io_u32(io, &m->proposal_revision);
	io_u8(io, &m->state);
	io_u8(io, &m->invalidation_reason);
}

static void io_previous_run(struct wire_io *io, struct furnace_wire_previous_run *m)
{
	io_u32(io, &m->run_id);
	io_u32(io, &m->program_id);
	io_u32(io, &m->program_revision);
	io_utf8(io, m->program_name, 124U);
	io_u8(io, &m->session_kind);
	io_u8(io, &m->outcome);
	io_u8(io, &m->termination);
	io_u32(io, &m->review_flags);
	io_u8(io, &m->cooling);
	io_u8(io, &m->cooling_ended_by);
	io_u32(io, &m->cooling_s);
}

static void io_run_status(struct wire_io *io, struct furnace_wire_run_status *m)
{
	io_u8(io, &m->run_state);
	io_u8(io, &m->fault_class);
	io_u32(io, &m->run_id);
	io_u8(io, &m->session_kind);
	io_u32(io, &m->program_id);
	io_u32(io, &m->program_revision);
	io_utf8(io, m->program_name, 124U);
	io_u8(io, &m->starting_stage);
	io_i32(io, &m->start_temperature);
	io_u8(io, &m->stage_index);
	io_u8(io, &m->stage_count);
	io_u8(io, &m->stage_kind);
	io_bool(io, &m->setpoint_valid);
	io_i32(io, &m->setpoint);
	io_bool(io, &m->setpoint_approaching);
	io_bool(io, &m->setpoint_moving);
	io_measured(io, &m->process_temperature);
	io_u8(io, &m->process_role);
	io_bool(io, &m->heating_requested);
	io_u32(io, &m->pause_reasons);
	io_u32(io, &m->blocking_conditions);
	io_u32(io, &m->decision_reasons);
	io_u32(io, &m->decision_remaining_s);
	io_u32(io, &m->run_elapsed_s);
	io_u32(io, &m->paused_s);
	io_u32(io, &m->run_nominal_s);
	io_u32(io, &m->accepted_s);
	io_u32(io, &m->running_s);
	io_u32(io, &m->runtime_limit_s);
	io_u32(io, &m->measurement_qualified_s);
	io_u32(io, &m->hold_undershoot_s);
	io_u8(io, &m->cooling);
	io_u32(io, &m->cooling_s);
	io_bool(io, &m->program_running);
	io_i32(io, &m->manual_target);
	io_i32(io, &m->manual_rate);
	io_u32(io, &m->manual_remaining_s);
	io_u8(io, &m->termination);
	io_u32(io, &m->review_flags);
	io_u8(io, &m->outcome);
	io_u8(io, &m->curing_acceptance);
	io_previous_run(io, &m->previous_run);
}

static void io_ramp_progress(struct wire_io *io, struct furnace_wire_ramp_progress *m)
{
	io_u8(io, &m->ramp_phase);
	io_i32(io, &m->tracking_deviation);
	io_u32(io, &m->holdback_s);
	io_u32(io, &m->holdback_total_s);
	io_u32(io, &m->holdback_count);
	io_u32(io, &m->stage_elapsed_s);
	io_u32(io, &m->ramp_nominal_s);
	io_u32(io, &m->ramp_delay_s);
	io_u32(io, &m->target_wait_s);
}

static void io_hold_progress(struct wire_io *io, struct furnace_wire_hold_progress *m)
{
	io_u8(io, &m->hold_phase);
	io_u8(io, &m->hold_condition);
	io_u32(io, &m->stage_elapsed_s);
	io_u32(io, &m->qualification_elapsed_s);
	io_u32(io, &m->treatment_credited_s);
	io_u32(io, &m->treatment_remaining_s);
	io_u32(io, &m->interruption_count);
	io_u32(io, &m->uncredited_after_qualification_s);
	io_i32(io, &m->max_deviation);
	io_u32(io, &m->excursion_count);
	io_u32(io, &m->excursion_s);
}

static void io_control_diagnostics(struct wire_io *io, struct furnace_wire_control_diagnostics *m)
{
	io_u16(io, &m->heating_power_request);
	io_i32(io, &m->control_error);
	io_u32(io, &m->control_flags);
}

static void io_heat_monitor(struct wire_io *io, struct furnace_wire_heat_monitor *m)
{
	io_u8(io, &m->mode);
	io_bool(io, &m->observed);
	io_bool(io, &m->above);
	io_bool(io, &m->suspected);
	io_bool(io, &m->condition);
	io_bool(io, &m->latched);
	io_u8(io, &m->basis);
	io_bool(io, &m->reference_valid);
	io_i32(io, &m->reference);
	io_i32(io, &m->ceiling);
	io_i32(io, &m->rise);
	io_u32(io, &m->evidence_s);
	io_u32(io, &m->counter_s);
	io_u32(io, &m->observed_s);
	io_u32(io, &m->unresolved_s);
	io_u32(io, &m->gap_s);
	io_u32(io, &m->gap_total_s);
	io_bool(io, &m->unobservable);
	io_u32(io, &m->suspicions);
	io_u32(io, &m->confirmations);
	io_u32(io, &m->escalations);
}

static void io_controller(struct wire_io *io, struct furnace_wire_controller *m)
{
	io_bool(io, &m->configured);
	io_u8(io, &m->config_state);
	io_u8(io, &m->storage);
	io_bool(io, &m->log_persistent);
	io_bool(io, &m->session_fresh);
	io_u8(io, &m->store_readiness);
}

static void io_lifecycle(struct wire_io *io, struct furnace_wire_lifecycle *m)
{
	io_bool(io, &m->execution_active);
	io_bool(io, &m->heating_authorized);
	io_bool(io, &m->navigation_permitted);
	io_bool(io, &m->library_mutations_permitted);
	io_bool(io, &m->settings_changes_permitted);
}

static void io_equipment(struct wire_io *io, struct furnace_wire_equipment *m)
{
	io_u8(io, &m->fan_command);
	io_u8(io, &m->fan_reason);
	io_u8(io, &m->fan_feedback);
	io_u8(io, &m->door_state);
	io_u8(io, &m->measurement_state);
	io_bool(io, &m->emergency_active);
	io_bool(io, &m->warning_active);
	io_u8(io, &m->warning_output);
}

static void io_msg_hello(struct wire_io *io, struct furnace_wire_hello *m)
{
	io_u32(io, &m->client_session_id);
	io_enum8(io, &m->client_kind, 1U, 2U);
}

static void io_msg_goodbye(struct wire_io *io, struct furnace_wire_goodbye *m)
{
	io_envelope(io, &m->envelope);
}

static void io_msg_heartbeat(struct wire_io *io, struct furnace_wire_heartbeat *m)
{
	io_u32(io, &m->client_session_id);
}

static void io_msg_snapshot_request(struct wire_io *io, struct furnace_wire_snapshot_request *m)
{
	io_envelope(io, &m->envelope);
}

static void io_msg_capabilities_request(struct wire_io *io,
					struct furnace_wire_capabilities_request *m)
{
	io_envelope(io, &m->envelope);
}

static void io_msg_settings_request(struct wire_io *io, struct furnace_wire_settings_request *m)
{
	io_envelope(io, &m->envelope);
}

static void io_msg_setting_change(struct wire_io *io, struct furnace_wire_setting_change *m)
{
	io_envelope(io, &m->envelope);
	io_enum8(io, &m->setting, 1U, 1U);
	io_i32(io, &m->value);
}

static void io_msg_start_request(struct wire_io *io, struct furnace_wire_start_request *m)
{
	io_envelope(io, &m->envelope);
	io_u32(io, &m->program_id);
	io_u32(io, &m->program_revision);
}

static void io_msg_start_confirm(struct wire_io *io, struct furnace_wire_start_confirm *m)
{
	io_envelope(io, &m->envelope);
	io_u32(io, &m->transaction_id);
	io_u32(io, &m->proposal_revision);
	io_u32(io, &m->program_id);
	io_u32(io, &m->program_revision);
	io_u8(io, &m->proposed_stage);
	io_u32(io, &m->accepted_duration_s);
}

static void io_msg_start_cancel(struct wire_io *io, struct furnace_wire_start_cancel *m)
{
	io_envelope(io, &m->envelope);
	io_u32(io, &m->transaction_id);
}

static void io_msg_start_prepare(struct wire_io *io, struct furnace_wire_start_prepare *m)
{
	io_envelope(io, &m->envelope);
}

static void io_msg_run_pause(struct wire_io *io, struct furnace_wire_run_pause *m)
{
	io_envelope(io, &m->envelope);
	io_u32(io, &m->run_id);
}

static void io_msg_resume_request(struct wire_io *io, struct furnace_wire_resume_request *m)
{
	io_envelope(io, &m->envelope);
	io_u32(io, &m->run_id);
}

static void io_msg_run_stop(struct wire_io *io, struct furnace_wire_run_stop *m)
{
	io_envelope(io, &m->envelope);
	io_u32(io, &m->run_id);
}

static void io_msg_run_continue(struct wire_io *io, struct furnace_wire_run_continue *m)
{
	io_envelope(io, &m->envelope);
	io_u32(io, &m->run_id);
}

static void io_msg_manual_start(struct wire_io *io, struct furnace_wire_manual_start *m)
{
	io_envelope(io, &m->envelope);
	io_i32(io, &m->target);
	io_i32(io, &m->rate);
	io_u32(io, &m->session_minutes);
}

static void io_msg_manual_retarget(struct wire_io *io, struct furnace_wire_manual_retarget *m)
{
	io_envelope(io, &m->envelope);
	io_u32(io, &m->run_id);
	io_i32(io, &m->target);
	io_i32(io, &m->rate);
}

static void io_msg_manual_set_length(struct wire_io *io, struct furnace_wire_manual_set_length *m)
{
	io_envelope(io, &m->envelope);
	io_u32(io, &m->run_id);
	io_u32(io, &m->session_minutes);
}

static void io_msg_fault_acknowledge(struct wire_io *io, struct furnace_wire_fault_acknowledge *m)
{
	io_envelope(io, &m->envelope);
	io_enum8(io, &m->fault_id, 1U, 18U);
	io_u8(io, &m->source);
}

static void io_msg_fault_recovery_request(struct wire_io *io,
					  struct furnace_wire_fault_recovery_request *m)
{
	io_envelope(io, &m->envelope);
	io_enum8(io, &m->fault_id, 1U, 18U);
	io_u8(io, &m->source);
}

static void io_msg_program_list_request(struct wire_io *io,
					struct furnace_wire_program_list_request *m)
{
	io_envelope(io, &m->envelope);
	io_u8(io, &m->first_index);
	io_u8(io, &m->max_count);
}

static void io_msg_program_read_request(struct wire_io *io,
					struct furnace_wire_program_read_request *m)
{
	io_envelope(io, &m->envelope);
	io_u32(io, &m->program_id);
}

static void io_msg_program_save(struct wire_io *io, struct furnace_wire_program_save *m)
{
	io_envelope(io, &m->envelope);
	io_u32(io, &m->program_id);
	io_u32(io, &m->base_revision);
	io_u32(io, &m->acknowledged_duration_s);
	io_utf8(io, m->name, 124U);
	io_count(io, &m->stages_count, 18U);
	for (uint8_t i = 0U; (i < m->stages_count) && !io->failed; i++) {
		io_stage(io, &m->stages[i]);
	}
}

static void io_msg_program_delete(struct wire_io *io, struct furnace_wire_program_delete *m)
{
	io_envelope(io, &m->envelope);
	io_u32(io, &m->program_id);
	io_u32(io, &m->program_revision);
}

static void io_msg_program_estimate_request(struct wire_io *io,
					    struct furnace_wire_program_estimate_request *m)
{
	io_envelope(io, &m->envelope);
	io_u32(io, &m->program_id);
	io_u32(io, &m->program_revision);
	io_enum8(io, &m->basis, 1U, 2U);
}

static void io_msg_health_request(struct wire_io *io, struct furnace_wire_health_request *m)
{
	io_envelope(io, &m->envelope);
}

static void io_msg_event_history_request(struct wire_io *io,
					 struct furnace_wire_event_history_request *m)
{
	io_envelope(io, &m->envelope);
	io_enum8(io, &m->log, 1U, 3U);
	io_u32(io, &m->from_sequence);
	io_u8(io, &m->max_entries);
	io_bits32(io, &m->categories, 0x000003FEU);
	io_enum8(io, &m->min_severity, 0U, 3U);
	io_u32(io, &m->boot_id);
	io_u64(io, &m->from_ms);
	io_u64(io, &m->to_ms);
}

static void io_msg_hello_ack(struct wire_io *io, struct furnace_wire_hello_ack *m)
{
	io_u8(io, &m->result);
	io_id128(io, m->session_id);
	io_u32(io, &m->firmware_id);
	io_u32(io, &m->boot_count);
}

static void io_msg_command_rejected(struct wire_io *io, struct furnace_wire_command_rejected *m)
{
	io_u32(io, &m->request_id);
	io_u16(io, &m->msg_type);
	io_u8(io, &m->reason);
}

static void io_msg_status(struct wire_io *io, struct furnace_wire_status *m)
{
	io_id128(io, m->session_id);
	io_u32(io, &m->status_sequence);
	io_u32(io, &m->events_dropped);
	io_u32(io, &m->library_revision);
	io_measured(io, &m->measured_temperature);
	io_u8(io, &m->operating_mode);
	io_count(io, &m->restrictions_count, 8U);
	for (uint8_t i = 0U; (i < m->restrictions_count) && !io->failed; i++) {
		io_u8(io, &m->restrictions[i]);
	}
	io_fault_status(io, &m->fault_status);
	io_start_status(io, &m->start);
	io_run_status(io, &m->run_status);
	io_bool(io, &m->ramp_progress_present);
	if (m->ramp_progress_present && !io->failed) {
		io_ramp_progress(io, &m->ramp_progress);
	}
	io_bool(io, &m->hold_progress_present);
	if (m->hold_progress_present && !io->failed) {
		io_hold_progress(io, &m->hold_progress);
	}
	io_bool(io, &m->control_diagnostics_present);
	if (m->control_diagnostics_present && !io->failed) {
		io_control_diagnostics(io, &m->control_diagnostics);
	}
	io_heat_monitor(io, &m->heat_monitor);
	io_controller(io, &m->controller);
	io_lifecycle(io, &m->lifecycle);
	io_equipment(io, &m->equipment);
}

static void io_msg_snapshot_end(struct wire_io *io, struct furnace_wire_snapshot_end *m)
{
	io_u32(io, &m->request_id);
	io_u32(io, &m->status_sequence);
	io_u32(io, &m->fault_sequence);
	io_u32(io, &m->run_event_sequence);
	io_u32(io, &m->library_revision);
}

static void io_msg_capabilities(struct wire_io *io, struct furnace_wire_capabilities *m)
{
	io_u32(io, &m->request_id);
	io_id128(io, m->session_id);
	io_u8(io, &m->proto_major);
	io_u8(io, &m->proto_minor);
	io_u32(io, &m->firmware_id);
	io_bool(io, &m->configured);
	io_u8(io, &m->config_state);
	io_utf8(io, m->model_id, 31U);
	io_u32(io, &m->config_revision);
	io_u32(io, &m->profile_revision);
	io_u8(io, &m->max_stages);
	io_u32(io, &m->stage_kinds);
	io_i32(io, &m->target_min);
	io_i32(io, &m->target_max);
	io_i32(io, &m->rate_min);
	io_i32(io, &m->rate_max);
	io_u32(io, &m->curing_max_minutes);
	io_u32(io, &m->recommended_curing_minutes);
	io_i32(io, &m->planning_reference_min);
	io_i32(io, &m->planning_reference_max);
	io_u8(io, &m->name_max_code_points);
	io_u8(io, &m->name_max_bytes);
	io_u32(io, &m->manual_max_minutes);
	io_u8(io, &m->library_capacity);
	io_u16(io, &m->max_payload);
	io_u32(io, &m->status_interval_ms);
	io_u32(io, &m->heartbeat_interval_ms);
	io_u32(io, &m->link_silence_ms);
}

static void io_msg_settings(struct wire_io *io, struct furnace_wire_settings *m)
{
	io_u32(io, &m->request_id);
	io_u8(io, &m->settings_state);
	io_i32(io, &m->planning_reference);
}

static void io_msg_setting_result(struct wire_io *io, struct furnace_wire_setting_result *m)
{
	io_u32(io, &m->request_id);
	io_u8(io, &m->setting);
	io_u8(io, &m->result);
	io_i32(io, &m->value);
}

static void io_msg_start_accepted(struct wire_io *io, struct furnace_wire_start_accepted *m)
{
	io_u32(io, &m->request_id);
	io_u32(io, &m->transaction_id);
	io_u32(io, &m->run_id);
	io_u8(io, &m->starting_stage);
	io_u32(io, &m->accepted_duration_s);
}

static void io_msg_start_proposal(struct wire_io *io, struct furnace_wire_start_proposal *m)
{
	io_u32(io, &m->request_id);
	io_u32(io, &m->transaction_id);
	io_u32(io, &m->proposal_revision);
	io_measured(io, &m->measured_temperature);
	io_u8(io, &m->proposed_stage);
	io_count(io, &m->skipped_stages_count, 18U);
	for (uint8_t i = 0U; (i < m->skipped_stages_count) && !io->failed; i++) {
		io_skipped_stage(io, &m->skipped_stages[i]);
	}
	io_u32(io, &m->remaining_curing_s);
	io_bool(io, &m->duration_ack_required);
	io_u32(io, &m->recommended_s);
	io_u32(io, &m->maximum_s);
	io_count(io, &m->warnings_count, 8U);
	for (uint8_t i = 0U; (i < m->warnings_count) && !io->failed; i++) {
		io_u8(io, &m->warnings[i]);
	}
}

static void io_msg_start_proposal_invalidated(struct wire_io *io,
					      struct furnace_wire_start_proposal_invalidated *m)
{
	io_u32(io, &m->transaction_id);
	io_u8(io, &m->reason);
}

static void io_msg_start_rejected(struct wire_io *io, struct furnace_wire_start_rejected *m)
{
	io_u32(io, &m->request_id);
	io_u8(io, &m->reason);
}

static void io_msg_start_cancelled(struct wire_io *io, struct furnace_wire_start_cancelled *m)
{
	io_u32(io, &m->request_id);
	io_u32(io, &m->transaction_id);
}

static void io_msg_start_prepare_result(struct wire_io *io,
					struct furnace_wire_start_prepare_result *m)
{
	io_u32(io, &m->request_id);
	io_u8(io, &m->store_readiness);
}

static void io_msg_run_command_accepted(struct wire_io *io,
					struct furnace_wire_run_command_accepted *m)
{
	io_u32(io, &m->request_id);
	io_u8(io, &m->command);
	io_u8(io, &m->reason);
	io_u32(io, &m->blocking_conditions);
	io_u32(io, &m->run_id);
}

static void io_msg_run_command_rejected(struct wire_io *io,
					struct furnace_wire_run_command_rejected *m)
{
	io_u32(io, &m->request_id);
	io_u8(io, &m->command);
	io_u8(io, &m->reason);
	io_u32(io, &m->blocking_conditions);
	io_u32(io, &m->run_id);
}

static void io_msg_run_event(struct wire_io *io, struct furnace_wire_run_event *m)
{
	io_u32(io, &m->sequence);
	io_u32(io, &m->run_id);
	io_u8(io, &m->event);
	io_u8(io, &m->stage);
	io_i64(io, &m->value);
}

static void io_msg_fault_event(struct wire_io *io, struct furnace_wire_fault_event *m)
{
	io_u32(io, &m->sequence);
	io_u64(io, &m->at);
	io_u8(io, &m->fault_id);
	io_u8(io, &m->source);
	io_u8(io, &m->class);
	io_u8(io, &m->transition);
	io_u8(io, &m->consequence);
	io_u8(io, &m->controller_state);
	io_i64(io, &m->value);
	io_bool(io, &m->time_regressed);
	io_u32(io, &m->boot);
}

static void io_msg_fault_acknowledge_result(struct wire_io *io,
					    struct furnace_wire_fault_acknowledge_result *m)
{
	io_u32(io, &m->request_id);
	io_u8(io, &m->fault_id);
	io_u8(io, &m->source);
	io_u8(io, &m->result);
}

static void io_msg_fault_recovery_result(struct wire_io *io,
					 struct furnace_wire_fault_recovery_result *m)
{
	io_u32(io, &m->request_id);
	io_u8(io, &m->fault_id);
	io_u8(io, &m->source);
	io_bool(io, &m->found);
	io_u8(io, &m->reply);
}

static void io_msg_fault_list(struct wire_io *io, struct furnace_wire_fault_list *m)
{
	io_u32(io, &m->request_id);
	io_u32(io, &m->status_sequence);
	io_u8(io, &m->total);
	io_u8(io, &m->first_index);
	io_count(io, &m->entries_count, 16U);
	for (uint8_t i = 0U; (i < m->entries_count) && !io->failed; i++) {
		io_fault_entry(io, &m->entries[i]);
	}
}

static void io_msg_program_list(struct wire_io *io, struct furnace_wire_program_list *m)
{
	io_u32(io, &m->request_id);
	io_u8(io, &m->result);
	io_u32(io, &m->library_revision);
	io_u8(io, &m->total);
	io_u8(io, &m->first_index);
	io_count(io, &m->entries_count, 6U);
	for (uint8_t i = 0U; (i < m->entries_count) && !io->failed; i++) {
		io_library_entry(io, &m->entries[i]);
	}
}

static void io_msg_program_data(struct wire_io *io, struct furnace_wire_program_data *m)
{
	io_u32(io, &m->request_id);
	io_u8(io, &m->result);
	io_u32(io, &m->program_id);
	io_u32(io, &m->program_revision);
	io_utf8(io, m->name, 124U);
	io_u32(io, &m->curing_estimate_s);
	io_i32(io, &m->planning_reference);
	io_count(io, &m->stages_count, 18U);
	for (uint8_t i = 0U; (i < m->stages_count) && !io->failed; i++) {
		io_stage(io, &m->stages[i]);
	}
}

static void io_msg_program_save_result(struct wire_io *io,
				       struct furnace_wire_program_save_result *m)
{
	io_u32(io, &m->request_id);
	io_u8(io, &m->result);
	io_u32(io, &m->program_id);
	io_u32(io, &m->program_revision);
	io_u8(io, &m->error_reason);
	io_u8(io, &m->error_field);
	io_u8(io, &m->error_stage);
	io_u32(io, &m->curing_estimate_s);
	io_i32(io, &m->planning_reference);
}

static void io_msg_program_delete_result(struct wire_io *io,
					 struct furnace_wire_program_delete_result *m)
{
	io_u32(io, &m->request_id);
	io_u8(io, &m->result);
	io_u32(io, &m->program_id);
}

static void io_msg_program_estimate(struct wire_io *io, struct furnace_wire_program_estimate *m)
{
	io_u32(io, &m->request_id);
	io_u8(io, &m->result);
	io_u32(io, &m->program_id);
	io_u32(io, &m->program_revision);
	io_u8(io, &m->basis);
	io_i32(io, &m->start_temperature);
	io_u8(io, &m->start_stage);
	io_u32(io, &m->curing_estimate_s);
	io_bool(io, &m->exceeds_recommended);
	io_bool(io, &m->above_maximum);
}

static void io_msg_health(struct wire_io *io, struct furnace_wire_health *m)
{
	io_u32(io, &m->request_id);
	io_id128(io, m->session_id);
	io_u64(io, &m->uptime_ms);
	io_u32(io, &m->boot_count);
	io_u8(io, &m->overall);
	io_bool(io, &m->reset_cause_known);
	io_u32(io, &m->reset_cause);
	io_count(io, &m->subsystems_count, 10U);
	for (uint8_t i = 0U; (i < m->subsystems_count) && !io->failed; i++) {
		io_health_subsystem(io, &m->subsystems[i]);
	}
	io_u8(io, &m->faults_active);
	io_u8(io, &m->faults_latched);
	io_u8(io, &m->faults_unacknowledged);
	io_u32(io, &m->service_s);
	io_bool(io, &m->service_valid);
}

static void io_msg_event_history_page(struct wire_io *io, struct furnace_wire_event_history_page *m)
{
	io_u32(io, &m->request_id);
	io_u8(io, &m->result);
	io_u8(io, &m->log);
	io_u32(io, &m->first_retained);
	io_u32(io, &m->next_sequence);
	io_u32(io, &m->evicted);
	io_u32(io, &m->scanned_to);
	io_bool(io, &m->more);
	io_count(io, &m->entries_count, 12U);
	for (uint8_t i = 0U; (i < m->entries_count) && !io->failed; i++) {
		io_history_entry(io, &m->entries[i]);
	}
}

/* False when the type has no layout. */
static bool io_message(struct wire_io *io, struct furnace_wire_message *m)
{
	switch (m->type) {
	case FURNACE_WIRE_MSG_HELLO:
		io_msg_hello(io, &m->u.hello);
		return true;
	case FURNACE_WIRE_MSG_GOODBYE:
		io_msg_goodbye(io, &m->u.goodbye);
		return true;
	case FURNACE_WIRE_MSG_HEARTBEAT:
		io_msg_heartbeat(io, &m->u.heartbeat);
		return true;
	case FURNACE_WIRE_MSG_SNAPSHOT_REQUEST:
		io_msg_snapshot_request(io, &m->u.snapshot_request);
		return true;
	case FURNACE_WIRE_MSG_CAPABILITIES_REQUEST:
		io_msg_capabilities_request(io, &m->u.capabilities_request);
		return true;
	case FURNACE_WIRE_MSG_SETTINGS_REQUEST:
		io_msg_settings_request(io, &m->u.settings_request);
		return true;
	case FURNACE_WIRE_MSG_SETTING_CHANGE:
		io_msg_setting_change(io, &m->u.setting_change);
		return true;
	case FURNACE_WIRE_MSG_START_REQUEST:
		io_msg_start_request(io, &m->u.start_request);
		return true;
	case FURNACE_WIRE_MSG_START_CONFIRM:
		io_msg_start_confirm(io, &m->u.start_confirm);
		return true;
	case FURNACE_WIRE_MSG_START_CANCEL:
		io_msg_start_cancel(io, &m->u.start_cancel);
		return true;
	case FURNACE_WIRE_MSG_START_PREPARE:
		io_msg_start_prepare(io, &m->u.start_prepare);
		return true;
	case FURNACE_WIRE_MSG_RUN_PAUSE:
		io_msg_run_pause(io, &m->u.run_pause);
		return true;
	case FURNACE_WIRE_MSG_RESUME_REQUEST:
		io_msg_resume_request(io, &m->u.resume_request);
		return true;
	case FURNACE_WIRE_MSG_RUN_STOP:
		io_msg_run_stop(io, &m->u.run_stop);
		return true;
	case FURNACE_WIRE_MSG_RUN_CONTINUE:
		io_msg_run_continue(io, &m->u.run_continue);
		return true;
	case FURNACE_WIRE_MSG_MANUAL_START:
		io_msg_manual_start(io, &m->u.manual_start);
		return true;
	case FURNACE_WIRE_MSG_MANUAL_RETARGET:
		io_msg_manual_retarget(io, &m->u.manual_retarget);
		return true;
	case FURNACE_WIRE_MSG_MANUAL_SET_LENGTH:
		io_msg_manual_set_length(io, &m->u.manual_set_length);
		return true;
	case FURNACE_WIRE_MSG_FAULT_ACKNOWLEDGE:
		io_msg_fault_acknowledge(io, &m->u.fault_acknowledge);
		return true;
	case FURNACE_WIRE_MSG_FAULT_RECOVERY_REQUEST:
		io_msg_fault_recovery_request(io, &m->u.fault_recovery_request);
		return true;
	case FURNACE_WIRE_MSG_PROGRAM_LIST_REQUEST:
		io_msg_program_list_request(io, &m->u.program_list_request);
		return true;
	case FURNACE_WIRE_MSG_PROGRAM_READ_REQUEST:
		io_msg_program_read_request(io, &m->u.program_read_request);
		return true;
	case FURNACE_WIRE_MSG_PROGRAM_SAVE:
		io_msg_program_save(io, &m->u.program_save);
		return true;
	case FURNACE_WIRE_MSG_PROGRAM_DELETE:
		io_msg_program_delete(io, &m->u.program_delete);
		return true;
	case FURNACE_WIRE_MSG_PROGRAM_ESTIMATE_REQUEST:
		io_msg_program_estimate_request(io, &m->u.program_estimate_request);
		return true;
	case FURNACE_WIRE_MSG_HEALTH_REQUEST:
		io_msg_health_request(io, &m->u.health_request);
		return true;
	case FURNACE_WIRE_MSG_EVENT_HISTORY_REQUEST:
		io_msg_event_history_request(io, &m->u.event_history_request);
		return true;
	case FURNACE_WIRE_MSG_HELLO_ACK:
		io_msg_hello_ack(io, &m->u.hello_ack);
		return true;
	case FURNACE_WIRE_MSG_COMMAND_REJECTED:
		io_msg_command_rejected(io, &m->u.command_rejected);
		return true;
	case FURNACE_WIRE_MSG_STATUS:
		io_msg_status(io, &m->u.status);
		return true;
	case FURNACE_WIRE_MSG_SNAPSHOT_END:
		io_msg_snapshot_end(io, &m->u.snapshot_end);
		return true;
	case FURNACE_WIRE_MSG_CAPABILITIES:
		io_msg_capabilities(io, &m->u.capabilities);
		return true;
	case FURNACE_WIRE_MSG_SETTINGS:
		io_msg_settings(io, &m->u.settings);
		return true;
	case FURNACE_WIRE_MSG_SETTING_RESULT:
		io_msg_setting_result(io, &m->u.setting_result);
		return true;
	case FURNACE_WIRE_MSG_START_ACCEPTED:
		io_msg_start_accepted(io, &m->u.start_accepted);
		return true;
	case FURNACE_WIRE_MSG_START_PROPOSAL:
		io_msg_start_proposal(io, &m->u.start_proposal);
		return true;
	case FURNACE_WIRE_MSG_START_PROPOSAL_INVALIDATED:
		io_msg_start_proposal_invalidated(io, &m->u.start_proposal_invalidated);
		return true;
	case FURNACE_WIRE_MSG_START_REJECTED:
		io_msg_start_rejected(io, &m->u.start_rejected);
		return true;
	case FURNACE_WIRE_MSG_START_CANCELLED:
		io_msg_start_cancelled(io, &m->u.start_cancelled);
		return true;
	case FURNACE_WIRE_MSG_START_PREPARE_RESULT:
		io_msg_start_prepare_result(io, &m->u.start_prepare_result);
		return true;
	case FURNACE_WIRE_MSG_RUN_COMMAND_ACCEPTED:
		io_msg_run_command_accepted(io, &m->u.run_command_accepted);
		return true;
	case FURNACE_WIRE_MSG_RUN_COMMAND_REJECTED:
		io_msg_run_command_rejected(io, &m->u.run_command_rejected);
		return true;
	case FURNACE_WIRE_MSG_RUN_EVENT:
		io_msg_run_event(io, &m->u.run_event);
		return true;
	case FURNACE_WIRE_MSG_FAULT_EVENT:
		io_msg_fault_event(io, &m->u.fault_event);
		return true;
	case FURNACE_WIRE_MSG_FAULT_ACKNOWLEDGE_RESULT:
		io_msg_fault_acknowledge_result(io, &m->u.fault_acknowledge_result);
		return true;
	case FURNACE_WIRE_MSG_FAULT_RECOVERY_RESULT:
		io_msg_fault_recovery_result(io, &m->u.fault_recovery_result);
		return true;
	case FURNACE_WIRE_MSG_FAULT_LIST:
		io_msg_fault_list(io, &m->u.fault_list);
		return true;
	case FURNACE_WIRE_MSG_PROGRAM_LIST:
		io_msg_program_list(io, &m->u.program_list);
		return true;
	case FURNACE_WIRE_MSG_PROGRAM_DATA:
		io_msg_program_data(io, &m->u.program_data);
		return true;
	case FURNACE_WIRE_MSG_PROGRAM_SAVE_RESULT:
		io_msg_program_save_result(io, &m->u.program_save_result);
		return true;
	case FURNACE_WIRE_MSG_PROGRAM_DELETE_RESULT:
		io_msg_program_delete_result(io, &m->u.program_delete_result);
		return true;
	case FURNACE_WIRE_MSG_PROGRAM_ESTIMATE:
		io_msg_program_estimate(io, &m->u.program_estimate);
		return true;
	case FURNACE_WIRE_MSG_HEALTH:
		io_msg_health(io, &m->u.health);
		return true;
	case FURNACE_WIRE_MSG_EVENT_HISTORY_PAGE:
		io_msg_event_history_page(io, &m->u.event_history_page);
		return true;
	default:
		return false;
	}
}

enum furnace_wire_result furnace_wire_payload_encode(const struct furnace_wire_message *msg,
						     uint8_t *out, size_t cap, size_t *out_len)
{
	struct wire_io io = {0};

	if ((msg == NULL) || (out == NULL) || (out_len == NULL)) {
		return FURNACE_WIRE_ERR_ARGUMENT;
	}
	io.buf = out;
	io.cap = (cap < FURNACE_WIRE_MAX_PAYLOAD) ? cap : FURNACE_WIRE_MAX_PAYLOAD;
	io.encoding = true;
	io.strict = furnace_wire_is_client_message(msg->type);
	/* Encoding only reads the message: the field functions serve both directions. */
	if (!io_message(&io, (struct furnace_wire_message *)(uintptr_t)msg)) {
		return FURNACE_WIRE_ERR_UNKNOWN_TYPE;
	}
	if (io.failed) {
		return FURNACE_WIRE_ERR_MALFORMED;
	}
	*out_len = io.pos;
	return FURNACE_WIRE_OK;
}

enum furnace_wire_result furnace_wire_payload_decode(uint16_t msg_type, const uint8_t *payload,
						     size_t len, struct furnace_wire_message *msg)
{
	struct wire_io io = {0};

	if ((msg == NULL) || ((payload == NULL) && (len != 0U))) {
		return FURNACE_WIRE_ERR_ARGUMENT;
	}
	memset(msg, 0, sizeof(*msg));
	msg->type = msg_type;
	/* Decoding only reads the buffer: the field functions serve both directions. */
	io.buf = (uint8_t *)(uintptr_t)payload;
	io.cap = len;
	io.strict = furnace_wire_is_client_message(msg_type);
	if (!io_message(&io, msg)) {
		memset(msg, 0, sizeof(*msg));
		return FURNACE_WIRE_ERR_UNKNOWN_TYPE;
	}
	if (io.failed || (io.strict && (io.pos != len))) {
		memset(msg, 0, sizeof(*msg));
		return FURNACE_WIRE_ERR_MALFORMED;
	}
	return FURNACE_WIRE_OK;
}

enum furnace_wire_result furnace_wire_envelope_decode(const uint8_t *payload, size_t len,
						      struct furnace_wire_envelope *envelope)
{
	struct wire_io io = {0};

	if ((envelope == NULL) || ((payload == NULL) && (len != 0U))) {
		return FURNACE_WIRE_ERR_ARGUMENT;
	}
	io.buf = (uint8_t *)(uintptr_t)payload;
	io.cap = len;
	io_envelope(&io, envelope);
	return io.failed ? FURNACE_WIRE_ERR_MALFORMED : FURNACE_WIRE_OK;
}

enum furnace_wire_result furnace_wire_tx_encode(struct furnace_wire_tx *tx,
						const struct furnace_wire_message *msg)
{
	enum furnace_wire_result result;
	size_t len = 0U;

	if ((tx == NULL) || (msg == NULL)) {
		return FURNACE_WIRE_ERR_ARGUMENT;
	}
	result = furnace_wire_payload_encode(msg, &tx->frame[FURNACE_WIRE_HEADER_SIZE],
					     FURNACE_WIRE_MAX_PAYLOAD, &len);
	if (result != FURNACE_WIRE_OK) {
		return result;
	}
	return furnace_wire_tx_finish(tx, msg->type, (uint16_t)len);
}
