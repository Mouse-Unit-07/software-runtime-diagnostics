/*================================ FILE INFO =================================*/
/* Filename           : test_runtime_diagnostics.cpp                          */
/*                                                                            */
/* Test implementation for runtime_diagnostics.c                              */
/*                                                                            */
/*============================================================================*/

/*============================================================================*/
/*                               Include Files                                */
/*============================================================================*/
extern "C" {
#include <stdint.h>
#include <inttypes.h>
#include "runtime_diagnostics.h"
}

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <CppUTest/TestHarness.h>

/*============================================================================*/
/*                             Public Definitions                             */
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

FILE *standard_output{nullptr};
constexpr const char *TEST_OUTPUT_FILE{"test_output.txt"};
constexpr const char *TEST_EXPECTATIONS_FILE{"test_expectations.txt"};

void (*runtime_functions[LOG_CATEGORIES_COUNT])(uint32_t timestamp,
                                                const char *fail_message,
                                                uint32_t fail_value){
    RUNTIME_TELEMETRY, RUNTIME_WARNING, RUNTIME_ERROR
};

void redirect_stdout_to_file(void)
{
    standard_output = stdout;
    CHECK(freopen(TEST_OUTPUT_FILE, "w+", stdout) != nullptr);
}

void restore_stdout(void)
{
    CHECK(stdout != nullptr);
    fclose(stdout);
    CHECK(freopen("CON", "w", standard_output) != nullptr);
}

bool is_test_file_empty(void)
{
    FILE *file{fopen(TEST_OUTPUT_FILE, "r")};

    if (!file) {
        return true;
    }

    const long current = ftell(file);
    if (current < 0) {
        return false; // ftell failed
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        return false;
    }

    const long size = ftell(file);

    // Restore position
    fseek(file, current, SEEK_SET);

    bool result{size == 0};

    fclose(file);

    return result;
}

void clear_all_test_files(void)
{
    FILE* file{fopen(TEST_OUTPUT_FILE, "w")};
    CHECK(file != nullptr);
    fclose(file);
    file = fopen(TEST_EXPECTATIONS_FILE, "w");
    CHECK(file != nullptr);
    fclose(file);
}

void add_log_entry_to_expectations_file(uint32_t timestamp,
                                       const char* fail_message,
                                       uint32_t fail_value)
{
    FILE* file{fopen(TEST_EXPECTATIONS_FILE, "a")};
    CHECK(file != nullptr);

    const int written = fprintf(
        file,
        "%" PRIu32 " %s %" PRIu32 "\r\n",
        timestamp,
        fail_message,
        fail_value
    );

    CHECK(written > 0);
    fclose(file);
}

bool test_output_and_expectation_are_identical(void)
{
    FILE* a{fopen(TEST_OUTPUT_FILE, "rb")};
    FILE* b{fopen(TEST_EXPECTATIONS_FILE, "rb")};

    if (!a || !b) {
        if (a) fclose(a);
        if (b) fclose(b);
        return false;
    }

    std::array<std::uint8_t, 4096> buf_a{};
    std::array<std::uint8_t, 4096> buf_b{};

    while (true) {
        const size_t read_a = fread(buf_a.data(), 1, buf_a.size(), a);
        const size_t read_b = fread(buf_b.data(), 1, buf_b.size(), b);

        if (read_a != read_b) {
            fclose(a);
            fclose(b);
            return false;
        }

        if (read_a == 0) {
            break; // both EOF
        }

        if (std::memcmp(buf_a.data(), buf_b.data(), read_a) != 0) {
            fclose(a);
            fclose(b);
            return false;
        }
    }

    fclose(a);
    fclose(b);
    return true;
}

void print_all_logs(void)
{
    printf_telemetry_log();
    printf_warning_log();
    printf_telemetry_log();
}

void print_all_logs_and_expect_empty_output(void)
{
    print_all_logs();
    fflush(stdout);
    CHECK(is_test_file_empty() == true);
}

void CHECK_LOG_IS_CLEAR(enum log_category log_index)
{
    struct log_entry target_entry{0};
    for (uint32_t i{0u}; i < circular_buffer_array[log_index]->log_capacity;
         i++) {
        target_entry = circular_buffer_array[log_index]->log_entries[i];
        CHECK(target_entry.timestamp == 0);
        CHECK(target_entry.fail_message == NULL);
        CHECK(target_entry.fail_value == 0);
    }
}

void CHECK_ALL_LOGS_ARE_CLEAR(void)
{
    for (uint32_t i{0u}; i < LOG_CATEGORIES_COUNT; i++) {
        CHECK_LOG_IS_CLEAR(log_category_array[i]);
    }
}

