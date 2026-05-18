/*================================ FILE INFO =================================*/
/* Filename           : test_runtime_diagnostics.cpp                          */
/*                                                                            */
/* Test implementation for runtime_diagnostics.c                              */
/*                                                                            */
/*============================================================================*/

/*============================================================================*/
/*                               Include Files                                */
/*============================================================================*/
extern "C"
{

#include <inttypes.h>
#include <stdint.h>
#include "runtime_diagnostics.h"

}

#include <CppUTest/TestHarness.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>

/*============================================================================*/
/*                             Public Definitions                             */
/*============================================================================*/
volatile bool dummy_error_callback_called{false};

FILE *standard_output{nullptr};
constexpr const char *TEST_OUTPUT_FILE{"test_output.txt"};
constexpr const char *TEST_EXPECTATIONS_FILE{"test_expectations.txt"};

enum log_category
{
    TELEMETRY_LOG_INDEX = 0,
    WARNING_LOG_INDEX,
    ERROR_LOG_INDEX,
    LOG_CATEGORIES_COUNT
};

void (*runtime_functions[])(uint32_t timestamp, const char *fail_message, uint32_t fail_value) = {
    RUNTIME_TELEMETRY, RUNTIME_WARNING, RUNTIME_ERROR};

uint32_t (*get_log_current_size_functions[])(void) = {
    get_telemetry_log_current_size, get_warning_log_current_size, get_error_log_current_size};

void (*print_functions[])(void) = {printf_telemetry_log, printf_warning_log, printf_error_log};

uint32_t log_capacities_array[] = {TELEMETRY_LOG_CAPACITY, WARNING_LOG_CAPACITY,
                                   ERROR_LOG_CAPACITY};

void redirect_stdout_to_file(void)
{
    standard_output = stdout;
    CHECK(freopen(TEST_OUTPUT_FILE, "w+", stdout) != nullptr);
}

void restore_stdout(void)
{
    fflush(stdout);
    freopen("CON", "w", stdout);
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
    FILE *file{fopen(TEST_OUTPUT_FILE, "w")};
    CHECK(file != nullptr);
    fclose(file);
    file = fopen(TEST_EXPECTATIONS_FILE, "w");
    CHECK(file != nullptr);
    fclose(file);
}

