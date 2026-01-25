/*================================ FILE INFO =================================*/
/* Filename           : test_print_hello_world.cpp                            */
/*                                                                            */
/* Test implementation for print_hello_world.c                                */
/*                                                                            */
/*============================================================================*/

/*============================================================================*/
/*                               Include Files                                */
/*============================================================================*/
#include <array>
#include <cstdio>
#include <fstream>

extern "C" {
#include "print_hello_world.h"
}

#include <CppUTest/TestHarness.h>

/*============================================================================*/
/*                             Private Definitions                            */
/*============================================================================*/
namespace
{
void failWithMessageIfNull(const void *ptr, const char *message)
{
    if (ptr == nullptr) {
        FAIL(message);
    }
}
}

/*============================================================================*/
/*                                 Test Group                                 */
/*============================================================================*/
TEST_GROUP(PrintHelloTest)
{
    FILE *standardOutput{nullptr};

    void setup() override
    {
        standardOutput = stdout;
        FILE *spyOutput = freopen("test_output.txt", "w+", stdout);
        failWithMessageIfNull(spyOutput, 
            "Failed to redirect stdout to test_output.txt");
    }

    void teardown() override
    {
        failWithMessageIfNull(stdout, "stdout is nullptr");
        fclose(stdout);
        FILE *restoredOutput = freopen("CON", "w", standardOutput);
        failWithMessageIfNull(restoredOutput,
            "Failed to restore stdout to console");
    }
};

/*============================================================================*/
/*                                    Tests                                   */
/*============================================================================*/
TEST(PrintHelloTest, PrintsHelloWorld)
{
    constexpr std::size_t MAX_BUFFER_SIZE{128};
    std::array<char, MAX_BUFFER_SIZE> buffer{};

    printHelloWorld();

    fflush(stdout);

    FILE *file = fopen("test_output.txt", "r");
    failWithMessageIfNull(file, "Failed to open test output file");

    fread(buffer.data(), sizeof(char), buffer.size(), file);
    fclose(file);

    STRCMP_EQUAL("Hello World\r\n", buffer.data());
}
