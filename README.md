# Runtime Diagnostics

- A lightweight library to avoid:
  - Functions that return statuses (statuses are dependency magnets)
  - Runtime logic to handle and offset errors/warnings/note taking in response to passing null, passing out of range values, etc

## Overview

- Core functions:
  - `RUNTIME_TELEMETRY()`- log notes
  - `RUNTIME_WARNING()`- log recoverable issues, and call your warning handle when capacity is reached
  - `RUNTIME_ERROR()`- log unrecoverable errors, and call your error handle in response to every call
  - All 3 functions have parameters: `uint32_t timestamp`, `const char *fail_message`, `uint32_t fail_value`
  - Every entry takes 12 bytes of memory w/ 4 byte boundaries (32-bit architecture)
- User handlers
  - `set_warning_handler(void (*handler)(void))`
    - Called when warning log reaches capacity
  - `set_error_handler(void (*handler)(void))`
    - Called in response to every `RUNTIME_ERROR()`
- Capacity
  - There's a hardcoded limit to the max number of log entries you can add per log category
  - These capacities can be modified in firmware as needed, but not at runtime
- Overwriting
  - All logs are circular- old entries will be overwritten
  - The contents of the first `RUNTIME_ERROR()` call is saved separately
- Printing
  - `printf_telemetry_log()`, `printf_warning_log()`, `printf_error_log()`
    - Each log category can be printed
  - `printf_first_runtime_error_entry()`
    - The first log entry that flags an unrecoverable error can be printed
  - `printf_call_counts()`
    - The number of times each `RUNTIME` function was called can be printed
  - Log printing functions are implemented w/ standard `printf()`