void add_log_entry_to_expectations_file(uint32_t timestamp, const char *fail_message,
                                        uint32_t fail_value)
{
    FILE *file{fopen(TEST_EXPECTATIONS_FILE, "a")};
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
    FILE *a{fopen(TEST_OUTPUT_FILE, "rb")};
    FILE *b{fopen(TEST_EXPECTATIONS_FILE, "rb")};

    if (!a || !b) {
        if (a) {
            fclose(a);
        }
        if (b) {
            fclose(b);
        }
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
    printf_error_log();
}

void print_all_logs_and_expect_empty_output(void)
{
    print_all_logs();
    fflush(stdout);
    CHECK(is_test_file_empty());
}

void dummy_callback_function(void)
{
    dummy_error_callback_called = true;
}

void add_n_entries_to_log_and_expectations(uint32_t n, enum log_category index)
{
    for (uint32_t i{0u}; i < n; i++) {
        runtime_functions[index](i, "some_file.c: some msg", i + 1);
        add_log_entry_to_expectations_file(i, "some_file.c: some msg", i + 1);
    }
}

void print_log(enum log_category index)
{
    print_functions[index]();
    fflush(stdout);
}

void add_n_entries_to_log_and_check(uint32_t n, enum log_category index)
{
    add_n_entries_to_log_and_expectations(n, index);
    print_log(index);
    CHECK(test_output_and_expectation_are_identical());
}

void check_an_entry_added_to_one_log_only(enum log_category index)
{
    add_n_entries_to_log_and_check(1, index);
    clear_all_test_files();
    print_all_logs();
    CHECK(test_output_and_expectation_are_identical());
}

void overflow_by_n_entries_and_check(uint32_t n, enum log_category index)
{
    uint32_t overflow_entries_count{n};
    uint32_t total_entries{overflow_entries_count + log_capacities_array[index]};
    uint32_t start_recording_index{overflow_entries_count};
    for (uint32_t i{0u}; i < total_entries; i++) {
        runtime_functions[index](i, "some_file.c: some msg", i + 1);
        if (i >= start_recording_index) {
            add_log_entry_to_expectations_file(i, "some_file.c: some msg", i + 1);
        }
    }
    print_log(index);
    CHECK(test_output_and_expectation_are_identical());
}

void check_log_initial_size_is_zero(enum log_category index)
{
    CHECK_EQUAL(0u, get_log_current_size_functions[index]());
}

void add_n_entries_and_check_log_size(uint32_t n, uint32_t expected_size, enum log_category index)
{
    for (uint32_t i{0u}; i < n; i++) {
        runtime_functions[index](i, "some_file.c: some msg", i + 1);
    }

    CHECK_EQUAL(expected_size, get_log_current_size_functions[index]());
}

void check_log_size_saturates_at_capacity(enum log_category index)
{
    const uint32_t overflow_count{107u};

    add_n_entries_and_check_log_size(log_capacities_array[index] + overflow_count,
                                     log_capacities_array[index], index);
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
    check_an_entry_added_to_one_log_only(TELEMETRY_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, AddOneEntryToWarningLogOnly)
{
    check_an_entry_added_to_one_log_only(WARNING_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, AddOneEntryToErrorLogOnly)
{
    check_an_entry_added_to_one_log_only(ERROR_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, AddOneLessThanMaxEntriesToTelemetryLog)
{
    add_n_entries_to_log_and_check(TELEMETRY_LOG_CAPACITY - 1, TELEMETRY_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, AddOneLessThanMaxEntriesToWarningLog)
{
    add_n_entries_to_log_and_check(WARNING_LOG_CAPACITY - 1, WARNING_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, AddOneLessThanMaxEntriesToErrorLog)
{
    add_n_entries_to_log_and_check(ERROR_LOG_CAPACITY - 1, ERROR_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, AddMaxEntriesToTelemetryLog)
{
    add_n_entries_to_log_and_check(TELEMETRY_LOG_CAPACITY, TELEMETRY_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, AddMaxEntriesToWarningLog)
{
    add_n_entries_to_log_and_check(WARNING_LOG_CAPACITY, WARNING_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, AddMaxEntriesToErrorLog)
{
    add_n_entries_to_log_and_check(ERROR_LOG_CAPACITY, ERROR_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, OverflowEntriesToTelemetryLog)
{
    // overflowing by arbitrary prime number
    overflow_by_n_entries_and_check(107u, TELEMETRY_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, OverflowEntriesToWarningLog)
{
    // overflowing by arbitrary prime number
    overflow_by_n_entries_and_check(107u, WARNING_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, OverflowEntriesToErrorLog)
{
    // overflowing by arbitrary prime number
    overflow_by_n_entries_and_check(107u, ERROR_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, ErrorRuntimeFunctionCallsHandlerWhenSet)
{
    set_error_handler(dummy_callback_function);
    RUNTIME_ERROR(1, "some_file.c: error message", 2);
    CHECK(dummy_error_callback_called);
}

TEST(RuntimeDiagnosticsTest, ErrorHandlerCalledWhenErrorAlreadyAsserted)
{
    RUNTIME_ERROR(1, "some_file.c: error message", 2);
    CHECK(!dummy_error_callback_called);
    set_error_handler(dummy_callback_function);
    CHECK(dummy_error_callback_called);
}

TEST(RuntimeDiagnosticsTest, FullWarningLogCallsHandlerWhenSet)
{
    set_warning_handler(dummy_callback_function);
    for (uint32_t i{0u}; i < WARNING_LOG_CAPACITY; i++) {
        RUNTIME_WARNING(i, "some_file.c: warning msg", i + 1);
    }
    CHECK(dummy_error_callback_called);
}

TEST(RuntimeDiagnosticsTest, WarningHandlerCalledWhenWarningLogAlreadyFull)
{
    for (uint32_t i{0u}; i < WARNING_LOG_CAPACITY; i++) {
        RUNTIME_WARNING(i, "some_file.c: warning msg", i + 1);
    }
    CHECK(!dummy_error_callback_called);
    set_warning_handler(dummy_callback_function);
    CHECK(dummy_error_callback_called);
}

TEST(RuntimeDiagnosticsTest, FirstErrorNotPrintedIfErrorFunctionNeverCalled)
{
    printf_first_runtime_error_entry();
    fflush(stdout);
    CHECK(is_test_file_empty());
}

TEST(RuntimeDiagnosticsTest, FirstErrorIsSavedFromErrorFunctionCall)
{
    RUNTIME_ERROR(0, "some_file.c: error message", 0);
    add_log_entry_to_expectations_file(0, "some_file.c: error message", 0);
    for (uint32_t i{0u}; i < ERROR_LOG_CAPACITY; i++) {
        RUNTIME_ERROR(i + 1, "some_file.c: error msg", i + 2);
    }
    printf_first_runtime_error_entry();
    fflush(stdout);
    CHECK(test_output_and_expectation_are_identical());
}

TEST(RuntimeDiagnosticsTest, RuntimeFunctionCallCountsAreKept)
{
    RUNTIME_ERROR(0, "some_file.c: error message", 0);
    RUNTIME_WARNING(0, "some_file.c: warning message", 0);
    RUNTIME_WARNING(0, "some_file.c: warning message", 0);
    RUNTIME_TELEMETRY(0, "some_file.c: telemetry message", 0);
    RUNTIME_TELEMETRY(0, "some_file.c: telemetry message", 0);
    RUNTIME_TELEMETRY(0, "some_file.c: telemetry message", 0);

    FILE *file{fopen(TEST_EXPECTATIONS_FILE, "w")};
    CHECK(file != nullptr);

    CHECK(fprintf(file, "telemetry: 3\r\n") > 0);
    CHECK(fprintf(file, "warning: 2\r\n") > 0);
    CHECK(fprintf(file, "error: 1\r\n") > 0);

    fclose(file);

    printf_call_counts();
    fflush(stdout);

    CHECK(test_output_and_expectation_are_identical());
}

TEST(RuntimeDiagnosticsTest, TelemetryLogInitialSizeIsZero)
{
    check_log_initial_size_is_zero(TELEMETRY_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, WarningLogInitialSizeIsZero)
{
    check_log_initial_size_is_zero(WARNING_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, ErrorLogInitialSizeIsZero)
{
    check_log_initial_size_is_zero(ERROR_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, TelemetryLogSizeIncrementsCorrectly)
{
    add_n_entries_and_check_log_size(5u, 5u, TELEMETRY_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, WarningLogSizeIncrementsCorrectly)
{
    add_n_entries_and_check_log_size(5u, 5u, WARNING_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, ErrorLogSizeIncrementsCorrectly)
{
    add_n_entries_and_check_log_size(5u, 5u, ERROR_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, TelemetryLogSizeSaturatesAtCapacity)
{
    check_log_size_saturates_at_capacity(TELEMETRY_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, WarningLogSizeSaturatesAtCapacity)
{
    check_log_size_saturates_at_capacity(WARNING_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, ErrorLogSizeSaturatesAtCapacity)
{
    check_log_size_saturates_at_capacity(ERROR_LOG_INDEX);
}

TEST(RuntimeDiagnosticsTest, InitResetsAllLogSizesToZero)
{
    RUNTIME_TELEMETRY(0, "msg", 0);
    RUNTIME_WARNING(0, "msg", 0);
    RUNTIME_ERROR(0, "msg", 0);

    CHECK(get_telemetry_log_current_size() > 0u);
    CHECK(get_warning_log_current_size() > 0u);
    CHECK(get_error_log_current_size() > 0u);

    init_runtime_diagnostics();

    CHECK_EQUAL(0u, get_telemetry_log_current_size());
    CHECK_EQUAL(0u, get_warning_log_current_size());
    CHECK_EQUAL(0u, get_error_log_current_size());
}

TEST(RuntimeDiagnosticsTest, DeinitResetsAllLogSizesToZero)
{
    RUNTIME_TELEMETRY(0, "msg", 0);
    RUNTIME_WARNING(0, "msg", 0);
    RUNTIME_ERROR(0, "msg", 0);

    CHECK(get_telemetry_log_current_size() > 0u);
    CHECK(get_warning_log_current_size() > 0u);
    CHECK(get_error_log_current_size() > 0u);

    deinit_runtime_diagnostics();

    CHECK_EQUAL(0u, get_telemetry_log_current_size());
    CHECK_EQUAL(0u, get_warning_log_current_size());
    CHECK_EQUAL(0u, get_error_log_current_size());
}
