# Implementation Plan: UART-Telnet Bridge

## Overview

Incremental implementation of the UART-Telnet Bridge in C++. Each task builds on the previous, starting with data models and interfaces, then core logic (ConfigParser, AutoResponder, BridgeController), then platform implementations, build system, and integration wiring. Tests are placed close to the code they validate. All code uses camelCase naming, Doxygen documentation, gtest/gmock for unit tests, and RapidCheck for property-based tests.

## Tasks

- [x] 1. Create project structure, data models, and platform interfaces
  - [x] 1.1 Create directory structure and CMakeLists.txt skeleton
    - Create top-level `CMakeLists.txt` with PLATFORM cache variable, core library target, platform subdirectory inclusion, test subdirectory, and main executable target
    - Create `include/interfaces/`, `include/core/`, `src/core/`, `src/platform/linux/`, `src/platform/esp32/`, `test/mocks/` directories via placeholder files or CMake
    - Create `test/CMakeLists.txt` that fetches or finds gtest, gmock, and RapidCheck, and defines the test executable target
    - _Requirements: 8.5, 8.6, 10.1, 10.7_

  - [x] 1.2 Define data model structs in `include/core/DataModels.h`
    - Implement `UartConfig`, `WiFiConfig`, `PromptRule`, `PostCommand`, `CommandSequence`, `BridgeConfig`, `DisplayStatus`, `HttpResponse` structs with Doxygen comments
    - All field names use camelCase
    - _Requirements: 1.1, 4.1, 4.6, 4.7, 5.6, 7.1, 9.1_

  - [x] 1.3 Define platform interface abstract classes
    - Create `include/interfaces/UartModule.h` — pure virtual `UartModule` class with `open`, `close`, `write`, `setOnDataCallback`, `isOpen`, `reopen` methods
    - Create `include/interfaces/TelnetServer.h` — pure virtual `TelnetServer` class with `start`, `stop`, `broadcast`, `setOnClientDataCallback`, `setOnClientEventCallback`, `clientCount` methods
    - Create `include/interfaces/HttpServer.h` — pure virtual `HttpServer` class with `start`, `stop`, `registerHandler` methods
    - Create `include/interfaces/DisplayModule.h` — pure virtual `DisplayModule` class with `init`, `update`, `setBacklight`, `onButtonPress` methods
    - Create `include/interfaces/WiFiModule.h` — pure virtual `WiFiModule` class with `connect`, `disconnect`, `isConnected`, `getIpAddress`, `getSignalStrengthDbm` methods
    - All interfaces documented with Doxygen (preconditions, postconditions, expected behavior)
    - No `I` prefix on interface names
    - _Requirements: 8.1, 8.3, 8.7_

  - [x] 1.4 Create gmock mock classes in `test/mocks/`
    - Create `MockUartModule.h`, `MockTelnetServer.h`, `MockHttpServer.h`, `MockDisplayModule.h`, `MockWiFiModule.h`
    - Each mock inherits from the corresponding interface and uses `MOCK_METHOD` macros for all virtual methods
    - _Requirements: 10.3_

- [x] 2. Implement ConfigParser
  - [x] 2.1 Implement `ConfigParser` in `include/core/ConfigParser.h` and `src/core/ConfigParser.cpp`
    - Parse a JSON file into a `BridgeConfig` struct (use RapidJSON library)
    - Validate all required fields are present; return error identifying the specific missing field
    - Parse `CommandSequence` from an HTTP request body string into a `CommandSequence` struct
    - Serialize `CommandSequence` to JSON string for HTTP GET responses
    - Doxygen documentation on all public methods
    - _Requirements: 5.2, 5.3, 5.6, 9.1, 9.2, 9.3_

  - [x] 2.2 Write property test: Config parsing round-trip (Property 11)
    - **Property 11: Configuration parsing round-trip**
    - For any valid `BridgeConfig`, serializing to JSON and parsing back produces an equivalent struct
    - Use RapidCheck custom generators for `BridgeConfig` and all nested structs
    - **Validates: Requirements 9.1**

  - [x] 2.3 Write property test: Missing configuration field detection (Property 12)
    - **Property 12: Missing configuration field detection**
    - For any valid `BridgeConfig` JSON with exactly one required field removed, `ConfigParser` reports an error identifying the missing field name
    - **Validates: Requirements 9.2**

  - [x] 2.4 Write unit tests for ConfigParser
    - Valid JSON config file parsed correctly with all fields populated
    - Missing required value produces descriptive error
    - Malformed JSON returns parse error
    - CommandSequence JSON parsing and serialization for HTTP payloads
    - _Requirements: 10.6_

