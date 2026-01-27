/*================================ FILE INFO =================================*/
/* Filename           : test_runtime_diagnostics.cpp                          */
/*                                                                            */
/* Test implementation for runtime_diagnostics.c                              */
/*                                                                            */
/*============================================================================*/
/* scratch notes- a list of tests:
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

volatile bool dummyErrorCallbackFunctionCalled{false};

static FILE *standardOutput{nullptr};

void (*runtime_functions[LOG_TYPES_COUNT])(uint32_t timestamp, const char *fail_message,
        uint32_t fail_value){RUNTIME_TELEMETRY, RUNTIME_WARNING, RUNTIME_ERROR};

void (*print_log_functions[LOG_TYPES_COUNT])(void){printf_telemetry_log, printf_warning_log, printf_error_log};

/*============================================================================*/
/*                             Private Definitions                            */
/*============================================================================*/
namespace
{

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
    
    log_entry new_entry = get_entry_at_index(log_index, circular_buffer_array[log_index]->count - 1);
    CHECK_LOG_ENTRY_EQUAL(expected, new_entry);
}

void CHECK_LOG_IS_CLEAR(enum log_types_indices log_index)
{
    struct log_entry target_entry{0};
    for (uint32_t i{0}; i < circular_buffer_array[log_index]->size; i++) {
        target_entry = circular_buffer_array[log_index]->log_entries[i]; 
        CHECK(target_entry.timestamp == 0);
        CHECK(target_entry.fail_message == NULL);
        CHECK(target_entry.fail_value == 0);
    }
}

void CHECK_ALL_LOGS_ARE_CLEAR(void)
{
    for (uint32_t i{0}; i < LOG_TYPES_COUNT; i++) {
        CHECK_LOG_IS_CLEAR(log_indices_array[i]);
    }
}

void CHECK_ALL_OTHER_LOGS_ARE_CLEAR(enum log_types_indices log_index)
{
    for (uint32_t i{0}; i < LOG_TYPES_COUNT; i++) {
        if (log_indices_array[i] != log_index) {
            CHECK_LOG_IS_CLEAR(log_indices_array[i]);
        }
    }
}

