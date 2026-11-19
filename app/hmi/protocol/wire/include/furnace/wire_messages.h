/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Decoded messages of the controller client protocol (module core/wire, Phase
 * 20b). Pure ISO C17: no Zephyr, no heap, no global state. The structures hold
 * the semantic values of docs/interfaces/controller-wire-format.md §7, whose
 * tables are the only definition of the bytes; field names and order are the
 * same. They are never copied to the wire (COMM-04): one function per message
 * in wire_messages.c moves each field by its explicit encoding.
 *
 * Enumerations are raw numeric codes, because a client keeps a value it does
 * not know as unknown (wire format §5). A `utf8[N]` field is NUL-terminated
 * here and has no terminator on the wire. An `array[N]` field has a `_count`
 * and an `opt` group a `_present`. Requirements: COMM-04, API-02. Decision:
 * ADR-0018.
 */

#ifndef FURNACE_WIRE_MESSAGES_H_
#define FURNACE_WIRE_MESSAGES_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <furnace/wire.h>

/* Message types (wire format §6). 0x0024 is reserved and never reused (Q18). */
enum furnace_wire_msg_type {
	FURNACE_WIRE_MSG_HELLO = 0x0001,
	FURNACE_WIRE_MSG_GOODBYE = 0x0002,
	FURNACE_WIRE_MSG_HEARTBEAT = 0x0003,
	FURNACE_WIRE_MSG_SNAPSHOT_REQUEST = 0x0004,
	FURNACE_WIRE_MSG_CAPABILITIES_REQUEST = 0x0005,
	FURNACE_WIRE_MSG_SETTINGS_REQUEST = 0x0006,
	FURNACE_WIRE_MSG_SETTING_CHANGE = 0x0007,
	FURNACE_WIRE_MSG_START_REQUEST = 0x0010,
	FURNACE_WIRE_MSG_START_CONFIRM = 0x0011,
	FURNACE_WIRE_MSG_START_CANCEL = 0x0012,
	FURNACE_WIRE_MSG_START_PREPARE = 0x0013,
	FURNACE_WIRE_MSG_RUN_PAUSE = 0x0020,
	FURNACE_WIRE_MSG_RESUME_REQUEST = 0x0021,
	FURNACE_WIRE_MSG_RUN_STOP = 0x0022,
	FURNACE_WIRE_MSG_RUN_CONTINUE = 0x0023,
	FURNACE_WIRE_MSG_MANUAL_START = 0x0030,
	FURNACE_WIRE_MSG_MANUAL_RETARGET = 0x0031,
	FURNACE_WIRE_MSG_MANUAL_SET_LENGTH = 0x0032,
	FURNACE_WIRE_MSG_FAULT_ACKNOWLEDGE = 0x0040,
	FURNACE_WIRE_MSG_FAULT_RECOVERY_REQUEST = 0x0041,
	FURNACE_WIRE_MSG_PROGRAM_LIST_REQUEST = 0x0050,
	FURNACE_WIRE_MSG_PROGRAM_READ_REQUEST = 0x0051,
	FURNACE_WIRE_MSG_PROGRAM_SAVE = 0x0052,
	FURNACE_WIRE_MSG_PROGRAM_DELETE = 0x0053,
	FURNACE_WIRE_MSG_PROGRAM_ESTIMATE_REQUEST = 0x0054,
	FURNACE_WIRE_MSG_HEALTH_REQUEST = 0x0060,
	FURNACE_WIRE_MSG_EVENT_HISTORY_REQUEST = 0x0061,
	FURNACE_WIRE_MSG_HELLO_ACK = 0x8001,
	FURNACE_WIRE_MSG_COMMAND_REJECTED = 0x8002,
	FURNACE_WIRE_MSG_STATUS = 0x8003,
	FURNACE_WIRE_MSG_SNAPSHOT_END = 0x8004,
	FURNACE_WIRE_MSG_CAPABILITIES = 0x8005,
	FURNACE_WIRE_MSG_SETTINGS = 0x8006,
	FURNACE_WIRE_MSG_SETTING_RESULT = 0x8007,
	FURNACE_WIRE_MSG_START_ACCEPTED = 0x8010,
	FURNACE_WIRE_MSG_START_PROPOSAL = 0x8011,
	FURNACE_WIRE_MSG_START_PROPOSAL_INVALIDATED = 0x8012,
	FURNACE_WIRE_MSG_START_REJECTED = 0x8013,
	FURNACE_WIRE_MSG_START_CANCELLED = 0x8014,
	FURNACE_WIRE_MSG_START_PREPARE_RESULT = 0x8015,
	FURNACE_WIRE_MSG_RUN_COMMAND_ACCEPTED = 0x8020,
	FURNACE_WIRE_MSG_RUN_COMMAND_REJECTED = 0x8021,
	FURNACE_WIRE_MSG_RUN_EVENT = 0x8022,
	FURNACE_WIRE_MSG_FAULT_EVENT = 0x8040,
	FURNACE_WIRE_MSG_FAULT_ACKNOWLEDGE_RESULT = 0x8041,
	FURNACE_WIRE_MSG_FAULT_RECOVERY_RESULT = 0x8042,
	FURNACE_WIRE_MSG_FAULT_LIST = 0x8043,
	FURNACE_WIRE_MSG_PROGRAM_LIST = 0x8050,
	FURNACE_WIRE_MSG_PROGRAM_DATA = 0x8051,
	FURNACE_WIRE_MSG_PROGRAM_SAVE_RESULT = 0x8052,
	FURNACE_WIRE_MSG_PROGRAM_DELETE_RESULT = 0x8053,
	FURNACE_WIRE_MSG_PROGRAM_ESTIMATE = 0x8054,
	FURNACE_WIRE_MSG_HEALTH = 0x8060,
	FURNACE_WIRE_MSG_EVENT_HISTORY_PAGE = 0x8061,
};

