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

struct circular_buffer {
    struct log_entry *log_entries;
    uint32_t log_capacity;
    uint32_t head;
    uint32_t count;
};

enum log_category
{
    TELEMETRY_LOG_INDEX = 0,
    WARNING_LOG_INDEX,
    ERROR_LOG_INDEX,
    LOG_CATEGORIES_COUNT
};

enum
{
    TELEMETRY_LOG_SIZE = 20,
    WARNING_LOG_SIZE = 10,
    ERROR_LOG_SIZE = 10
};

/*----------------------------------------------------------------------------*/
/*                         Public Function Prototypes                         */
/*----------------------------------------------------------------------------*/
void init_runtime_diagnostics();
void deinit_runtime_diagnostics();

void RUNTIME_TELEMETRY(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value);
void RUNTIME_WARNING(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value);
void RUNTIME_ERROR(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value);

void set_error_handler_function(void (*handler_function)(void));
void printf_telemetry_log(void);
void printf_warning_log(void);
void printf_error_log(void);

//private helper for test
struct log_entry get_entry_at_index(enum log_category log_index, 
        uint32_t entry_index);

#endif /* RUNTIME_DIAGNOSTICS_H_ */
