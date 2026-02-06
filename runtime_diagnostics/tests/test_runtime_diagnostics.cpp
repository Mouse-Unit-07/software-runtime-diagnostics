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

void (*runtime_functions[])(
        uint32_t timestamp, const char *fail_message, uint32_t fail_value){
    RUNTIME_TELEMETRY, RUNTIME_WARNING, RUNTIME_ERROR
};

void (*print_functions[])(void){
    printf_telemetry_log, printf_warning_log, printf_error_log
};

uint32_t log_capacities_array[] = {
    TELEMETRY_LOG_CAPACITY, WARNING_LOG_CAPACITY, ERROR_LOG_CAPACITY
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

void dummy_callback_function(void)
{
    dummy_error_callback_called = true;
}

void check_dummy_callback_called_flag_asserted(void)
{
    CHECK(dummy_error_callback_called == true);
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
    add_n_entries_to_log_and_check(1, TELEMETRY_LOG_INDEX);
    clear_all_test_files();
    print_all_logs();
    CHECK(test_output_and_expectation_are_identical());
}

TEST(RuntimeDiagnosticsTest, AddOneEntryToWarningLogOnly)
{
    add_n_entries_to_log_and_check(1, WARNING_LOG_INDEX);
    clear_all_test_files();
    print_all_logs();
    CHECK(test_output_and_expectation_are_identical());
}

TEST(RuntimeDiagnosticsTest, AddOneEntryToErrorLogOnly)
{
    add_n_entries_to_log_and_check(1, ERROR_LOG_INDEX);
    clear_all_test_files();
    print_all_logs();
    CHECK(test_output_and_expectation_are_identical());
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
    uint32_t overflow_entries_count{17u}; // arbitrary prime number
    uint32_t total_entries{overflow_entries_count + TELEMETRY_LOG_CAPACITY};
    uint32_t start_recording_index{overflow_entries_count};
    for (uint32_t i{0u}; i < total_entries; i++) {
        RUNTIME_TELEMETRY(i, "some_file.c: telemetry msg", i + 1);
        if (i >= start_recording_index) {
            add_log_entry_to_expectations_file(i, "some_file.c: telemetry msg", i + 1);
        }
    }
    print_log(TELEMETRY_LOG_INDEX);
    CHECK(test_output_and_expectation_are_identical());
}

TEST(RuntimeDiagnosticsTest, ErrorRuntimeFunctionCallsCallbackWhenSet)
{
    set_error_handler_function(dummy_callback_function);
    RUNTIME_ERROR(1, "some_file.c: error message", 2);
    check_dummy_callback_called_flag_asserted();
}

TEST(RuntimeDiagnosticsTest, FullWarningLogAssertsErrorAndCallsCallback)
{
    set_error_handler_function(dummy_callback_function);
    for (uint32_t i{0u}; i < WARNING_LOG_CAPACITY; i++) {
        RUNTIME_WARNING(i, "some_file.c: warning msg", i + 1);
    }
    check_dummy_callback_called_flag_asserted();
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

TEST(RuntimeDiagnosticsTest, FirstErrorIsSavedFromFullWarningLog)
{
    for (uint32_t i{0u}; i < WARNING_LOG_CAPACITY; i++) {
        RUNTIME_WARNING(i, "some_file.c: warning msg", i + 1);
        if (i == (WARNING_LOG_CAPACITY - 1)) {
            add_log_entry_to_expectations_file(i, "some_file.c: warning msg", i + 1);
        }
    }
    for (uint32_t i{0u}; i < WARNING_LOG_CAPACITY; i++) {
        RUNTIME_WARNING(i + 1, "some_file.c: warning msg", i + 2);
    }

    printf_first_runtime_error_entry();
    fflush(stdout);
    CHECK(test_output_and_expectation_are_identical());
}