/* The `measured` primitive (wire format §4). Validity 0 is never valid. */
struct furnace_wire_measured {
	int32_t value;
	uint8_t validity;
	uint32_t age_ms;
};

struct furnace_wire_envelope {
	uint8_t session_id[FURNACE_WIRE_SESSION_ID_SIZE];
	uint32_t client_session_id;
	uint32_t request_id;
};

struct furnace_wire_stage {
	uint8_t kind;
	int32_t target;
	int32_t rate;
	uint32_t duration_minutes;
};

struct furnace_wire_skipped_stage {
	uint8_t index;
	uint8_t kind;
	int32_t target;
	uint32_t hold_minutes;
};

struct furnace_wire_fault_entry {
	uint8_t fault_id;
	uint8_t source;
	uint8_t class;
	bool condition_active;
	bool latched;
	bool acknowledged;
	uint8_t recovery;
	uint32_t occurrences;
	uint64_t first_at;
	uint64_t last_change;
	int64_t value;
	uint8_t category;
	bool continuable;
	bool continued;
	uint32_t first_boot;
	uint32_t last_boot;
};

struct furnace_wire_library_entry {
	uint32_t program_id;
	uint32_t program_revision;
	char name[125];
	uint8_t stage_count;
	int32_t max_target;
	uint32_t curing_estimate_s;
	int32_t planning_reference;
	bool damaged;
};

struct furnace_wire_health_subsystem {
	uint8_t subsystem;
	uint8_t state;
	uint32_t detail;
};

struct furnace_wire_event_param {
	uint8_t type;
	uint32_t value;
};

struct furnace_wire_history_entry {
	uint32_t log_sequence;
	uint32_t boot_id;
	uint32_t sequence;
	uint64_t mono_ms;
	uint8_t wall_state;
	int64_t wall_ms;
	uint16_t event_id;
	uint8_t severity;
	uint8_t category;
	uint8_t params_count;
	struct furnace_wire_event_param params[8U];
};

struct furnace_wire_fault_status {
	bool heating_inhibited;
	bool resume_allowed;
	uint8_t latched_class;
	uint32_t inhibit_ids;
	uint32_t continuable_ids;
	uint8_t unacknowledged_count;
	uint32_t last_sequence;
	uint32_t history_dropped;
};

struct furnace_wire_start_status {
	uint32_t transaction_id;
	uint32_t proposal_revision;
	uint8_t state;
	uint8_t invalidation_reason;
};

struct furnace_wire_previous_run {
	uint32_t run_id;
	uint32_t program_id;
	uint32_t program_revision;
	char program_name[125];
	uint8_t session_kind;
	uint8_t outcome;
	uint8_t termination;
	uint32_t review_flags;
	uint8_t cooling;
	uint8_t cooling_ended_by;
	uint32_t cooling_s;
};