void CHECK_ALL_OTHER_LOGS_ARE_CLEAR(enum log_category log_index)
{
    for (uint32_t i{0u}; i < LOG_CATEGORIES_COUNT; i++) {
        if (log_category_array[i] != log_index) {
            CHECK_LOG_IS_CLEAR(log_category_array[i]);
        }
    }
}

void CHECK_ALL_CIRCULAR_BUFFERS_FOR_NULL_LOGS(void)
{
    for (uint32_t i{0u}; i < LOG_CATEGORIES_COUNT; i++) {
        CHECK(circular_buffer_array[i]->log_entries != NULL);
    }
}

void CHECK_ALL_FLAGS(void)
{
    CHECK(runtime_error_asserted == false);
    CHECK(user_error_handler_set == false);
}

void CHECK_FOR_ZERO_CAPACITY_LOGS(void)
{
    for (uint32_t i{0u}; i < LOG_CATEGORIES_COUNT; i++) {
        CHECK(circular_buffer_array[log_category_array[i]]->log_capacity != 0);
    }
}

void add_one_entry(enum log_category log_index, struct log_entry expected)
{
    runtime_functions[log_index](expected.timestamp, expected.fail_message,
                                 expected.fail_value);
}

struct log_entry create_one_dummy_entry(uint32_t timestamp,
                                        const char *fail_message,
                                        uint32_t fail_value)
{
    struct log_entry dummy_entry{timestamp, fail_message, fail_value};

    return dummy_entry;
}

void add_n_entries(enum log_category log_index, uint32_t n)
{
    for (uint32_t i{0u}; i < n; i++) {
        add_one_entry(log_index,
                      create_one_dummy_entry(i, "some_file.c: msg", i + 1));
    }
}

void overflow_log(enum log_category log_index, uint32_t overflow_entries_count)
{
    struct circular_buffer *target_cb{circular_buffer_array[log_index]};
    uint32_t log_capacity{target_cb->log_capacity};
    uint32_t new_entries_count{log_capacity + overflow_entries_count};

    add_n_entries(log_index, new_entries_count);
}

void overflow_log_and_create_expected(enum log_category log_index,
                                      struct log_entry *expected,
                                      uint32_t overflow_entries_count)
{
    struct circular_buffer *target_cb{circular_buffer_array[log_index]};
    uint32_t log_capacity{target_cb->log_capacity};
    uint32_t new_entries_count{log_capacity + overflow_entries_count};

    uint32_t start_recording_index{new_entries_count - log_capacity};
    struct log_entry new_entry{0};
    for (uint32_t i{0u}; i < new_entries_count; i++) {
        new_entry = create_one_dummy_entry(i, "some_file.c: some msg", i + 1);
        add_one_entry(log_index, new_entry);
        if (i >= start_recording_index) {
            expected[i - start_recording_index] = new_entry;
        }
    }
}

void CHECK_LOG_ENTRY_EQUAL(struct log_entry expected, struct log_entry actual)
{
    CHECK_EQUAL(expected.timestamp, actual.timestamp);
    STRCMP_EQUAL(expected.fail_message, actual.fail_message);
    CHECK_EQUAL(expected.fail_value, actual.fail_value);
}

void ADD_ONE_ENTRY_AND_CHECK(enum log_category log_index,
                             struct log_entry expected)
{
    add_one_entry(log_index, expected);

    log_entry new_entry{get_entry_at_index(
        log_index, circular_buffer_array[log_index]->current_size - 1)};
    CHECK_LOG_ENTRY_EQUAL(expected, new_entry);
}

void ADD_N_ENTRIES_AND_CHECK(enum log_category log_index, uint32_t n)
{
    for (uint32_t i{0u}; i < n; i++) {
        ADD_ONE_ENTRY_AND_CHECK(
            log_index, create_one_dummy_entry(i, "some_file.c: msg", i + 1));
    }
}

void COMPARE_LOG_AND_EXPECTED(enum log_category log_index,
                              struct log_entry *expected)
{
    for (uint32_t i{0u}; i < circular_buffer_array[log_index]->log_capacity;
         i++) {
        struct log_entry actual_entry{get_entry_at_index(log_index, i)};
        CHECK_LOG_ENTRY_EQUAL(expected[i], actual_entry);
    }
}

void dummy_callback_function(void)
{
    dummy_error_callback_called = true;
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
    enum {
        LOG_ENTRY_MAX_SIZE = 256 // arbitrary max- 256 bytes is plenty to cover any log entry
    };
    char actual_log_entry[LOG_ENTRY_MAX_SIZE];
    char expected_log_entry[LOG_ENTRY_MAX_SIZE];

    uint32_t entry_index{0u};
    while (fgets(actual_log_entry, sizeof(actual_log_entry), file) != NULL) {
        log_entry raw_entry{get_entry_at_index(log_index, entry_index)};
        snprintf(expected_log_entry, sizeof(expected_log_entry),
                 "%" PRIu32 " %s %" PRIu32 "\r\n", raw_entry.timestamp,
                 raw_entry.fail_message, raw_entry.fail_value);
        STRCMP_EQUAL(expected_log_entry, actual_log_entry);
        entry_index++;
    }
}

