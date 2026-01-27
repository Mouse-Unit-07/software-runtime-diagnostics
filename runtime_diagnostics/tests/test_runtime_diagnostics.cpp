/*================================ FILE INFO =================================*/
/* Filename           : test_runtime_diagnostics.cpp                          */
/*                                                                            */
/* Test implementation for runtime_diagnostics.c                              */
/*                                                                            */
/*============================================================================*/
/* scratch notes- a list of tests:
- does the backend print function print all logs?
- can you keep pushing to the buffer until capacity is reached?
- is the right behavior observed in response to reaching capacity for:
    - warning- assert the error flag and call the error callback
- does pushing past the buffer capacity exhibit the right behavior?:
    - telemetry- overwrite from the oldest element
    - warning- overwrite from the oldest element, and assert the error flag and call the error callback
    - error- overwrite from the newest element
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
extern struct log_entry telemetry_entries[TELEMETRY_LOG_SIZE];
extern struct log_entry warning_entries[WARNING_LOG_SIZE];
extern struct log_entry error_entries[ERROR_LOG_SIZE];
extern enum log_types_indices log_indices_array[LOG_TYPES_COUNT];

extern struct circular_buffer telemetry_cb;
extern struct circular_buffer warning_cb;
extern struct circular_buffer error_cb;

extern struct circular_buffer *circular_buffer_array[LOG_TYPES_COUNT];

extern volatile bool runtime_error_asserted;

volatile bool dummyErrorCallbackFunctionCalled = false;

static FILE *saved_stdout = nullptr;

/*============================================================================*/
/*                             Private Definitions                            */
/*============================================================================*/
namespace
{

void (*runtime_functions[LOG_TYPES_COUNT])(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value) = {RUNTIME_TELEMETRY, RUNTIME_WARNING, RUNTIME_ERROR};

void CHECK_LOG_ENTRY_EQUAL(struct log_entry expected, struct log_entry actual)
{
    CHECK_EQUAL(expected.timestamp, actual.timestamp);
    STRCMP_EQUAL(expected.fail_message, actual.fail_message);
    CHECK_EQUAL(expected.fail_value, actual.fail_value);
}

void addOneEntry(enum log_types_indices log_index, struct log_entry expected)
{
    runtime_functions[log_index](expected.timestamp, expected.fail_message, expected.fail_value);
}

void ADD_ONE_ENTRY_AND_CHECK(enum log_types_indices log_index, struct log_entry expected)
{
    addOneEntry(log_index, expected);
    
    CHECK_LOG_ENTRY_EQUAL(expected, circular_buffer_array[log_index]->log_entries[0]);
}

void ADD_THREE_ENTRIES_AND_CHECK(enum log_types_indices log_index, struct log_entry *expected)
{
    for (uint32_t i = 0; i < 3; i++) {
        addOneEntry(log_index, expected[i]);
        CHECK_LOG_ENTRY_EQUAL(expected[i], circular_buffer_array[log_index]->log_entries[i]);
    }
}

void CHECK_LOG_IS_CLEAR(enum log_types_indices log_index)
{
    struct log_entry target_entry = {0};
    for (uint32_t i = 0; i <  circular_buffer_array[log_index]->size; i++) {
        target_entry = circular_buffer_array[log_index]->log_entries[i]; 
        CHECK(target_entry.timestamp == 0);
        CHECK(target_entry.fail_message == NULL);
        CHECK(target_entry.fail_value == 0);
    }
}

void CHECK_ALL_LOGS_ARE_CLEAR(void)
{
    for (uint32_t i = 0; i < LOG_TYPES_COUNT; i++) {
        CHECK_LOG_IS_CLEAR(log_indices_array[i]);
    }
}

void CHECK_ALL_CIRCULAR_BUFFERS_FOR_NULL_LOGS(void)
{
    for (uint32_t i = 0; i < LOG_TYPES_COUNT; i++) {
        CHECK(circular_buffer_array[i]->log_entries != NULL);
    }
}

struct log_entry createOneDummyEntry(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value)
{
    struct log_entry dummyEntry = {timestamp, fail_message, fail_value};

    return dummyEntry;
}

void dummyErrorCallbackFunction(void)
{
    dummyErrorCallbackFunctionCalled = true;
}

void redirect_stdout(const char *filename)
{
    fflush(stdout);
    saved_stdout = freopen(filename, "w", stdout);
    CHECK(saved_stdout != nullptr);
}

void restore_stdout(void)
{
    fflush(stdout);
    CHECK(saved_stdout != nullptr);
    if (saved_stdout) {
        freopen("CON", "w", stdout);
    }
}

}

/*============================================================================*/
/*                                 Test Group                                 */
/*============================================================================*/
TEST_GROUP(RuntimeDiagnosticsTest)
{
    void setup() override
    {
        init_runtime_diagnostics();
    }

    void teardown() override
    {
        deinit_runtime_diagnostics();
    }
};

/*============================================================================*/
/*                                    Tests                                   */
/*============================================================================*/
TEST(RuntimeDiagnosticsTest, TelemetryLogIsInitializedToZero)
{
    CHECK_LOG_IS_CLEAR(TELEMETRY_INDEX);
}

TEST(RuntimeDiagnosticsTest, WarningLogIsInitializedToZero)
{
    CHECK_LOG_IS_CLEAR(WARNING_INDEX);
}

