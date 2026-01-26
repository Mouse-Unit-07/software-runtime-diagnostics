/*================================ FILE INFO =================================*/
/* Filename           : test_runtime_diagnostics.cpp                          */
/*                                                                            */
/* Test implementation for runtime_diagnostics.c                              */
/*                                                                            */
/*============================================================================*/

/* scratch notes- a list of tests:
- does the telemetry log initialize to 0?
- do all 3 logs initialize to 0's?
- does the logging function append the right data to a buffer?
- does the logging function continue to append the right data when called continuously?
- is the right data stored to each buffer when called w/ different run levels?
- is the right data stored to each buffer when continuously called w/ different run levels?
- does calling each macro add a new entry to the buffer?
    - RUNTIME_TELEMETRY
    - RUNTIME_WARNING
    - RUNTIME_ERROR
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