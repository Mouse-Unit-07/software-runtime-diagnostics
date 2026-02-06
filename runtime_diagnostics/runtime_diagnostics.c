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
#include <stdbool.h>
#include <inttypes.h>
#include <stdio.h>
#include "runtime_diagnostics.h"

/*----------------------------------------------------------------------------*/
/*                           Struct, Enum, Typedefs                           */
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
    uint32_t current_size;
};

enum log_category
{
    TELEMETRY_LOG_INDEX = 0,
    WARNING_LOG_INDEX,
    ERROR_LOG_INDEX,
    LOG_CATEGORIES_COUNT
};

/*----------------------------------------------------------------------------*/
/*                         Private Function Prototypes                        */
/*----------------------------------------------------------------------------*/
static void reset_all_flags(void);
static struct log_entry create_log_entry(uint32_t timestamp,
                                         const char *fail_message,
                                         uint32_t fail_value);
static void add_entry_to_circular_buffer(enum log_category log_index,
                                         struct log_entry new_entry);
static bool is_log_full(enum log_category log_index);
static void save_entry_if_first_runtime_error(struct log_entry new_log);
static void assert_runtime_error_flag(void);
static void call_error_handler_if_set(void);
static void runtime_error_procedure(struct log_entry new_log);
static void reset_log_entries(struct log_entry *entries, uint32_t entries_count);
static void reset_circular_buffer(enum log_category log_index);
static void reset_all_circular_buffers(void);
static void assert_error_handler_set_flag(void);
static struct log_entry get_entry_at_index(enum log_category log_index,
                                    uint32_t entry_index);
static void print_log_entry(struct log_entry entry);
static void printf_log(enum log_category log_index);

/*----------------------------------------------------------------------------*/
/*                               Private Globals                              */
/*----------------------------------------------------------------------------*/
enum log_category log_category_array[LOG_CATEGORIES_COUNT] = {
    TELEMETRY_LOG_INDEX, WARNING_LOG_INDEX, ERROR_LOG_INDEX
};

struct log_entry telemetry_entries[TELEMETRY_LOG_CAPACITY] = {{0}};
struct log_entry warning_entries[WARNING_LOG_CAPACITY] = {{0}};
struct log_entry error_entries[ERROR_LOG_CAPACITY] = {{0}};

struct circular_buffer telemetry_cb \
        = {telemetry_entries, TELEMETRY_LOG_CAPACITY, 0, 0};
struct circular_buffer warning_cb \
        = {warning_entries, WARNING_LOG_CAPACITY, 0, 0};
struct circular_buffer error_cb \
        = {error_entries, ERROR_LOG_CAPACITY, 0, 0};

struct circular_buffer *circular_buffer_array[LOG_CATEGORIES_COUNT] = {
    &telemetry_cb, &warning_cb, &error_cb
};

volatile bool runtime_error_asserted = false;
struct log_entry first_runtime_error_cause = {0};
bool user_error_handler_set = false;
void (*user_error_handler)(void) = NULL;

/*----------------------------------------------------------------------------*/
/*                         Public Function Definitions                        */
/*----------------------------------------------------------------------------*/
void RUNTIME_TELEMETRY(uint32_t timestamp, const char *fail_message,
                       uint32_t fail_value)
{
    add_entry_to_circular_buffer(TELEMETRY_LOG_INDEX,
        create_log_entry(timestamp, fail_message, fail_value));
}

void RUNTIME_WARNING(uint32_t timestamp, const char *fail_message,
                     uint32_t fail_value)
{
    struct log_entry new_entry = create_log_entry(
        timestamp, fail_message, fail_value);
    add_entry_to_circular_buffer(WARNING_LOG_INDEX, new_entry);

    if (is_log_full(WARNING_LOG_INDEX)) {
        runtime_error_procedure(new_entry);
    }
}

void RUNTIME_ERROR(uint32_t timestamp, const char *fail_message,
                   uint32_t fail_value)
{
    struct log_entry new_entry = create_log_entry(
        timestamp, fail_message, fail_value);
    add_entry_to_circular_buffer(ERROR_LOG_INDEX, new_entry);

    runtime_error_procedure(new_entry);
}

