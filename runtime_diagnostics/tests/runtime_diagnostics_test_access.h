/*-------------------------------- FILE INFO ---------------------------------*/
/* Filename           : runtime_diagnostics.h                                 */
/*                                                                            */
/* Interface to runtime logging for telemetry, warnings, and errors           */
/*                                                                            */
/*----------------------------------------------------------------------------*/
#ifndef RUNTIME_DIAGNOSTICS_TEST_ACCESS_H_
#define RUNTIME_DIAGNOSTICS_TEST_ACCESS_H_

/*----------------------------------------------------------------------------*/
/*                             Public Definitions                             */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                         Public Function Prototypes                         */
/*----------------------------------------------------------------------------*/
void init_runtime_diagnostics();
void deinit_runtime_diagnostics();
struct log_entry *get_telemetry_log(void);
struct log_entry *get_warning_log(void);
struct log_entry *get_error_log(void);
struct log_entry create_log_entry(uint32_t timestamp,
    const char *file_identifier, uint16_t line);

#endif /* RUNTIME_DIAGNOSTICS_TEST_ACCESS_H_ */
