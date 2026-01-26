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
    const char *fail_message;
    uint32_t fail_value;
};

enum
{
    TELEMETRY_LOG_SIZE = 20,
    WARNING_LOG_SIZE = 10,
    ERROR_LOG_SIZE = 10
};

void RUNTIME_TELEMETRY(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value);
void RUNTIME_WARNING(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value);
void RUNTIME_ERROR(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value);

/*----------------------------------------------------------------------------*/
/*                         Public Function Prototypes                         */
/*----------------------------------------------------------------------------*/
/* none */

#endif /* RUNTIME_DIAGNOSTICS_H_ */