struct furnace_wire_run_status {
	uint8_t run_state;
	uint8_t fault_class;
	uint32_t run_id;
	uint8_t session_kind;
	uint32_t program_id;
	uint32_t program_revision;
	char program_name[125];
	uint8_t starting_stage;
	int32_t start_temperature;
	uint8_t stage_index;
	uint8_t stage_count;
	uint8_t stage_kind;
	bool setpoint_valid;
	int32_t setpoint;
	bool setpoint_approaching;
	bool setpoint_moving;
	struct furnace_wire_measured process_temperature;
	uint8_t process_role;
	bool heating_requested;
	uint32_t pause_reasons;
	uint32_t blocking_conditions;
	uint32_t decision_reasons;
	uint32_t decision_remaining_s;
	uint32_t run_elapsed_s;
	uint32_t paused_s;
	uint32_t run_nominal_s;
	uint32_t accepted_s;
	uint32_t running_s;
	uint32_t runtime_limit_s;
	uint32_t measurement_qualified_s;
	uint32_t hold_undershoot_s;
	uint8_t cooling;
	uint32_t cooling_s;
	bool program_running;
	int32_t manual_target;
	int32_t manual_rate;
	uint32_t manual_remaining_s;
	uint8_t termination;
	uint32_t review_flags;
	uint8_t outcome;
	uint8_t curing_acceptance;
	struct furnace_wire_previous_run previous_run;
};

struct furnace_wire_ramp_progress {
	uint8_t ramp_phase;
	int32_t tracking_deviation;
	uint32_t holdback_s;
	uint32_t holdback_total_s;
	uint32_t holdback_count;
	uint32_t stage_elapsed_s;
	uint32_t ramp_nominal_s;
	uint32_t ramp_delay_s;
	uint32_t target_wait_s;
};

struct furnace_wire_hold_progress {
	uint8_t hold_phase;
	uint8_t hold_condition;
	uint32_t stage_elapsed_s;
	uint32_t qualification_elapsed_s;
	uint32_t treatment_credited_s;
	uint32_t treatment_remaining_s;
	uint32_t interruption_count;
	uint32_t uncredited_after_qualification_s;
	int32_t max_deviation;
	uint32_t excursion_count;
	uint32_t excursion_s;
};

struct furnace_wire_control_diagnostics {
	uint16_t heating_power_request;
	int32_t control_error;
	uint32_t control_flags;
};

struct furnace_wire_heat_monitor {
	uint8_t mode;
	bool observed;
	bool above;
	bool suspected;
	bool condition;
	bool latched;
	uint8_t basis;
	bool reference_valid;
	int32_t reference;
	int32_t ceiling;
	int32_t rise;
	uint32_t evidence_s;
	uint32_t counter_s;
	uint32_t observed_s;
	uint32_t unresolved_s;
	uint32_t gap_s;
	uint32_t gap_total_s;
	bool unobservable;
	uint32_t suspicions;
	uint32_t confirmations;
	uint32_t escalations;
};

struct furnace_wire_controller {
	bool configured;
	uint8_t config_state;
	uint8_t storage;
	bool log_persistent;
	bool session_fresh;
	uint8_t store_readiness;
};

struct furnace_wire_lifecycle {
	bool execution_active;
	bool heating_authorized;
	bool navigation_permitted;
	bool library_mutations_permitted;
	bool settings_changes_permitted;
};

struct furnace_wire_equipment {
	uint8_t fan_command;
	uint8_t fan_reason;
	uint8_t fan_feedback;
	uint8_t door_state;
	uint8_t measurement_state;
	bool emergency_active;
	bool warning_active;
	uint8_t warning_output;
};

struct furnace_wire_hello {
	uint32_t client_session_id;
	uint8_t client_kind;
};

struct furnace_wire_goodbye {
	struct furnace_wire_envelope envelope;
};

struct furnace_wire_heartbeat {
	uint32_t client_session_id;
};

struct furnace_wire_snapshot_request {
	struct furnace_wire_envelope envelope;
};

struct furnace_wire_capabilities_request {
	struct furnace_wire_envelope envelope;
};

struct furnace_wire_settings_request {
	struct furnace_wire_envelope envelope;
};