- [x] 3. Implement AutoResponder
  - [x] 3.1 Implement `AutoResponder` in `include/core/AutoResponder.h` and `src/core/AutoResponder.cpp`
    - Maintain a rolling buffer of recent UART output for prompt matching
    - Match incoming UART data against the current `PromptRule` trigger; send response + lineEnding to UART via a write callback
    - Process `CommandSequence` rules in order, advancing only after current rule matches or times out
    - Execute `PostCommand` list after all rules complete, with configurable delay between commands
    - Support optional `expectedPrompt` waiting for PostCommands, falling back to delay if timeout
    - Log warning on prompt timeout containing the unmatched trigger string
    - Provide `setConfig(const CommandSequence&)` and `getConfig() const` for runtime reconfiguration
    - Support at least 20 PromptRules and 10 PostCommands
    - Doxygen documentation on all public methods
    - _Requirements: 4.1, 4.2, 4.3, 4.5, 4.6, 4.7, 4.8, 4.9, 4.10_

  - [x] 3.2 Write property test: Prompt matching sends correct response with line ending (Property 4)
    - **Property 4: Prompt matching sends correct response with line ending**
    - For any PromptRule (trigger T, response R) and any UART data containing T, AutoResponder sends exactly R + lineEnding
    - **Validates: Requirements 4.1, 4.6**

  - [x] 3.3 Write property test: CommandSequence rules processed in order (Property 5)
    - **Property 5: CommandSequence rules processed in order**
    - For any CommandSequence with N ≥ 2 rules, feeding triggers in order produces responses in the same order; rule[i+1] is not matched before rule[i]
    - **Validates: Requirements 4.2**

  - [ ]* 3.4 Write property test: Timeout skips unmatched prompt rules (Property 6)
    - **Property 6: Timeout skips unmatched prompt rules**
    - For any PromptRule whose trigger does not appear within promptTimeoutMs, AutoResponder skips it and advances to the next rule
    - **Validates: Requirements 4.5**

  - [ ]* 3.5 Write property test: PostCommands sent after all PromptRules complete (Property 7)
    - **Property 7: PostCommands sent after all PromptRules complete**
    - For any CommandSequence with N rules and M PostCommands, PostCommands are sent only after all N rules are processed, in defined order
    - **Validates: Requirements 4.7**

  - [ ]* 3.6 Write property test: PostCommand expected-prompt waiting (Property 8)
    - **Property 8: PostCommand expected-prompt waiting**
    - For any PostCommand with a non-empty expectedPrompt, AutoResponder waits for that prompt before sending the next PostCommand; falls back to delay on timeout
    - **Validates: Requirements 4.10**

  - [x] 3.7 Write unit tests for AutoResponder
    - Prompt matching triggers correct response (specific example)
    - CommandSequence rules processed in order (specific example with login/password)
    - PostCommands sent after all PromptRules complete
    - Timeout behavior skips unmatched rules and logs warning
    - Line endings appended correctly
    - Capacity: 20 PromptRules, 10 PostCommands
    - Configurable delay between PostCommands
    - Runtime config replacement via setConfig/getConfig
    - _Requirements: 10.4_

- [x] 4. Checkpoint
  - Ensure all tests pass, ask the user if questions arise.

