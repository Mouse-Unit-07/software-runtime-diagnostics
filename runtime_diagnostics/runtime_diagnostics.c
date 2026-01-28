/*-------------------------------- FILE INFO ---------------------------------*/
/* Filename           : runtime_diagnostics.c                                 */
/*                                                                            */
/* An implementation of a simple log w/ a circular buffer                     */
/*                                                                            */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                               Include Files                                */
/*----------------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdio.h>
#include "runtime_diagnostics.h"

/*----------------------------------------------------------------------------*/
/*                                 Debug Space                                */
/*----------------------------------------------------------------------------*/
/* keep empty */

/*----------------------------------------------------------------------------*/
/*                               Private Globals                              */
/*----------------------------------------------------------------------------*/
struct log_entry telemetry_entries[TELEMETRY_LOG_SIZE] = {{0}};
struct log_entry warning_entries[WARNING_LOG_SIZE] = {{0}};
struct log_entry error_entries[ERROR_LOG_SIZE] = {{0}};

enum log_types_indices log_indices_array[LOG_TYPES_COUNT]
        = {TELEMETRY_INDEX, WARNING_INDEX, ERROR_INDEX};

struct circular_buffer telemetry_cb \
        = {telemetry_entries, TELEMETRY_LOG_SIZE, 0, 0};
struct circular_buffer warning_cb \
        = {warning_entries, WARNING_LOG_SIZE, 0, 0};
struct circular_buffer error_cb \
        = {error_entries, ERROR_LOG_SIZE, 0, 0};

struct circular_buffer *circular_buffer_array[LOG_TYPES_COUNT] \
        = { &telemetry_cb, &warning_cb, &error_cb };

volatile bool runtime_error_asserted = false;
struct log_entry first_runtime_error_cause = {0};
bool user_error_handler_set = false;
void (*user_error_handler)(void) = NULL;

/*----------------------------------------------------------------------------*/
/*                         Interrupt Service Routines                         */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                         Private Function Prototypes                        */
/*----------------------------------------------------------------------------*/
static void reset_circular_buffer(struct circular_buffer *cb)
{
    cb->head = 0;
    cb->count = 0;
}

static void reset_all_logs(void)
{
    runtime_error_asserted = false;
    user_error_handler_set = false;
    
    for (uint32_t i = 0; i < LOG_TYPES_COUNT; i++)
    {
        memset(circular_buffer_array[i]->log_entries, 0, sizeof(struct log_entry) * circular_buffer_array[i]->size);
        reset_circular_buffer(circular_buffer_array[i]);
    }
}

static void add_entry_to_log(enum log_types_indices log_index, 
    struct log_entry new_entry)
{
    struct circular_buffer *target_cb = circular_buffer_array[log_index];
    struct log_entry *target_entry = &(target_cb->log_entries[target_cb->head]);
    memcpy(target_entry, &new_entry, sizeof(new_entry));

    target_cb->head = (target_cb->head + 1) % target_cb->size;
    if (target_cb->count != target_cb->size) {
        target_cb->count++;
    }
}

static struct log_entry create_log_entry(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value)
{
    struct log_entry new_entry = {timestamp, fail_message, fail_value};
    return new_entry;
}

//private helper, but removing static to allow test access
struct log_entry get_entry_at_index(enum log_types_indices log_index, 
        uint32_t entry_index)
{
    struct circular_buffer *target_cb = circular_buffer_array[log_index];
    uint32_t oldest_entry_index = (target_cb->head + target_cb->size - target_cb->count) % target_cb->size;
    uint32_t return_entry_index = (oldest_entry_index + entry_index) % target_cb->size;
    return target_cb->log_entries[return_entry_index];
}

static void printf_log(enum log_types_indices log_index)
{
    for (uint32_t i = 0; i < circular_buffer_array[log_index]->count; i++) {
        struct log_entry entry = get_entry_at_index(log_index, i);
        printf("%" PRIu32 " %s %" PRIu32 "\r\n", 
                entry.timestamp, entry.fail_message, entry.fail_value);
    }
}

void set_error_flag_and_call_handler(void)
{
    runtime_error_asserted = true;

    if (user_error_handler_set) {
        user_error_handler();
    }
}

bool is_log_full(enum log_types_indices log_index)
{
    return circular_buffer_array[log_index]->size == circular_buffer_array[log_index]->count;
}

void save_first_runtime_error_cause(struct log_entry new_log)
{
    if (!runtime_error_asserted) {
        first_runtime_error_cause = new_log;
    }
}

/*----------------------------------------------------------------------------*/
/*                         Public Function Definitions                        */
/*----------------------------------------------------------------------------*/
void init_runtime_diagnostics()
{
    reset_all_logs();
}

void deinit_runtime_diagnostics()
{
    reset_all_logs();
}

void set_error_handler_function(void (*handler_function)(void))
{
    user_error_handler = handler_function;
    user_error_handler_set = true;
}

void RUNTIME_TELEMETRY(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value)
{
    add_entry_to_log(TELEMETRY_INDEX,
        create_log_entry(timestamp, fail_message, fail_value));
}

void RUNTIME_WARNING(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value)
{
    struct log_entry new_entry = create_log_entry(timestamp, fail_message, fail_value);
    add_entry_to_log(WARNING_INDEX, new_entry);

    if (is_log_full(WARNING_INDEX)) {
        save_first_runtime_error_cause(new_entry);
        set_error_flag_and_call_handler();
    }
}

void RUNTIME_ERROR(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value)
{
    struct log_entry new_entry = create_log_entry(timestamp, fail_message, fail_value);
    add_entry_to_log(ERROR_INDEX, new_entry);

    save_first_runtime_error_cause(new_entry);
    set_error_flag_and_call_handler();
}

void printf_telemetry_log(void)
{
    printf_log(TELEMETRY_INDEX);
}

void printf_warning_log(void)
{
    printf_log(WARNING_INDEX);
}

void printf_error_log(void)
{
    printf_log(ERROR_INDEX);
}

/*----------------------------------------------------------------------------*/
/*                        Private Function Definitions                        */
/*----------------------------------------------------------------------------*/
/* none */