struct furnace_wire_setting_change {
	struct furnace_wire_envelope envelope;
	uint8_t setting;
	int32_t value;
};

struct furnace_wire_start_request {
	struct furnace_wire_envelope envelope;
	uint32_t program_id;
	uint32_t program_revision;
};

struct furnace_wire_start_confirm {
	struct furnace_wire_envelope envelope;
	uint32_t transaction_id;
	uint32_t proposal_revision;
	uint32_t program_id;
	uint32_t program_revision;
	uint8_t proposed_stage;
	uint32_t accepted_duration_s;
};

struct furnace_wire_start_cancel {
	struct furnace_wire_envelope envelope;
	uint32_t transaction_id;
};

struct furnace_wire_start_prepare {
	struct furnace_wire_envelope envelope;
};

struct furnace_wire_run_pause {
	struct furnace_wire_envelope envelope;
	uint32_t run_id;
};

struct furnace_wire_resume_request {
	struct furnace_wire_envelope envelope;
	uint32_t run_id;
};

struct furnace_wire_run_stop {
	struct furnace_wire_envelope envelope;
	uint32_t run_id;
};

struct furnace_wire_run_continue {
	struct furnace_wire_envelope envelope;
	uint32_t run_id;
};

struct furnace_wire_manual_start {
	struct furnace_wire_envelope envelope;
	int32_t target;
	int32_t rate;
	uint32_t session_minutes;
};

struct furnace_wire_manual_retarget {
	struct furnace_wire_envelope envelope;
	uint32_t run_id;
	int32_t target;
	int32_t rate;
};

struct furnace_wire_manual_set_length {
	struct furnace_wire_envelope envelope;
	uint32_t run_id;
	uint32_t session_minutes;
};

struct furnace_wire_fault_acknowledge {
	struct furnace_wire_envelope envelope;
	uint8_t fault_id;
	uint8_t source;
};

struct furnace_wire_fault_recovery_request {
	struct furnace_wire_envelope envelope;
	uint8_t fault_id;
	uint8_t source;
};

struct furnace_wire_program_list_request {
	struct furnace_wire_envelope envelope;
	uint8_t first_index;
	uint8_t max_count;
};

struct furnace_wire_program_read_request {
	struct furnace_wire_envelope envelope;
	uint32_t program_id;
};

struct furnace_wire_program_save {
	struct furnace_wire_envelope envelope;
	uint32_t program_id;
	uint32_t base_revision;
	uint32_t acknowledged_duration_s;
	char name[125];
	uint8_t stages_count;
	struct furnace_wire_stage stages[18U];
};

struct furnace_wire_program_delete {
	struct furnace_wire_envelope envelope;
	uint32_t program_id;
	uint32_t program_revision;
};

struct furnace_wire_program_estimate_request {
	struct furnace_wire_envelope envelope;
	uint32_t program_id;
	uint32_t program_revision;
	uint8_t basis;
};

struct furnace_wire_health_request {
	struct furnace_wire_envelope envelope;
};

struct furnace_wire_event_history_request {
	struct furnace_wire_envelope envelope;
	uint8_t log;
	uint32_t from_sequence;
	uint8_t max_entries;
	uint32_t categories;
	uint8_t min_severity;
	uint32_t boot_id;
	uint64_t from_ms;
	uint64_t to_ms;
};

struct furnace_wire_hello_ack {
	uint8_t result;
	uint8_t session_id[FURNACE_WIRE_SESSION_ID_SIZE];
	uint32_t firmware_id;
	uint32_t boot_count;
};

struct furnace_wire_command_rejected {
	uint32_t request_id;
	uint16_t msg_type;
	uint8_t reason;
};

struct furnace_wire_status {
	uint8_t session_id[FURNACE_WIRE_SESSION_ID_SIZE];
	uint32_t status_sequence;
	uint32_t events_dropped;
	uint32_t library_revision;
	struct furnace_wire_measured measured_temperature;
	uint8_t operating_mode;
	uint8_t restrictions_count;
	uint8_t restrictions[8U];
	struct furnace_wire_fault_status fault_status;
	struct furnace_wire_start_status start;
	struct furnace_wire_run_status run_status;
	bool ramp_progress_present;
	struct furnace_wire_ramp_progress ramp_progress;
	bool hold_progress_present;
	struct furnace_wire_hold_progress hold_progress;
	bool control_diagnostics_present;
	struct furnace_wire_control_diagnostics control_diagnostics;
	struct furnace_wire_heat_monitor heat_monitor;
	struct furnace_wire_controller controller;
	struct furnace_wire_lifecycle lifecycle;
	struct furnace_wire_equipment equipment;
};

