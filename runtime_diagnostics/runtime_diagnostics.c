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
struct log_entry telemetry_log[TELEMETRY_LOG_CAPACITY] = {0};
struct log_entry warning_log[WARNING_LOG_CAPACITY] = {0};
struct log_entry error_log[ERROR_LOG_CAPACITY] = {0};

enum log_array_index
{
    TELEMETRY_LOG_INDEX = 0,
    WARNING_LOG_INDEX = 1,
    ERROR_LOG_INDEX = 2,
    LOG_ARRAY_SIZE = 3
};

struct log_entry *log_array[LOG_ARRAY_SIZE] = {
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
    log_array[log_index][0].file = new_entry.file;
    log_array[log_index][0].line = new_entry.line;
    log_array[log_index][0].runtime_diagnostic_identifier \
        = new_entry.runtime_diagnostic_identifier;
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

void add_entry_to_telemetry_log(uint32_t timestamp, const char *file, 
    uint16_t line, uint16_t runtime_diagnostic_identifier)
{
    struct log_entry new_entry = {timestamp, file, line, runtime_diagnostic_identifier};
    add_entry_to_log(TELEMETRY_LOG_INDEX, new_entry);
}

void add_entry_to_warning_log(uint32_t timestamp, const char *file, 
    uint16_t line, uint16_t runtime_diagnostic_identifier)
{
    struct log_entry new_entry = {timestamp, file, line, runtime_diagnostic_identifier};
    add_entry_to_log(WARNING_LOG_INDEX, new_entry);
}

/*----------------------------------------------------------------------------*/
/*                        Private Function Definitions                        */
/*----------------------------------------------------------------------------*/
/* none */
