/*================================ FILE INFO =================================*/
/* Filename           : test_runtime_diagnostics.cpp                          */
/*                                                                            */
/* Test implementation for runtime_diagnostics.c                              */
/*                                                                            */
/*============================================================================*/
/* scratch notes- a list of tests:
- does the init/deinit functions do what they're supposed to do?
- does the logging function continue to append the right data when called continuously?
- does calling RUNTIME_ERROR assert the error flag?
- is the error callback function not called when it's not set yet?
- does calling RUNTIME_ERROR call the error callback function when it's set?
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
  #include "runtime_diagnostics_test_access.h"
}

#include <cstdint>
#include <CppUTest/TestHarness.h>

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
    struct log_entry *actual_telemetry_log = get_telemetry_log();
    uint8_t expected[sizeof(*actual_telemetry_log)] = {0};

    MEMCMP_EQUAL(expected, actual_telemetry_log, sizeof(*actual_telemetry_log));
}

TEST(RuntimeDiagnosticsTest, WarningLogIsInitializedToZero)
{
    struct log_entry *actual_warning_log = get_warning_log();
    uint8_t expected[sizeof(*actual_warning_log)] = {0};

    MEMCMP_EQUAL(expected, actual_warning_log, sizeof(*actual_warning_log));
}

TEST(RuntimeDiagnosticsTest, ErrorLogIsInitializedToZero)
{
    struct log_entry *actual_error_log = get_error_log();
    uint8_t expected[sizeof(*actual_error_log)] = {0};

    MEMCMP_EQUAL(expected, actual_error_log, sizeof(*actual_error_log));
}

TEST(RuntimeDiagnosticsTest, AddOneEntryToTelemetryLog)
{
    struct log_entry *actual_telemetry_log = get_telemetry_log();
    struct log_entry expected = { 1, "test_runtime_diagnostics.cpp: new telemetry", 2 };
    RUNTIME_TELEMETRY(expected.timestamp, expected.fail_message, expected.fail_value);

    CHECK_LOG_ENTRY_EQUAL(expected, actual_telemetry_log[0]);
}

TEST(RuntimeDiagnosticsTest, AddOneEntryToWarningLog)
{
    struct log_entry *actual_warning_log = get_warning_log();
    struct log_entry expected = { 3, "test_runtime_diagnostics.cpp: new warning", 4 };
    RUNTIME_WARNING(expected.timestamp, expected.fail_message, expected.fail_value);

    CHECK_LOG_ENTRY_EQUAL(expected, actual_warning_log[0]);
}

TEST(RuntimeDiagnosticsTest, AddOneEntryToErrorLog)
{
    struct log_entry *actual_error_log = get_error_log();
    struct log_entry expected = { 5, "test_runtime_diagnostics.cpp: new error", 6 };
    RUNTIME_ERROR(expected.timestamp, expected.fail_message, expected.fail_value);

    CHECK_LOG_ENTRY_EQUAL(expected, actual_error_log[0]);
}

TEST(RuntimeDiagnosticsTest, AddThreeEntriesToTelemetryLog)
{
    struct log_entry *actual_telemetry_log = get_telemetry_log();
    struct log_entry expected_1 = { 7, "first telemetry", 8 };
    struct log_entry expected_2 = { 9, "second telemetry", 10 };
    struct log_entry expected_3 = { 11, "third telemetry", 12 };
    RUNTIME_TELEMETRY(expected_1.timestamp, expected_1.fail_message, expected_1.fail_value);
    RUNTIME_TELEMETRY(expected_2.timestamp, expected_2.fail_message, expected_2.fail_value);
    RUNTIME_TELEMETRY(expected_3.timestamp, expected_3.fail_message, expected_3.fail_value);

    CHECK_LOG_ENTRY_EQUAL(expected_1, actual_telemetry_log[0]);
    CHECK_LOG_ENTRY_EQUAL(expected_2, actual_telemetry_log[1]);
    CHECK_LOG_ENTRY_EQUAL(expected_3, actual_telemetry_log[2]);
}