struct furnace_wire_snapshot_end {
	uint32_t request_id;
	uint32_t status_sequence;
	uint32_t fault_sequence;
	uint32_t run_event_sequence;
	uint32_t library_revision;
};

struct furnace_wire_capabilities {
	uint32_t request_id;
	uint8_t session_id[FURNACE_WIRE_SESSION_ID_SIZE];
	uint8_t proto_major;
	uint8_t proto_minor;
	uint32_t firmware_id;
	bool configured;
	uint8_t config_state;
	char model_id[32];
	uint32_t config_revision;
	uint32_t profile_revision;
	uint8_t max_stages;
	uint32_t stage_kinds;
	int32_t target_min;
	int32_t target_max;
	int32_t rate_min;
	int32_t rate_max;
	uint32_t curing_max_minutes;
	uint32_t recommended_curing_minutes;
	int32_t planning_reference_min;
	int32_t planning_reference_max;
	uint8_t name_max_code_points;
	uint8_t name_max_bytes;
	uint32_t manual_max_minutes;
	uint8_t library_capacity;
	uint16_t max_payload;
	uint32_t status_interval_ms;
	uint32_t heartbeat_interval_ms;
	uint32_t link_silence_ms;
};

struct furnace_wire_settings {
	uint32_t request_id;
	uint8_t settings_state;
	int32_t planning_reference;
};

struct furnace_wire_setting_result {
	uint32_t request_id;
	uint8_t setting;
	uint8_t result;
	int32_t value;
};

struct furnace_wire_start_accepted {
	uint32_t request_id;
	uint32_t transaction_id;
	uint32_t run_id;
	uint8_t starting_stage;
	uint32_t accepted_duration_s;
};

struct furnace_wire_start_proposal {
	uint32_t request_id;
	uint32_t transaction_id;
	uint32_t proposal_revision;
	struct furnace_wire_measured measured_temperature;
	uint8_t proposed_stage;
	uint8_t skipped_stages_count;
	struct furnace_wire_skipped_stage skipped_stages[18U];
	uint32_t remaining_curing_s;
	bool duration_ack_required;
	uint32_t recommended_s;
	uint32_t maximum_s;
	uint8_t warnings_count;
	uint8_t warnings[8U];
};

struct furnace_wire_start_proposal_invalidated {
	uint32_t transaction_id;
	uint8_t reason;
};

struct furnace_wire_start_rejected {
	uint32_t request_id;
	uint8_t reason;
};

struct furnace_wire_start_cancelled {
	uint32_t request_id;
	uint32_t transaction_id;
};

struct furnace_wire_start_prepare_result {
	uint32_t request_id;
	uint8_t store_readiness;
};

struct furnace_wire_run_command_accepted {
	uint32_t request_id;
	uint8_t command;
	uint8_t reason;
	uint32_t blocking_conditions;
	uint32_t run_id;
};

struct furnace_wire_run_command_rejected {
	uint32_t request_id;
	uint8_t command;
	uint8_t reason;
	uint32_t blocking_conditions;
	uint32_t run_id;
};

struct furnace_wire_run_event {
	uint32_t sequence;
	uint32_t run_id;
	uint8_t event;
	uint8_t stage;
	int64_t value;
};

struct furnace_wire_fault_event {
	uint32_t sequence;
	uint64_t at;
	uint8_t fault_id;
	uint8_t source;
	uint8_t class;
	uint8_t transition;
	uint8_t consequence;
	uint8_t controller_state;
	int64_t value;
	bool time_regressed;
	uint32_t boot;
};

struct furnace_wire_fault_acknowledge_result {
	uint32_t request_id;
	uint8_t fault_id;
	uint8_t source;
	uint8_t result;
};

struct furnace_wire_fault_recovery_result {
	uint32_t request_id;
	uint8_t fault_id;
	uint8_t source;
	bool found;
	uint8_t reply;
};