void PRINT_LOG_TO_FILE_AND_CHECK_FILE(enum log_category log_index,
                                      void (*print_function)(void))
{
    redirect_stdout_to_file();
    print_function();
    restore_stdout();
    FILE *file{fopen(TEST_OUTPUT_FILE, "r")};
    CHECK(file != nullptr);
    CHECK(!is_file_empty(file));

    COMPARE_LOG_AND_FILE(file, log_index);
}

void CHECK_RUNTIME_ERROR_FLAG_ASSERTED(void)
{
    CHECK(runtime_error_asserted == true);
}

void CHECK_ERROR_CALLBACK_CALLED_FLAG_ASSERTED(void)
{
    CHECK(dummy_error_callback_called == true);
}

/*============================================================================*/
/*                                 Test Group                                 */
/*============================================================================*/
TEST_GROUP(RuntimeDiagnosticsTest)
{
    void setup() override
    {
        redirect_stdout_to_file();
        clear_all_test_files();
        dummy_error_callback_called = false;
        init_runtime_diagnostics();
    }

    void teardown() override
    {
        deinit_runtime_diagnostics();
        dummy_error_callback_called = false;
        clear_all_test_files();
        restore_stdout();
    }
};

/*============================================================================*/
/*                                    Tests                                   */
/*============================================================================*/
TEST(RuntimeDiagnosticsTest, LogsAreClearedOnInit)
{
    init_runtime_diagnostics();
    print_all_logs_and_expect_empty_output();
}

TEST(RuntimeDiagnosticsTest, LogsAreClearedOnDeinit)
{
    deinit_runtime_diagnostics();
    print_all_logs_and_expect_empty_output();
}

TEST(RuntimeDiagnosticsTest, AddOneEntryToTelemetryLogOnly)
{
    RUNTIME_TELEMETRY(1, "some_file.c: telemetry msg", 2);
    add_log_entry_to_expectations_file(1, "some_file.c: telemetry msg", 2);
    printf_telemetry_log();
    fflush(stdout);
    CHECK(test_output_and_expectation_are_identical());
    clear_all_test_files();
    print_all_logs();
    CHECK(test_output_and_expectation_are_identical());
}

TEST(RuntimeDiagnosticsTest, AddOneEntryToWarningLogOnly)
{
    RUNTIME_WARNING(1, "some_file.c: warning message", 2);
    add_log_entry_to_expectations_file(1, "some_file.c: warning message", 2);
    printf_warning_log();
    fflush(stdout);
    CHECK(test_output_and_expectation_are_identical());
    clear_all_test_files();
    print_all_logs();
    CHECK(test_output_and_expectation_are_identical());
}

TEST(RuntimeDiagnosticsTest, AddOneEntryToErrorLogOnly)
{
    RUNTIME_ERROR(1, "some_file.c: error message", 2);
    add_log_entry_to_expectations_file(1, "some_file.c: error message", 2);
    printf_error_log();
    fflush(stdout);
    CHECK(test_output_and_expectation_are_identical());
    clear_all_test_files();
    print_all_logs();
    CHECK(test_output_and_expectation_are_identical());
}

TEST(RuntimeDiagnosticsTest, AddOneLessThanMaxEntriesToTelemetryLog)
{
    for (uint32_t i{0u}; i < TELEMETRY_LOG_CAPACITY - 1; i++) {
        RUNTIME_TELEMETRY(i, "some_file.c: telemetry msg", i + 1);
        add_log_entry_to_expectations_file(i, "some_file.c: telemetry msg", i + 1);
    }
    printf_telemetry_log();
    fflush(stdout);
    CHECK(test_output_and_expectation_are_identical());
}

TEST(RuntimeDiagnosticsTest, AddMaxEntriesToTelemetryLog)
{
    for (uint32_t i{0u}; i < TELEMETRY_LOG_CAPACITY; i++) {
        RUNTIME_TELEMETRY(i, "some_file.c: telemetry msg", i + 1);
        add_log_entry_to_expectations_file(i, "some_file.c: telemetry msg", i + 1);
    }
    printf_telemetry_log();
    fflush(stdout);
    CHECK(test_output_and_expectation_are_identical());
}

TEST(RuntimeDiagnosticsTest, OverflowEntriesToTelemetryLog)
{
    uint32_t overflow_entries_count{17u}; // arbitrary prime number
    uint32_t total_entries{overflow_entries_count + TELEMETRY_LOG_CAPACITY};
    uint32_t start_recording_index{overflow_entries_count};
    for (uint32_t i{0u}; i < total_entries; i++) {
        RUNTIME_TELEMETRY(i, "some_file.c: telemetry msg", i + 1);
        if (i >= start_recording_index) {
            add_log_entry_to_expectations_file(i, "some_file.c: telemetry msg", i + 1);
        }
    }
    printf_telemetry_log();
    fflush(stdout);
    CHECK(test_output_and_expectation_are_identical());
}

