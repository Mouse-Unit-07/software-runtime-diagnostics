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
    const char *file;
    uint32_t timestamp;
    uint16_t line;
    uint16_t runtime_diagnostic_identifier;
};

enum {LOG_CAPACITY = 16};

/*----------------------------------------------------------------------------*/
/*                         Public Function Prototypes                         */
/*----------------------------------------------------------------------------*/
void init_runtime_diagnostics();
void deinit_runtime_diagnostics();
struct log_entry *get_telemetry_log(void);

#endif /* RUNTIME_DIAGNOSTICS_H_ */