struct furnace_wire_fault_list {
	uint32_t request_id;
	uint32_t status_sequence;
	uint8_t total;
	uint8_t first_index;
	uint8_t entries_count;
	struct furnace_wire_fault_entry entries[16U];
};

struct furnace_wire_program_list {
	uint32_t request_id;
	uint8_t result;
	uint32_t library_revision;
	uint8_t total;
	uint8_t first_index;
	uint8_t entries_count;
	struct furnace_wire_library_entry entries[6U];
};

struct furnace_wire_program_data {
	uint32_t request_id;
	uint8_t result;
	uint32_t program_id;
	uint32_t program_revision;
	char name[125];
	uint32_t curing_estimate_s;
	int32_t planning_reference;
	uint8_t stages_count;
	struct furnace_wire_stage stages[18U];
};

struct furnace_wire_program_save_result {
	uint32_t request_id;
	uint8_t result;
	uint32_t program_id;
	uint32_t program_revision;
	uint8_t error_reason;
	uint8_t error_field;
	uint8_t error_stage;
	uint32_t curing_estimate_s;
	int32_t planning_reference;
};

struct furnace_wire_program_delete_result {
	uint32_t request_id;
	uint8_t result;
	uint32_t program_id;
};

struct furnace_wire_program_estimate {
	uint32_t request_id;
	uint8_t result;
	uint32_t program_id;
	uint32_t program_revision;
	uint8_t basis;
	int32_t start_temperature;
	uint8_t start_stage;
	uint32_t curing_estimate_s;
	bool exceeds_recommended;
	bool above_maximum;
};

struct furnace_wire_health {
	uint32_t request_id;
	uint8_t session_id[FURNACE_WIRE_SESSION_ID_SIZE];
	uint64_t uptime_ms;
	uint32_t boot_count;
	uint8_t overall;
	bool reset_cause_known;
	uint32_t reset_cause;
	uint8_t subsystems_count;
	struct furnace_wire_health_subsystem subsystems[10U];
	uint8_t faults_active;
	uint8_t faults_latched;
	uint8_t faults_unacknowledged;
	uint32_t service_s;
	bool service_valid;
};

struct furnace_wire_event_history_page {
	uint32_t request_id;
	uint8_t result;
	uint8_t log;
	uint32_t first_retained;
	uint32_t next_sequence;
	uint32_t evicted;
	uint32_t scanned_to;
	bool more;
	uint8_t entries_count;
	struct furnace_wire_history_entry entries[12U];
};

/* One decoded message; type selects the member. */
struct furnace_wire_message {
	uint16_t type;
	union {
		struct furnace_wire_hello hello;
		struct furnace_wire_goodbye goodbye;
		struct furnace_wire_heartbeat heartbeat;
		struct furnace_wire_snapshot_request snapshot_request;
		struct furnace_wire_capabilities_request capabilities_request;
		struct furnace_wire_settings_request settings_request;
		struct furnace_wire_setting_change setting_change;
		struct furnace_wire_start_request start_request;
		struct furnace_wire_start_confirm start_confirm;
		struct furnace_wire_start_cancel start_cancel;
		struct furnace_wire_start_prepare start_prepare;
		struct furnace_wire_run_pause run_pause;
		struct furnace_wire_resume_request resume_request;
		struct furnace_wire_run_stop run_stop;
		struct furnace_wire_run_continue run_continue;
		struct furnace_wire_manual_start manual_start;
		struct furnace_wire_manual_retarget manual_retarget;
		struct furnace_wire_manual_set_length manual_set_length;
		struct furnace_wire_fault_acknowledge fault_acknowledge;
		struct furnace_wire_fault_recovery_request fault_recovery_request;
		struct furnace_wire_program_list_request program_list_request;
		struct furnace_wire_program_read_request program_read_request;
		struct furnace_wire_program_save program_save;
		struct furnace_wire_program_delete program_delete;
		struct furnace_wire_program_estimate_request program_estimate_request;
		struct furnace_wire_health_request health_request;
		struct furnace_wire_event_history_request event_history_request;
		struct furnace_wire_hello_ack hello_ack;
		struct furnace_wire_command_rejected command_rejected;
		struct furnace_wire_status status;
		struct furnace_wire_snapshot_end snapshot_end;
		struct furnace_wire_capabilities capabilities;
		struct furnace_wire_settings settings;
		struct furnace_wire_setting_result setting_result;
		struct furnace_wire_start_accepted start_accepted;
		struct furnace_wire_start_proposal start_proposal;
		struct furnace_wire_start_proposal_invalidated start_proposal_invalidated;
		struct furnace_wire_start_rejected start_rejected;
		struct furnace_wire_start_cancelled start_cancelled;
		struct furnace_wire_start_prepare_result start_prepare_result;
		struct furnace_wire_run_command_accepted run_command_accepted;
		struct furnace_wire_run_command_rejected run_command_rejected;
		struct furnace_wire_run_event run_event;
		struct furnace_wire_fault_event fault_event;
		struct furnace_wire_fault_acknowledge_result fault_acknowledge_result;
		struct furnace_wire_fault_recovery_result fault_recovery_result;
		struct furnace_wire_fault_list fault_list;
		struct furnace_wire_program_list program_list;
		struct furnace_wire_program_data program_data;
		struct furnace_wire_program_save_result program_save_result;
		struct furnace_wire_program_delete_result program_delete_result;
		struct furnace_wire_program_estimate program_estimate;
		struct furnace_wire_health health;
		struct furnace_wire_event_history_page event_history_page;
	} u;
};

