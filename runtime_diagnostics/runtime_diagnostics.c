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

void add_entry_to_telemetry_log(uint32_t timestamp, const char *file, uint16_t line,
    uint16_t runtime_diagnostic_identifier)
{
    telemetry_log[0].timestamp = timestamp;
    telemetry_log[0].file = file;
    telemetry_log[0].line = line;
    telemetry_log[0].runtime_diagnostic_identifier = runtime_diagnostic_identifier;
}

/*----------------------------------------------------------------------------*/
/*                        Private Function Definitions                        */
/*----------------------------------------------------------------------------*/
/* none */