TEST(RuntimeDiagnosticsTest, ErrorRuntimeFunctionAssertsFlag)
{
    add_one_entry(ERROR_LOG_INDEX,
                  create_one_dummy_entry(1, "some_file.c: error message", 2));
    CHECK_RUNTIME_ERROR_FLAG_ASSERTED();
}

TEST(RuntimeDiagnosticsTest, ErrorRuntimeFunctionCallsCallbackWhenSet)
{
    set_error_handler_function(dummy_callback_function);
    add_one_entry(ERROR_LOG_INDEX,
                  create_one_dummy_entry(1, "some_file.c: error message", 2));
    CHECK_ERROR_CALLBACK_CALLED_FLAG_ASSERTED();
}

TEST(RuntimeDiagnosticsTest, FullWarningLogAssertsErrorAndCallsCallback)
{
    set_error_handler_function(dummy_callback_function);
    for (uint32_t i{0u}; i < WARNING_LOG_CAPACITY; i++) {
        add_one_entry(
            WARNING_LOG_INDEX,
            create_one_dummy_entry(i, "some_file.c: warning msg", i + 1));
    }
    CHECK_RUNTIME_ERROR_FLAG_ASSERTED();
    CHECK_ERROR_CALLBACK_CALLED_FLAG_ASSERTED();
}

TEST(RuntimeDiagnosticsTest, FirstErrorIsSavedFromErrorFunctionCall)
{
    struct log_entry expected{
        create_one_dummy_entry(1, "some_file.c: error message", 2)};
    add_one_entry(ERROR_LOG_INDEX, expected);
    overflow_log(ERROR_LOG_INDEX, ERROR_LOG_CAPACITY);
    CHECK_LOG_ENTRY_EQUAL(expected, first_runtime_error_cause);
}

TEST(RuntimeDiagnosticsTest, FirstErrorIsSavedFromFullWarningLog)
{
    struct log_entry expected{0};
    for (uint32_t i{0u}; i < WARNING_LOG_CAPACITY; i++) {
        struct log_entry new_entry{
            create_one_dummy_entry(i, "some_file.c: warning msg", i + 1)};
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
    add_n_entries(TELEMETRY_LOG_INDEX, TELEMETRY_LOG_CAPACITY - 1);
    PRINT_LOG_TO_FILE_AND_CHECK_FILE(TELEMETRY_LOG_INDEX, printf_telemetry_log);
}

TEST(RuntimeDiagnosticsTest, WarningLogPrintedWhenPartiallyFilled)
{
    add_n_entries(WARNING_LOG_INDEX, WARNING_LOG_CAPACITY - 1);
    PRINT_LOG_TO_FILE_AND_CHECK_FILE(WARNING_LOG_INDEX, printf_warning_log);
}

TEST(RuntimeDiagnosticsTest, ErrorLogPrintedWhenPartiallyFilled)
{
    add_n_entries(ERROR_LOG_INDEX, ERROR_LOG_CAPACITY - 1);
    PRINT_LOG_TO_FILE_AND_CHECK_FILE(ERROR_LOG_INDEX, printf_error_log);
}

TEST(RuntimeDiagnosticsTest, TelemetryLogPrintedOnOverflow)
{
    overflow_log(TELEMETRY_LOG_INDEX, TELEMETRY_LOG_CAPACITY);
    PRINT_LOG_TO_FILE_AND_CHECK_FILE(TELEMETRY_LOG_INDEX, printf_telemetry_log);
}

TEST(RuntimeDiagnosticsTest, WarningLogPrintedOnOverflow)
{
    overflow_log(WARNING_LOG_INDEX, WARNING_LOG_CAPACITY);
    PRINT_LOG_TO_FILE_AND_CHECK_FILE(WARNING_LOG_INDEX, printf_warning_log);
}

TEST(RuntimeDiagnosticsTest, ErrorLogPrintedOnOverflow)
{
    overflow_log(ERROR_LOG_INDEX, ERROR_LOG_CAPACITY);
    PRINT_LOG_TO_FILE_AND_CHECK_FILE(ERROR_LOG_INDEX, printf_error_log);
}

TEST(RuntimeDiagnosticsTest, FirstRuntTimeErrorPrinted)
{
    struct log_entry expected{
        create_one_dummy_entry(1, "some_file.c: error message", 2)};
    add_one_entry(ERROR_LOG_INDEX, expected);
    PRINT_LOG_TO_FILE_AND_CHECK_FILE(ERROR_LOG_INDEX,
                                     printf_first_runtime_error_entry);
}