TEST(RuntimeDiagnosticsTest, ErrorLogIsInitializedToZero)
{
    CHECK_LOG_IS_CLEAR(ERROR_INDEX);
}

TEST(RuntimeDiagnosticsTest, CircularBufferArrayIsInitializedOnInit)
{
    init_runtime_diagnostics();
    
    CHECK_ALL_CIRCULAR_BUFFERS_FOR_NULL_LOGS();
}

TEST(RuntimeDiagnosticsTest, LogsAreClearedOnInit)
{
    init_runtime_diagnostics();
    
    CHECK_ALL_LOGS_ARE_CLEAR();
}

TEST(RuntimeDiagnosticsTest, CircularBufferArrayIsInitializedOnDeinit)
{
    deinit_runtime_diagnostics();
    
    CHECK_ALL_CIRCULAR_BUFFERS_FOR_NULL_LOGS();
}

TEST(RuntimeDiagnosticsTest, LogsAreClearedOnDeinit)
{
    deinit_runtime_diagnostics();
    
    CHECK_ALL_LOGS_ARE_CLEAR();
}

TEST(RuntimeDiagnosticsTest, AddOneEntryToTelemetryLog)
{
    ADD_ONE_ENTRY_AND_CHECK(TELEMETRY_INDEX,
        createOneDummyEntry(1, "test_runtime_diagnostics.cpp: telemetry msg", 2));
}

TEST(RuntimeDiagnosticsTest, AddOneEntryToWarningLog)
{
    ADD_ONE_ENTRY_AND_CHECK(WARNING_INDEX,
        createOneDummyEntry(1, "test_runtime_diagnostics.cpp: warning message", 2));
}

TEST(RuntimeDiagnosticsTest, AddOneEntryToErrorLog)
{
    ADD_ONE_ENTRY_AND_CHECK(ERROR_INDEX,
        createOneDummyEntry(1, "test_runtime_diagnostics.cpp: error message", 2));
}

TEST(RuntimeDiagnosticsTest, AddThreeEntriesToTelemetryLog)
{
    struct log_entry dummyEntries[3] = {0};
    for (uint32_t i = 0; i < 3; i++) {
        dummyEntries[i] = createOneDummyEntry(i, "test_runtime_diagnostics.cpp: telemetry msg", i + 1);
    }
    ADD_THREE_ENTRIES_AND_CHECK(TELEMETRY_INDEX, dummyEntries);
}

TEST(RuntimeDiagnosticsTest, AddThreeEntriesToWarningLog)
{
    struct log_entry dummyEntries[3] = {0};
    for (uint32_t i = 0; i < 3; i++) {
        dummyEntries[i] = createOneDummyEntry(i, "test_runtime_diagnostics.cpp: warning msg", i + 1);
    }
    ADD_THREE_ENTRIES_AND_CHECK(WARNING_INDEX, dummyEntries);
}

TEST(RuntimeDiagnosticsTest, AddThreeEntriesToErrorLog)
{
    struct log_entry dummyEntries[3] = {0};
    for (uint32_t i = 0; i < 3; i++) {
        dummyEntries[i] = createOneDummyEntry(i, "test_runtime_diagnostics.cpp: error msg", i + 1);
    }
    ADD_THREE_ENTRIES_AND_CHECK(ERROR_INDEX, dummyEntries);
}

TEST(RuntimeDiagnosticsTest, ErrorRuntimeFunctionAssertsFlag)
{
    addOneEntry(ERROR_INDEX,
        createOneDummyEntry(1, "test_runtime_diagnostics.cpp: error message", 2));
    CHECK(runtime_error_asserted == true);
}

TEST(RuntimeDiagnosticsTest, ErrorRuntimeFunctionCallsCallbackWhenSet)
{
    set_error_handler_function(dummyErrorCallbackFunction);
    addOneEntry(ERROR_INDEX,
        createOneDummyEntry(1, "test_runtime_diagnostics.cpp: error message", 2));
    CHECK(dummyErrorCallbackFunctionCalled == true);
}

TEST(RuntimeDiagnosticsTest, TelemetryLogPrintedWhenPartiallyFilled)
{
    for (uint32_t i = 0; i < 3; i++) {
        addOneEntry(TELEMETRY_INDEX, createOneDummyEntry(i, "dummy message", i + 1));
    }

    redirect_stdout("test_output.txt");
    printf_telemetry_log();
    restore_stdout();
    FILE *file = fopen("test_output.txt", "r");
    CHECK(file != nullptr);

    char actual_log_entry[256]; //arbitrary buffer size- individual log entries shouldn't be this long
    char expected_log_entry[256];
    uint32_t line_number = 0u;
    while (fgets(actual_log_entry, sizeof(actual_log_entry), file) != NULL)
    {
        snprintf(expected_log_entry, sizeof(expected_log_entry), "%" PRIu32 " %s %" PRIu32 "\r\n",
            circular_buffer_array[TELEMETRY_INDEX]->log_entries[line_number].timestamp,
            circular_buffer_array[TELEMETRY_INDEX]->log_entries[line_number].fail_message,
            circular_buffer_array[TELEMETRY_INDEX]->log_entries[line_number].fail_value
        );
        STRCMP_EQUAL(expected_log_entry, actual_log_entry);
        line_number++;
    }
}