/*================================ FILE INFO =================================*/
/* Filename           : test_runtime_diagnostics.cpp                          */
/*                                                                            */
/* Test implementation for runtime_diagnostics.c                              */
/*                                                                            */
/*============================================================================*/
/* scratch notes- a list of tests:
- can the log saved from the first runtime error be printed?
- are all globals cleared on init/deinit? 
*/

/*============================================================================*/
/*                               Include Files                                */
/*============================================================================*/
extern "C" {
  #include <stdint.h>
  #include "runtime_diagnostics.h"
  #include <inttypes.h>
}

#include <cstdint>
#include <cstdio>
#include <CppUTest/TestHarness.h>

/*============================================================================*/
/*                                   Globals                                  */
/*============================================================================*/
extern struct log_entry telemetry_entries[TELEMETRY_LOG_CAPACITY];
extern struct log_entry warning_entries[WARNING_LOG_CAPACITY];
extern struct log_entry error_entries[ERROR_LOG_CAPACITY];
extern enum log_category log_category_array[LOG_CATEGORIES_COUNT];
extern struct circular_buffer telemetry_cb;
extern struct circular_buffer warning_cb;
extern struct circular_buffer error_cb;
extern struct circular_buffer *circular_buffer_array[LOG_CATEGORIES_COUNT];
extern volatile bool runtime_error_asserted;
extern struct log_entry first_runtime_error_cause;
extern bool user_error_handler_set;
extern void (*user_error_handler)(void);

volatile bool dummy_error_callback_called{false};

static FILE *saved_output{nullptr};

void (*runtime_functions[LOG_CATEGORIES_COUNT])(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value){RUNTIME_TELEMETRY, RUNTIME_WARNING, RUNTIME_ERROR};

void (*print_log_functions[LOG_CATEGORIES_COUNT])(void){printf_telemetry_log, printf_warning_log, printf_error_log};

/*============================================================================*/
/*                             Private Definitions                            */
/*============================================================================*/
namespace
{

struct log_entry create_one_dummy_entry(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value)
{
    struct log_entry dummy_entry = {timestamp, fail_message, fail_value};

