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
    const char *file_identifier;
    uint16_t line;
};

enum
{
    TELEMETRY_LOG_CAPACITY = 20,
    WARNING_LOG_CAPACITY = 10,
    ERROR_LOG_CAPACITY = 10
};

#define RUNTIME_TELEMETRY(timestamp, file_identifier) \
    add_entry_to_telemetry_log((timestamp), file_identifier, __LINE__)

#define RUNTIME_WARNING(timestamp, file_identifier) \
    add_entry_to_warning_log((timestamp), file_identifier, __LINE__)

/*----------------------------------------------------------------------------*/
/*                         Public Function Prototypes                         */
/*----------------------------------------------------------------------------*/
/*
...The parameters in below functions are begging to be swapped out for a 
single log_entry, but we can't do so because:
  - We want preprocessor macro wrappers to sprinkle __FILE__ and __LINE__
  - A macro that takes a struct as a param is hard to use
  - Macro needs to dynamically create a struct w/ params to pass to function
  - Preprocessor macros expand in both C production and C++ test source files,
     and there's no uniform way to dynamically create a struct in both C/C++
  - ...4 parameters is horrible, but can't avoid it w/o roundabout logic
*/
void add_entry_to_telemetry_log(uint32_t timestamp, const char *file_identifier, 
    uint16_t line);
void add_entry_to_warning_log(uint32_t timestamp, const char *file_identifier, 
    uint16_t line);

#endif /* RUNTIME_DIAGNOSTICS_H_ */
