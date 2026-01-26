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
static struct log_entry telemetry_log[TELEMETRY_LOG_SIZE] = {{0}};
static struct log_entry warning_log[WARNING_LOG_SIZE] = {{0}};
static struct log_entry error_log[ERROR_LOG_SIZE] = {{0}};

enum log_array_index
{
    TELEMETRY_LOG_INDEX = 0,
    WARNING_LOG_INDEX = 1,
    ERROR_LOG_INDEX = 2,
    LOG_ARRAY_SIZE = 3
};

static struct log_entry *log_array[LOG_ARRAY_SIZE] = {
    telemetry_log, warning_log, error_log
};


/*----------------------------------------------------------------------------*/
/*                         Interrupt Service Routines                         */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                         Private Function Prototypes                        */
/*----------------------------------------------------------------------------*/
static void clear_all_logs(void)
{
    memset(telemetry_log, 0, sizeof(telemetry_log));
    memset(warning_log, 0, sizeof(warning_log));
    memset(error_log, 0, sizeof(error_log));
}

static void add_entry_to_log(enum log_array_index log_index, 
    struct log_entry new_entry)
{
    log_array[log_index][0].timestamp = new_entry.timestamp;
    log_array[log_index][0].fail_message = new_entry.fail_message;
    log_array[log_index][0].fail_value = new_entry.fail_value;
}

/*----------------------------------------------------------------------------*/
/*                         Public Function Definitions                        */
/*----------------------------------------------------------------------------*/
void init_runtime_diagnostics()
{
    clear_all_logs();
}

void deinit_runtime_diagnostics()
{
    clear_all_logs();
}

struct log_entry *get_telemetry_log(void)
{
    return telemetry_log;
}

struct log_entry *get_warning_log(void)
{
    return warning_log;
}

struct log_entry *get_error_log(void)
{
    return error_log;
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