    return dummy_entry;
}

void add_one_entry(enum log_category log_index, struct log_entry expected)
{
    runtime_functions[log_index](expected.timestamp, expected.fail_message, expected.fail_value);
}

void add_n_entries(enum log_category log_index, uint32_t n)
{
    for (uint32_t i{0}; i < n; i++) {
        add_one_entry(log_index, create_one_dummy_entry(i, "test_runtime_diagnostics.cpp: msg", i + 1));
    }
}

void overflow_log_and_create_expected(enum log_category log_index, struct log_entry *expected, 
    uint32_t overflow_entries_count)
{
    struct circular_buffer *target_cb{circular_buffer_array[log_index]};
    uint32_t log_capacity{target_cb->log_capacity};
    uint32_t new_entries_count = log_capacity + overflow_entries_count;
    
    uint32_t start_recording_index{new_entries_count - log_capacity};
    struct log_entry new_entry{0};
    for (uint32_t i{0}; i < new_entries_count; i++) {
        new_entry = create_one_dummy_entry(i, "test_runtime_diagnostics.cpp: some msg", i + 1);
        add_one_entry(log_index, new_entry);
        if (i >= start_recording_index) {
            expected[i - start_recording_index] = new_entry;
        }
    }
}

void overflow_log(enum log_category log_index, uint32_t overflow_entries_count)
{
    struct circular_buffer *target_cb{circular_buffer_array[log_index]};
    uint32_t log_capacity{target_cb->log_capacity};
    uint32_t new_entries_count = log_capacity + overflow_entries_count;
    
    add_n_entries(log_index, new_entries_count);
}

void CHECK_LOG_ENTRY_EQUAL(struct log_entry expected, struct log_entry actual)
{
    CHECK_EQUAL(expected.timestamp, actual.timestamp);
    STRCMP_EQUAL(expected.fail_message, actual.fail_message);
    CHECK_EQUAL(expected.fail_value, actual.fail_value);
}

void COMPARE_LOG_AND_EXPECTED(enum log_category log_index, struct log_entry *expected)
{
    for (uint32_t i{0}; i < circular_buffer_array[log_index]->log_capacity; i++) {
        struct log_entry actual_entry = get_entry_at_index(log_index, i);
        CHECK_LOG_ENTRY_EQUAL(expected[i], actual_entry);
    }
}

void ADD_ONE_ENTRY_AND_CHECK(enum log_category log_index, struct log_entry expected)
{
    add_one_entry(log_index, expected);
    
    log_entry new_entry = get_entry_at_index(log_index, circular_buffer_array[log_index]->current_size - 1);
    CHECK_LOG_ENTRY_EQUAL(expected, new_entry);
}

void ADD_N_ENTRIES_AND_CHECK(enum log_category log_index, uint32_t n)
{
    for (uint32_t i{0}; i < n; i++) {
        ADD_ONE_ENTRY_AND_CHECK(log_index, create_one_dummy_entry(i, "test_runtime_diagnostics.cpp: msg", i + 1));
    }
}

void CHECK_LOG_IS_CLEAR(enum log_category log_index)
{
    struct log_entry target_entry{0};
    for (uint32_t i{0}; i < circular_buffer_array[log_index]->log_capacity; i++) {
        target_entry = circular_buffer_array[log_index]->log_entries[i]; 
        CHECK(target_entry.timestamp == 0);
        CHECK(target_entry.fail_message == NULL);
        CHECK(target_entry.fail_value == 0);
    }
}

void CHECK_ALL_LOGS_ARE_CLEAR(void)
{
    for (uint32_t i{0}; i < LOG_CATEGORIES_COUNT; i++) {
        CHECK_LOG_IS_CLEAR(log_category_array[i]);
    }
}

void CHECK_ALL_OTHER_LOGS_ARE_CLEAR(enum log_category log_index)
{
    for (uint32_t i{0}; i < LOG_CATEGORIES_COUNT; i++) {
        if (log_category_array[i] != log_index) {
            CHECK_LOG_IS_CLEAR(log_category_array[i]);
        }
    }
}

void CHECK_ALL_CIRCULAR_BUFFERS_FOR_NULL_LOGS(void)
{
    for (uint32_t i{0}; i < LOG_CATEGORIES_COUNT; i++) {
        CHECK(circular_buffer_array[i]->log_entries != NULL);
    }
}

void dummy_callback_function(void)
{
    dummy_error_callback_called = true;
}

void redirect_stdout(const char *filename)
{
    fflush(stdout);
    saved_output = freopen(filename, "w", stdout);
    CHECK(saved_output != nullptr);
}

void restore_stdout(void)
{
    fflush(stdout);
    CHECK(saved_output != nullptr);
    if (saved_output) {
        freopen("CON", "w", stdout);
    }
}

bool is_file_empty(FILE *file)
{
    int c{fgetc(file)};
    if (c == EOF) {
        return true;
    }
    ungetc(c, file);

    return false;
}

void COMPARE_LOG_AND_FILE(FILE *file, enum log_category log_index)
{
    enum {LOG_ENTRY_MAX_SIZE = 256}; //arbitrary max- 256 bytes is plenty to cover any log entry
    char actual_log_entry[LOG_ENTRY_MAX_SIZE];
    char expected_log_entry[LOG_ENTRY_MAX_SIZE];

    uint32_t entry_index = 0u;
    while (fgets(actual_log_entry, sizeof(actual_log_entry), file) != NULL) {
        log_entry raw_entry = get_entry_at_index(log_index, entry_index);
        snprintf(expected_log_entry, sizeof(expected_log_entry), "%" PRIu32 " %s %" PRIu32 "\r\n",
            raw_entry.timestamp,
            raw_entry.fail_message,
            raw_entry.fail_value
        );
        STRCMP_EQUAL(expected_log_entry, actual_log_entry);
        entry_index++;
    }
}

void PRINT_LOG_TO_FILE_AND_CHECK_FILE(enum log_category log_index)
{
    const char *TEST_FILENAME{"test_output.txt"};
    redirect_stdout(TEST_FILENAME);
    print_log_functions[log_index]();
    restore_stdout();
    FILE *file{fopen(TEST_FILENAME, "r")};
    CHECK(file != nullptr);
    CHECK(!is_file_empty(file));

    COMPARE_LOG_AND_FILE(file, log_index);
}

void CHECK_ALL_FLAGS(void)
{
    CHECK(runtime_error_asserted == false);
    CHECK(user_error_handler_set == false);
}

void CHECK_FOR_ZERO_CAPACITY_LOGS(void)
{
    for (uint32_t i{0}; i < LOG_CATEGORIES_COUNT; i++) {
        CHECK(circular_buffer_array[log_category_array[i]]->log_capacity != 0);
    }
}

void CHECK_RUNTIME_ERROR_FLAG_ASSERTED(void)
{
    CHECK(runtime_error_asserted == true);
}

void CHECK_ERROR_CALLBACK_CALLED_FLAG_ASSERTED(void)
{
    CHECK(dummy_error_callback_called == true);
}

}

