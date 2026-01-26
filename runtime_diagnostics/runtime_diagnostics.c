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
struct log_struct {
    struct log_entry *log_entries;
    uint32_t size;
    uint32_t head;
    uint32_t tail;
    uint32_t count;
};

static struct log_entry telemetry_entries[TELEMETRY_LOG_SIZE] = {{0}};
static struct log_entry warning_entries[WARNING_LOG_SIZE] = {{0}};
static struct log_entry error_entries[ERROR_LOG_SIZE] = {{0}};

static struct log_struct telemetry_struct \
        = { telemetry_entries, TELEMETRY_LOG_SIZE, 0, 0, 0 };
static struct log_struct warning_struct \
        = { warning_entries, WARNING_LOG_SIZE, 0, 0, 0 };
static struct log_struct error_struct \
        = { error_entries, ERROR_LOG_SIZE, 0, 0, 0 };

enum log_struct_array_index
{
    TELEMETRY_LOG_INDEX = 0,
    WARNING_LOG_INDEX,
    ERROR_LOG_INDEX,
    LOG_STRUCT_ARRAY_SIZE
};

static struct log_struct *log_struct_array[LOG_STRUCT_ARRAY_SIZE];


/*----------------------------------------------------------------------------*/
/*                         Interrupt Service Routines                         */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                         Private Function Prototypes                        */
/*----------------------------------------------------------------------------*/
static void reset_log_struct(struct log_struct *input_log_struct)
{
    input_log_struct->head = 0;
    input_log_struct->tail = 0;
    input_log_struct->count = 0;
}

static void reset_all_logs(void)
{
    memset(telemetry_entries, 0, sizeof(telemetry_entries));
    memset(warning_entries, 0, sizeof(warning_entries));
    memset(error_entries, 0, sizeof(error_entries));

    reset_log_struct(&telemetry_struct);
    reset_log_struct(&warning_struct);
    reset_log_struct(&error_struct);
}

static void add_entry_to_log(enum log_struct_array_index log_index, 
    struct log_entry new_entry)
{
    struct log_struct *target_struct = log_struct_array[log_index];
    struct log_entry *target_entry = &(target_struct->log_entries[target_struct->tail]);
    memcpy(target_entry, &new_entry, sizeof(new_entry));

    target_struct->tail = (target_struct->tail + 1) % target_struct->size;
    target_struct->count++;
}

/*----------------------------------------------------------------------------*/
/*                         Public Function Definitions                        */
/*----------------------------------------------------------------------------*/
void init_runtime_diagnostics()
{
    log_struct_array[TELEMETRY_LOG_INDEX] = &telemetry_struct;
    log_struct_array[WARNING_LOG_INDEX] = &warning_struct;
    log_struct_array[ERROR_LOG_INDEX] = &error_struct;
    reset_all_logs();
}

void deinit_runtime_diagnostics()
{
    reset_all_logs();
}

struct log_entry *get_telemetry_log(void)
{
    return telemetry_entries;
}

struct log_entry *get_warning_log(void)
{
    return warning_entries;
}

struct log_entry *get_error_log(void)
{
    return error_entries;
}

struct log_entry create_log_entry(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value)
{
    struct log_entry new_entry = {timestamp, fail_message, fail_value};
    return new_entry;
}

void RUNTIME_TELEMETRY(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value)
{
    add_entry_to_log(TELEMETRY_LOG_INDEX,
        create_log_entry(timestamp, fail_message, fail_value));
}

void RUNTIME_WARNING(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value)
{
    add_entry_to_log(WARNING_LOG_INDEX,
        create_log_entry(timestamp, fail_message, fail_value));
}

void RUNTIME_ERROR(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value)
{
    add_entry_to_log(ERROR_LOG_INDEX,
        create_log_entry(timestamp, fail_message, fail_value));
}

/*----------------------------------------------------------------------------*/
/*                        Private Function Definitions                        */
/*----------------------------------------------------------------------------*/
/* none */