void set_error_handler_function(void (*handler_function)(void))
{
    user_error_handler = handler_function;
    assert_error_handler_set_flag();
}

void printf_telemetry_log(void)
{
    printf_log(TELEMETRY_LOG_INDEX);
}

void printf_warning_log(void)
{
    printf_log(WARNING_LOG_INDEX);
}

void printf_error_log(void)
{
    printf_log(ERROR_LOG_INDEX);
}

void printf_first_runtime_error_entry(void)
{
    print_log_entry(first_runtime_error_cause);
}

void init_runtime_diagnostics()
{
    reset_all_flags();
    reset_all_circular_buffers();
}

void deinit_runtime_diagnostics()
{
    reset_all_flags();
    reset_all_circular_buffers();
}

/*----------------------------------------------------------------------------*/
/*                        Private Function Definitions                        */
/*----------------------------------------------------------------------------*/
static void reset_all_flags(void)
{
    runtime_error_asserted = false;
    user_error_handler_set = false;
}

static struct log_entry create_log_entry(uint32_t timestamp,
                                         const char *fail_message,
                                         uint32_t fail_value)
{
    return (struct log_entry){timestamp, fail_message, fail_value};
}

static void add_entry_to_circular_buffer(enum log_category log_index,
                                         struct log_entry new_entry)
{
    struct circular_buffer *target_cb = circular_buffer_array[log_index];
    struct log_entry *target_entry = &(target_cb->log_entries[target_cb->head]);
    memcpy(target_entry, &new_entry, sizeof(new_entry));

    target_cb->head = (target_cb->head + 1) % target_cb->log_capacity;
    if (target_cb->current_size != target_cb->log_capacity) {
        target_cb->current_size++;
    }
}

static bool is_log_full(enum log_category log_index)
{
    return circular_buffer_array[log_index]->log_capacity ==
           circular_buffer_array[log_index]->current_size;
}

static void save_entry_if_first_runtime_error(struct log_entry new_log)
{
    if (!runtime_error_asserted) {
        first_runtime_error_cause = new_log;
    }
}

static void assert_runtime_error_flag(void)
{
    runtime_error_asserted = true;
}

static void call_error_handler_if_set(void)
{
    if (user_error_handler_set) {
        user_error_handler();
    }
}

static void runtime_error_procedure(struct log_entry new_log)
{
    save_entry_if_first_runtime_error(new_log);
    assert_runtime_error_flag();
    call_error_handler_if_set();
}

static void reset_log_entries(struct log_entry *entries, uint32_t entries_count)
{
    memset(entries, 0, sizeof(struct log_entry) * entries_count);
}

static void reset_circular_buffer(enum log_category log_index)
{
    struct circular_buffer *target_cb = circular_buffer_array[log_index];
    reset_log_entries(target_cb->log_entries, target_cb->log_capacity);
    target_cb->head = 0;
    target_cb->current_size = 0;
}

static void reset_all_circular_buffers(void)
{
    for (uint32_t i = 0u; i < LOG_CATEGORIES_COUNT; i++) {
        reset_circular_buffer(log_category_array[i]);
    }
}

static void assert_error_handler_set_flag(void)
{
    user_error_handler_set = true;
}

static struct log_entry get_entry_at_index(enum log_category log_index,
                                    uint32_t entry_index)
{
    struct circular_buffer *target_cb = circular_buffer_array[log_index];
    uint32_t oldest_entry_index \
        = (target_cb->head + target_cb->log_capacity - target_cb->current_size) \
        % target_cb->log_capacity;
    uint32_t return_entry_index =
        (oldest_entry_index + entry_index) % target_cb->log_capacity;
    return target_cb->log_entries[return_entry_index];
}

static void print_log_entry(struct log_entry entry)
{
    printf("%" PRIu32 " %s %" PRIu32 "\r\n", entry.timestamp,
           entry.fail_message, entry.fail_value);
}

static void printf_log(enum log_category log_index)
{
    uint32_t current_size = circular_buffer_array[log_index]->current_size;
    for (uint32_t i = 0u; i < current_size; i++) {
        struct log_entry entry = get_entry_at_index(log_index, i);
        print_log_entry(entry);
    }
}