- [x] 5. Implement BridgeController
  - [x] 5.1 Implement `BridgeController` in `include/core/BridgeController.h` and `src/core/BridgeController.cpp`
    - Accept references/pointers to all platform interfaces (UartModule, TelnetServer, HttpServer, DisplayModule, WiFiModule) and AutoResponder via constructor (dependency injection)
    - Wire UART data callback: forward bytes to AutoResponder.process() and TelnetServer.broadcast()
    - Wire Telnet client data callback: forward bytes to UartModule.write()
    - Wire HTTP handlers: POST /config updates AutoResponder config, GET /config returns current config, malformed JSON returns 400, unknown endpoint returns 404
    - Track UART RX/TX byte counts; compute bytes-per-second throughput
    - Periodically update DisplayModule with current DisplayStatus
    - Handle startup: open UART, start Telnet, start HTTP, connect WiFi, init display — terminate gracefully on fatal errors with descriptive log messages
    - Handle UART connection loss: call reopen()
    - Handle graceful shutdown: close modules in reverse initialization order
    - Doxygen documentation on all public methods
    - _Requirements: 1.1, 1.2, 1.3, 1.4, 1.5, 2.1, 2.2, 2.3, 2.4, 2.5, 3.1, 3.2, 3.3, 4.4, 5.1, 5.2, 5.3, 5.4, 5.5, 5.7, 7.3, 8.3_

  - [x] 5.2 Write property test: UART-to-Telnet data integrity (Property 1)
    - **Property 1: UART-to-Telnet data integrity**
    - For any byte sequence from UART and any number of connected clients ≥ 1, BridgeController forwards the exact bytes to all clients via broadcast
    - Use MockUartModule and MockTelnetServer
    - **Validates: Requirements 2.2, 2.3**

  - [x] 5.3 Write property test: Telnet-to-UART data integrity (Property 2)
    - **Property 2: Telnet-to-UART data integrity**
    - For any byte sequence from a Telnet client, BridgeController forwards exact bytes to UartModule.write()
    - **Validates: Requirements 3.1, 3.3**

  - [x] 5.4 Write property test: No command echo to other Telnet clients (Property 3)
    - **Property 3: No command echo to other Telnet clients**
    - For any data sent by one Telnet client, BridgeController does not broadcast that input to other clients
    - **Validates: Requirements 3.2**

  - [x] 5.5 Write property test: UART throughput computation (Property 13)
    - **Property 13: UART throughput computation**
    - For any sequence of byte counts over a time interval, computed RX/TX bytes-per-second is accurate to within 1 byte/sec
    - **Validates: Requirements 7.3**

  - [x] 5.6 Write unit tests for BridgeController
    - Startup: UART open failure logs error with port name and terminates
    - Startup: Telnet port in use logs error with port number and terminates
    - Startup: HTTP port in use logs error with port number and terminates
    - UART connection loss triggers reopen()
    - Telnet client disconnect releases resources, remaining clients unaffected
    - Telnet broadcast continues during AutoResponder processing
    - Graceful shutdown closes modules in reverse order
    - _Requirements: 1.4, 1.5, 2.4, 2.5, 4.4, 5.7, 10.2_

- [x] 6. Implement HTTP handler logic
  - [x] 6.1 Implement HTTP route handlers (can be methods on BridgeController or standalone functions)
    - POST /config: parse JSON body via ConfigParser, update AutoResponder config, return 200 on success
    - GET /config: serialize current AutoResponder config via ConfigParser, return 200 with JSON body
    - Malformed JSON: return 400 with descriptive error JSON
    - Unknown endpoint: return 404
    - Register handlers on HttpServer during startup
    - _Requirements: 5.2, 5.3, 5.4, 5.5, 5.6_

  - [x] 6.2 Write property test: HTTP configuration round-trip (Property 9)
    - **Property 9: HTTP configuration round-trip**
    - For any valid CommandSequence, POSTing to /config then GETting /config returns equivalent JSON
    - **Validates: Requirements 5.2, 5.3**

  - [x] 6.3 Write property test: HTTP error responses for invalid requests (Property 10)
    - **Property 10: HTTP error responses for invalid requests**
    - For any malformed JSON, POST /config returns 400 with error message; for any unknown path, returns 404
    - **Validates: Requirements 5.4, 5.5**

  - [x] 6.4 Write unit tests for HTTP handlers
    - Valid JSON payload updates AutoResponder configuration
    - Malformed JSON returns HTTP 400 with descriptive error
    - Unknown endpoint returns HTTP 404
    - GET returns current configuration as JSON
    - _Requirements: 10.5_

