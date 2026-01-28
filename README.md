# Runtime Diagnostics

- A lightweight library to replace:
  - Functions that return statuses (statuses are dependency magnets)
  - Runtime logic to handle/offset errors/warnings/note taking (passing null, passing out of range values, etc)

## Overview

- You can log:
  - Notes w/ `RUNTIME_TELEMETRY`
  - Recoverable issues (those that are "unsettling" when repeated too many times) w/ `RUNTIME_WARNING`
  - Unrecoverable errors w/ `RUNTIME_ERROR`
- Every log entry:
  - Has a timestamp, a string message, and error value
  - Takes 16 Bytes (rounded up from 12 Bytes) of memory
- Error handling
  - You can set the error callback function to stop your system in response to an unrecoverable failure
  - The callback is called in response to the first `RUNTIME_ERROR` call, and to the warning log reaching capacity (treated as an unrecoverable error)
  - The first log entry that flags an unrecoverable error is saved separately
- Capacity
  - There's a hardcoded limit to the max number of log entries you can add per log category
  - These capacities can be modified in firmware as needed, but not at runtime
- Overwriting
  - All logs are circular, where old entries will be overwritten
    - Telemetry entries will silenty be overwritten
    - The user callback will be called before warning entries begin to be overwritten
    - User callback will be called in response to the first runtime error entry, and following error entries will be logged (and will be overwritten) 
- Printing
  - Each log category can be printed
  - The first log entry that flags an unrecoverable error can be printed
  - Log printing functions are implemeneted w/ standard `printf`
