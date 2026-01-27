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
uint32_t log_sizes_array[LOG_TYPES_COUNT]
        = {TELEMETRY_LOG_SIZE, WARNING_LOG_SIZE, ERROR_LOG_SIZE};
enum log_types_indices log_indices_array[LOG_TYPES_COUNT]
        = {TELEMETRY_INDEX, WARNING_INDEX, ERROR_INDEX};

struct circular_buffer telemetry_cb \
        = {telemetry_entries, TELEMETRY_LOG_SIZE, 0, 0, 0};
struct circular_buffer warning_cb \
        = {warning_entries, WARNING_LOG_SIZE, 0, 0, 0};
struct circular_buffer error_cb \
        = { error_entries, ERROR_LOG_SIZE, 0, 0, 0 };

struct circular_buffer *circular_buffer_array[LOG_TYPES_COUNT] \
        = { &telemetry_cb, &warning_cb, &error_cb };

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
    cb->tail = 0;
    cb->count = 0;
}

static void reset_all_logs(void)
{
    for (uint32_t i = 0; i < LOG_TYPES_COUNT; i++)
    {
        memset(circular_buffer_array[i]->log_entries, 0, sizeof(struct log_entry) * log_sizes_array[i]);
        reset_circular_buffer(circular_buffer_array[i]);
    }
}

static void add_entry_to_log(enum log_types_indices log_index, 
    struct log_entry new_entry)
{
    struct circular_buffer *target_struct = circular_buffer_array[log_index];
    struct log_entry *target_entry = &(target_struct->log_entries[target_struct->tail]);
    memcpy(target_entry, &new_entry, sizeof(new_entry));

    target_struct->tail = (target_struct->tail + 1) % target_struct->size;
    target_struct->count++;
}

static struct log_entry create_log_entry(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value)
{
    struct log_entry new_entry = {timestamp, fail_message, fail_value};
    return new_entry;
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

void RUNTIME_TELEMETRY(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value)
{
    add_entry_to_log(TELEMETRY_INDEX,
        create_log_entry(timestamp, fail_message, fail_value));
}

void RUNTIME_WARNING(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value)
{
    add_entry_to_log(WARNING_INDEX,
        create_log_entry(timestamp, fail_message, fail_value));
}

void RUNTIME_ERROR(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value)
{
    add_entry_to_log(ERROR_INDEX,
        create_log_entry(timestamp, fail_message, fail_value));
}

/*----------------------------------------------------------------------------*/
/*                        Private Function Definitions                        */
/*----------------------------------------------------------------------------*/
/* none */