- [x] 7. Checkpoint
  - Ensure all tests pass, ask the user if questions arise.

- [x] 8. Implement PlatformFactory and Linux platform stubs
  - [x] 8.1 Implement `PlatformFactory` in `include/PlatformFactory.h` and `src/PlatformFactory.cpp`
    - Map platform identifier string (e.g., "linux", "esp32") to a set of concrete interface implementations
    - Provide a factory method that returns all module instances for the given platform
    - Doxygen documentation
    - _Requirements: 8.2, 8.4, 8.8_

  - [x] 8.2 Implement Linux platform stubs
    - `LinuxUartModule` in `src/platform/linux/LinuxUartModule.h/.cpp` — open/read/write via POSIX termios
    - `LinuxTelnetServer` in `src/platform/linux/LinuxTelnetServer.h/.cpp` — TCP server using POSIX sockets
    - `LinuxHttpServer` in `src/platform/linux/LinuxHttpServer.h/.cpp` — minimal HTTP server using POSIX sockets
    - `LinuxDisplayModule` in `src/platform/linux/LinuxDisplayModule.h/.cpp` — print IP and UART RX/TX speed to console once per second; backlight methods are no-ops
    - `LinuxWiFiStub` in `src/platform/linux/LinuxWiFiStub.h/.cpp` — isConnected() returns true, getIpAddress() returns system IP, getSignalStrengthDbm() returns 0
    - Create `src/platform/linux/CMakeLists.txt` defining `platform_linux` library
    - _Requirements: 6.5, 7.6, 8.5_

  - [x] 8.3 Write unit tests for PlatformFactory
    - Returns correct implementation types for known platform IDs
    - _Requirements: 10.2_

- [x] 9. Implement main.cpp and wire everything together
  - [x] 9.1 Implement `src/main.cpp`
    - Parse command-line argument for config file path (default: `config.json`)
    - Use ConfigParser to load BridgeConfig
    - Use PlatformFactory to create platform modules
    - Construct AutoResponder and BridgeController with all dependencies
    - Call BridgeController startup sequence
    - Run main loop (or event loop)
    - Handle signals (SIGINT/SIGTERM) for graceful shutdown
    - _Requirements: 9.1, 9.2, 9.3_

  - [x] 9.2 Create default `config.json` example file
    - Include all configuration fields with sensible defaults and comments
    - _Requirements: 9.3_

- [x] 10. Checkpoint
  - Ensure all tests pass, ask the user if questions arise.

- [x] 11. ESP32 / M5 Stack platform implementation (skeleton)
  - [x] 11.1 Create ESP32 platform implementation files
    - `Esp32UartModule` in `src/platform/esp32/Esp32UartModule.h/.cpp`
    - `Esp32TelnetServer` in `src/platform/esp32/Esp32TelnetServer.h/.cpp`
    - `Esp32HttpServer` in `src/platform/esp32/Esp32HttpServer.h/.cpp`
    - `M5StackDisplayModule` in `src/platform/esp32/M5StackDisplayModule.h/.cpp` — render WiFi status, signal strength, IP, UART RX/TX on M5 Stack screen; manage backlight timeout and button press
    - `Esp32WiFiModule` in `src/platform/esp32/Esp32WiFiModule.h/.cpp` — connect with retry logic (5s intervals, maxRetries, cycle restart), auto-reconnect on drop
    - Create `src/platform/esp32/CMakeLists.txt` defining `platform_esp32` library
    - Register ESP32 platform in PlatformFactory
    - _Requirements: 6.1, 6.2, 6.3, 6.4, 7.1, 7.2, 7.3, 7.4, 7.5, 8.2, 8.5, 8.6, 8.8_

- [x] 12. Final checkpoint
  - Ensure all tests pass, ask the user if questions arise.

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- Each task references specific requirements for traceability
- Checkpoints ensure incremental validation
- Property tests validate universal correctness properties from the design document
- Unit tests validate specific examples, edge cases, and error conditions
- All code uses camelCase naming, no `I` prefix on interfaces, Doxygen documentation
- gtest/gmock for unit tests, RapidCheck with gtest integration for property-based tests
