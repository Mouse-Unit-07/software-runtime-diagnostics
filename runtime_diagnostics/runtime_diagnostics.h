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
enum
{
    TELEMETRY_LOG_CAPACITY = 32,
    WARNING_LOG_CAPACITY = 16,
    ERROR_LOG_CAPACITY = 8
};

/*----------------------------------------------------------------------------*/
/*                         Public Function Prototypes                         */
/*----------------------------------------------------------------------------*/
void RUNTIME_TELEMETRY(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value);
void RUNTIME_WARNING(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value);
void RUNTIME_ERROR(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value);

void set_warning_handler(void (*handler)(void));
void set_error_handler(void (*handler)(void));
void printf_telemetry_log(void);
void printf_warning_log(void);
void printf_error_log(void);
void printf_first_runtime_error_entry(void);

/* init and deinit are for testing only */
void init_runtime_diagnostics();
void deinit_runtime_diagnostics();

#endif /* RUNTIME_DIAGNOSTICS_H_ */
