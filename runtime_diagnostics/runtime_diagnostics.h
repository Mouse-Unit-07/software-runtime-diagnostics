/*-------------------------------- FILE INFO ---------------------------------*/
/* Filename           : runtime_diagnostics.h                                 */
/*                                                                            */
/* Interface to runtime logging for telemetry, warnings, and errors           */
/*                                                                            */
/*----------------------------------------------------------------------------*/
#ifndef RUNTIME_DIAGNOSTICS_H_
#define RUNTIME_DIAGNOSTICS_H_

/*----------------------------------------------------------------------------*/
/*                             Public Definitions                             */
/*----------------------------------------------------------------------------*/
struct log_entry {
    uint32_t timestamp;
    const char *file;
    uint16_t line;
    uint16_t runtime_diagnostic_identifier;
};

enum
{
    TELEMETRY_LOG_CAPACITY = 20,
    WARNING_LOG_CAPACITY = 10,
    ERROR_LOG_CAPACITY = 10
};

#define ADD_RUNTIME_TELEMETRY(timestamp, runtime_diagnostic_identifier) \
    add_entry_to_telemetry_log(timestamp, __FILE__, __LINE__, runtime_diagnostic_identifier);

/*----------------------------------------------------------------------------*/
/*                         Public Function Prototypes                         */
/*----------------------------------------------------------------------------*/
void add_entry_to_telemetry_log(uint32_t timestamp, const char *file, uint16_t line, 
    uint16_t runtime_diagnostic_identifier);

#endif /* RUNTIME_DIAGNOSTICS_H_ */
