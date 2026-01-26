# Runtime Diagnostics

- A library w/ macros for logging any runtime telemetry, warnings, and errors
  - ...Yes, the one time preprocessor macros help make cleaner code
  - Macros are defined instead of functions to sprinkle predefined `__file__` and `__line__` macros everywhere
- An optional source implementation is provided for printing and flushing the log