/**
 * @brief Encode a message's payload.
 *
 * Refuses what the receiver would refuse: a value outside its encoding, a count
 * or string above its bound, invalid UTF-8, and, in a client message, an
 * undefined enumeration value or bit.
 *
 * @param msg     Message; type selects the layout.
 * @param out     Output buffer.
 * @param cap     Its size; at most FURNACE_WIRE_MAX_PAYLOAD is used.
 * @param out_len Bytes written.
 * @retval FURNACE_WIRE_OK Written.
 * @retval FURNACE_WIRE_ERR_ARGUMENT A NULL argument.
 * @retval FURNACE_WIRE_ERR_UNKNOWN_TYPE No layout for this type.
 * @retval FURNACE_WIRE_ERR_MALFORMED A field cannot be encoded, or it does not fit.
 */
enum furnace_wire_result furnace_wire_payload_encode(const struct furnace_wire_message *msg,
						     uint8_t *out, size_t cap, size_t *out_len);

/**
 * @brief Decode a payload (wire format §3 steps 6 and 9, §5).
 *
 * A client message (bit 15 clear) must have exactly its layout's length and only
 * defined values. A controller message may be longer, its extra bytes ignored,
 * and keeps unknown values. Never reads outside the payload. After a failure
 * the message is zero, with no partial content.
 *
 * @param msg_type Type from the frame header.
 * @param payload  Payload bytes; may be NULL when len is 0.
 * @param len      Their number.
 * @param msg      Result.
 * @retval FURNACE_WIRE_OK Decoded.
 * @retval FURNACE_WIRE_ERR_ARGUMENT A NULL argument.
 * @retval FURNACE_WIRE_ERR_UNKNOWN_TYPE No layout for this type, also a reserved one.
 * @retval FURNACE_WIRE_ERR_MALFORMED The bytes do not fit the layout.
 */
enum furnace_wire_result furnace_wire_payload_decode(uint16_t msg_type, const uint8_t *payload,
						     size_t len, struct furnace_wire_message *msg);

/**
 * @brief Decode only the command envelope (wire format §3 step 7).
 *
 * The service checks the envelope before it looks at the type or the rest of the
 * payload. Longer payloads are fine here.
 *
 * @retval FURNACE_WIRE_OK Decoded.
 * @retval FURNACE_WIRE_ERR_ARGUMENT A NULL argument.
 * @retval FURNACE_WIRE_ERR_MALFORMED Too short for an envelope: drop, no reply.
 */
enum furnace_wire_result furnace_wire_envelope_decode(const uint8_t *payload, size_t len,
						      struct furnace_wire_envelope *envelope);

/**
 * @brief Encode a message into the transmit buffers, ready for the transport.
 *
 * @retval FURNACE_WIRE_OK tx->encoded holds tx->encoded_len bytes, delimiter included.
 * @return The result of furnace_wire_payload_encode() otherwise.
 */
enum furnace_wire_result furnace_wire_tx_encode(struct furnace_wire_tx *tx,
						const struct furnace_wire_message *msg);

#endif /* FURNACE_WIRE_MESSAGES_H_ */