/*============================================================================*/
/*                                 Test Group                                 */
/*============================================================================*/
TEST_GROUP(RuntimeDiagnosticsTest)
{
    void setup() override
    {
        dummy_error_callback_called = false;
        init_runtime_diagnostics();
    }

    void teardown() override
    {
        deinit_runtime_diagnostics();
        dummy_error_callback_called = false;
    }
};

/*============================================================================*/
/*                                    Tests                                   */
/*============================================================================*/
TEST(RuntimeDiagnosticsTest, TelemetryLogIsInitializedToZero)
{
    CHECK_LOG_IS_CLEAR(TELEMETRY_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, WarningLogIsInitializedToZero)
{
    CHECK_LOG_IS_CLEAR(WARNING_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, ErrorLogIsInitializedToZero)
{
    CHECK_LOG_IS_CLEAR(ERROR_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, NoCircularBufferHasNullBufferOnInit)
{
    init_runtime_diagnostics();
    
    CHECK_ALL_CIRCULAR_BUFFERS_FOR_NULL_LOGS();
}

TEST(RuntimeDiagnosticsTest, LogsAreClearedOnInit)
{
    init_runtime_diagnostics();
    
    CHECK_ALL_LOGS_ARE_CLEAR();
}

TEST(RuntimeDiagnosticsTest, NoCircularBufferHasNullBufferOnDeinit)
{
    deinit_runtime_diagnostics();
    
    CHECK_ALL_CIRCULAR_BUFFERS_FOR_NULL_LOGS();
}

TEST(RuntimeDiagnosticsTest, LogsAreClearedOnDeinit)
{
    deinit_runtime_diagnostics();
    
    CHECK_ALL_LOGS_ARE_CLEAR();
}

TEST(RuntimeDiagnosticsTest, FlagsAreClearedOnInit)
{
    init_runtime_diagnostics();
    
    CHECK_ALL_FLAGS();
}

TEST(RuntimeDiagnosticsTest, FlagsAreClearedOnDeinit)
{
    deinit_runtime_diagnostics();
    
    CHECK_ALL_FLAGS();
}

TEST(RuntimeDiagnosticsTest, NoBuffersHaveZeroCapacityOnInit)
{
    init_runtime_diagnostics();

    CHECK_FOR_ZERO_CAPACITY_LOGS();
}

TEST(RuntimeDiagnosticsTest, NoBuffersHaveZeroCapacityOnDeinit)
{
    deinit_runtime_diagnostics();
    
    CHECK_FOR_ZERO_CAPACITY_LOGS();
}

TEST(RuntimeDiagnosticsTest, AddOneEntryToTelemetryLogOnly)
{
    ADD_ONE_ENTRY_AND_CHECK(TELEMETRY_LOG_INDEX,
        create_one_dummy_entry(1, "test_runtime_diagnostics.cpp: telemetry msg", 2));
    
    CHECK_ALL_OTHER_LOGS_ARE_CLEAR(TELEMETRY_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, AddOneEntryToWarningLogOnly)
{
    ADD_ONE_ENTRY_AND_CHECK(WARNING_LOG_INDEX,
        create_one_dummy_entry(1, "test_runtime_diagnostics.cpp: warning message", 2));
    
    CHECK_ALL_OTHER_LOGS_ARE_CLEAR(WARNING_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, AddOneEntryToErrorLogOnly)
{
    ADD_ONE_ENTRY_AND_CHECK(ERROR_LOG_INDEX,
        create_one_dummy_entry(1, "test_runtime_diagnostics.cpp: error message", 2));
    
    CHECK_ALL_OTHER_LOGS_ARE_CLEAR(ERROR_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, AddOneLessThanMaxEntriesToTelemetryLog)
{
    ADD_N_ENTRIES_AND_CHECK(TELEMETRY_LOG_INDEX, TELEMETRY_LOG_CAPACITY - 1);
}

TEST(RuntimeDiagnosticsTest, AddMaxEntriesToTelemetryLog)
{
    ADD_N_ENTRIES_AND_CHECK(TELEMETRY_LOG_INDEX, TELEMETRY_LOG_CAPACITY);
}

TEST(RuntimeDiagnosticsTest, OverflowEntriesToTelemetryLog)
{
    struct log_entry expected[TELEMETRY_LOG_CAPACITY] = {{0}};
    uint32_t overflow_entries_count = 17; // arbitrary [prime number] overflow entries
    
    overflow_log_and_create_expected(TELEMETRY_LOG_INDEX, expected, overflow_entries_count);

    COMPARE_LOG_AND_EXPECTED(TELEMETRY_LOG_INDEX, expected);
}

TEST(RuntimeDiagnosticsTest, ErrorRuntimeFunctionAssertsFlag)
{
    add_one_entry(ERROR_LOG_INDEX,
        create_one_dummy_entry(1, "test_runtime_diagnostics.cpp: error message", 2));
    CHECK_RUNTIME_ERROR_FLAG_ASSERTED();
}

TEST(RuntimeDiagnosticsTest, ErrorRuntimeFunctionCallsCallbackWhenSet)
{
    set_error_handler_function(dummy_callback_function);
    add_one_entry(ERROR_LOG_INDEX,
        create_one_dummy_entry(1, "test_runtime_diagnostics.cpp: error message", 2));
    CHECK_ERROR_CALLBACK_CALLED_FLAG_ASSERTED();
}

TEST(RuntimeDiagnosticsTest, FullWarningLogAssertsErrorAndCallsCallback)
{
    set_error_handler_function(dummy_callback_function);
    for (uint32_t i{0}; i < WARNING_LOG_CAPACITY; i++) {
        add_one_entry(WARNING_LOG_INDEX, create_one_dummy_entry(i, "test_runtime_diagnostics.cpp: warning msg", i + 1));
    }
    CHECK_RUNTIME_ERROR_FLAG_ASSERTED();
    CHECK_ERROR_CALLBACK_CALLED_FLAG_ASSERTED();
}

TEST(RuntimeDiagnosticsTest, FirstErrorIsSavedFromErrorFunctionCall)
{
    struct log_entry expected = create_one_dummy_entry(1, "test_runtime_diagnostics.cpp: error message", 2);
    add_one_entry(ERROR_LOG_INDEX, expected);
    overflow_log(ERROR_LOG_INDEX, ERROR_LOG_CAPACITY);
    CHECK_LOG_ENTRY_EQUAL(expected, first_runtime_error_cause);
}

TEST(RuntimeDiagnosticsTest, FirstErrorIsSavedFromFullWarningLog)
{
    struct log_entry expected{0};
    for (uint32_t i{0}; i < WARNING_LOG_CAPACITY; i++) {
        struct log_entry new_entry = create_one_dummy_entry(i, "test_runtime_diagnostics.cpp: warning msg", i + 1);
        add_one_entry(WARNING_LOG_INDEX, new_entry);
        if (i == (WARNING_LOG_CAPACITY - 1)) {
            expected = new_entry;
        }
    }
    overflow_log(WARNING_LOG_INDEX, WARNING_LOG_CAPACITY);

    CHECK_LOG_ENTRY_EQUAL(expected, first_runtime_error_cause);
}

TEST(RuntimeDiagnosticsTest, TelemetryLogPrintedWhenPartiallyFilled)
{
    for (uint32_t i{0}; i < 3; i++) {
        add_one_entry(TELEMETRY_LOG_INDEX, create_one_dummy_entry(i, "test_runtime_diagnostics.cpp: telemetry message", i + 1));
    }
    PRINT_LOG_TO_FILE_AND_CHECK_FILE(TELEMETRY_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, WarningLogPrintedWhenPartiallyFilled)
{
    for (uint32_t i{0}; i < 3; i++) {
        add_one_entry(WARNING_LOG_INDEX, create_one_dummy_entry(i, "test_runtime_diagnostics.cpp: warning message", i + 1));
    }
    PRINT_LOG_TO_FILE_AND_CHECK_FILE(WARNING_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, ErrorLogPrintedWhenPartiallyFilled)
{
    for (uint32_t i{0}; i < 3; i++) {
        add_one_entry(ERROR_LOG_INDEX, create_one_dummy_entry(i, "test_runtime_diagnostics.cpp: error message", i + 1));
    }
    PRINT_LOG_TO_FILE_AND_CHECK_FILE(ERROR_LOG_INDEX);
}