void CHECK_ALL_CIRCULAR_BUFFERS_FOR_NULL_LOGS(void)
{
    for (uint32_t i{0}; i < LOG_TYPES_COUNT; i++) {
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

void redirectStdout(const char *filename)
{
    fflush(stdout);
    standardOutput = freopen(filename, "w", stdout);
    CHECK(standardOutput != nullptr);
}

void restoreStdout(void)
{
    fflush(stdout);
    CHECK(standardOutput != nullptr);
    if (standardOutput) {
        freopen("CON", "w", stdout);
    }
}

bool isFileEmpty(FILE *file)
{
    int c{fgetc(file)};
    if (c == EOF) {
        return true;
    }
    ungetc(c, file);

    return false;
}

void COMPARE_LOG_AND_FILE(FILE *file, enum log_types_indices log_index)
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

void PRINT_LOG_TO_FILE_AND_CHECK_FILE(enum log_types_indices log_index)
{
    const char *TEST_FILENAME{"test_output.txt"};
    redirectStdout(TEST_FILENAME);
    print_log_functions[log_index]();
    restoreStdout();
    FILE *file{fopen(TEST_FILENAME, "r")};
    CHECK(file != nullptr);
    CHECK(!isFileEmpty(file));

    COMPARE_LOG_AND_FILE(file, log_index);
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

TEST(RuntimeDiagnosticsTest, CheckThatNoBuffersHaveZeroSize)
{
    for (uint32_t i{0}; i < LOG_TYPES_COUNT; i++) {
        CHECK(circular_buffer_array[log_indices_array[i]]->size != 0);
    }
}

TEST(RuntimeDiagnosticsTest, AddOneEntryToTelemetryLogOnly)
{
    ADD_ONE_ENTRY_AND_CHECK(TELEMETRY_INDEX,
        createOneDummyEntry(1, "test_runtime_diagnostics.cpp: telemetry msg", 2));
    
    CHECK_ALL_OTHER_LOGS_ARE_CLEAR(TELEMETRY_INDEX);
}

TEST(RuntimeDiagnosticsTest, AddOneEntryToWarningLogOnly)
{
    ADD_ONE_ENTRY_AND_CHECK(WARNING_INDEX,
        createOneDummyEntry(1, "test_runtime_diagnostics.cpp: warning message", 2));
    
    CHECK_ALL_OTHER_LOGS_ARE_CLEAR(WARNING_INDEX);
}

TEST(RuntimeDiagnosticsTest, AddOneEntryToErrorLogOnly)
{
    ADD_ONE_ENTRY_AND_CHECK(ERROR_INDEX,
        createOneDummyEntry(1, "test_runtime_diagnostics.cpp: error message", 2));
    
    CHECK_ALL_OTHER_LOGS_ARE_CLEAR(ERROR_INDEX);
}

TEST(RuntimeDiagnosticsTest, AddOneLessThanMaxEntriesToTelemetryLog)
{
    for (uint32_t i{0}; i < (TELEMETRY_LOG_SIZE - 1); i++) {
        ADD_ONE_ENTRY_AND_CHECK(TELEMETRY_INDEX, createOneDummyEntry(i, "test_runtime_diagnostics.cpp: telemetry msg", i + 1));
    }
}

TEST(RuntimeDiagnosticsTest, AddMaxEntriesToTelemetryLog)
{
    for (uint32_t i{0}; i < TELEMETRY_LOG_SIZE; i++) {
        ADD_ONE_ENTRY_AND_CHECK(TELEMETRY_INDEX, createOneDummyEntry(i, "test_runtime_diagnostics.cpp: telemetry msg", i + 1));
    }
}

TEST(RuntimeDiagnosticsTest, OverflowEntriesToTelemetryLog)
{
    struct log_entry expected[TELEMETRY_LOG_SIZE] = {{0}};
    struct log_entry new_entry{0};
    uint32_t totalNewEntriesCount = TELEMETRY_LOG_SIZE + 17; // arbitrary [prime number] overflow entries
    uint32_t startRecordingIndex = totalNewEntriesCount - TELEMETRY_LOG_SIZE;

    for (uint32_t i{0}; i < totalNewEntriesCount; i++) {
        new_entry = createOneDummyEntry(i, "test_runtime_diagnostics.cpp: telemetry msg", i + 1);
        addOneEntry(TELEMETRY_INDEX, new_entry);
        if (i >= startRecordingIndex) {
            expected[i - startRecordingIndex] = new_entry;
        }
    }
    for (uint32_t i{0}; i < TELEMETRY_LOG_SIZE; i++) {
        struct log_entry actual_entry = get_entry_at_index(TELEMETRY_INDEX, i);
        CHECK_LOG_ENTRY_EQUAL(expected[i], actual_entry);
    }
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
    for (uint32_t i{0}; i < 3; i++) {
        addOneEntry(TELEMETRY_INDEX, createOneDummyEntry(i, "test_runtime_diagnostics.cpp: telemetry message", i + 1));
    }
    PRINT_LOG_TO_FILE_AND_CHECK_FILE(TELEMETRY_INDEX);
}

TEST(RuntimeDiagnosticsTest, WarningLogPrintedWhenPartiallyFilled)
{
    for (uint32_t i{0}; i < 3; i++) {
        addOneEntry(WARNING_INDEX, createOneDummyEntry(i, "test_runtime_diagnostics.cpp: warning message", i + 1));
    }
    PRINT_LOG_TO_FILE_AND_CHECK_FILE(WARNING_INDEX);
}

TEST(RuntimeDiagnosticsTest, ErrorLogPrintedWhenPartiallyFilled)
{
    for (uint32_t i{0}; i < 3; i++) {
        addOneEntry(ERROR_INDEX, createOneDummyEntry(i, "test_runtime_diagnostics.cpp: error message", i + 1));
    }
    PRINT_LOG_TO_FILE_AND_CHECK_FILE(ERROR_INDEX);
}
