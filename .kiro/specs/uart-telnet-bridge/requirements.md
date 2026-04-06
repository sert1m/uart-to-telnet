# Requirements Document

## Introduction

The UART-Telnet Bridge is a cross-platform C++ application that bridges a UART-connected device with remote access over Telnet and HTTP. It forwards UART logs to a Telnet client, accepts commands from Telnet and sends them to the UART device, and can automatically respond to known prompts (e.g., login sequences). Configuration is managed via an embedded HTTP server. The application uses an interface-based module architecture designed for extensibility: initial platforms are ESP32 (M5 Stack) and Linux-based systems (Ubuntu, Raspberry Pi, iMX, etc.), but the architecture must allow new platforms to be added in the future by implementing the defined interfaces without modifying core logic or existing platform code.

## Glossary

- **Bridge**: The UART-Telnet Bridge application
- **UART_Module**: The module responsible for opening, reading from, and writing to a hardware UART port
- **Telnet_Server**: The module that listens for incoming Telnet connections and relays data to/from the UART_Module
- **HTTP_Server**: The module that serves an HTTP endpoint for accepting runtime configuration
- **Auto_Responder**: The module that monitors UART output for known prompts and automatically sends preconfigured responses
- **Display_Module**: The module responsible for showing status information (platform-specific: M5 Stack screen or Linux console)
- **WiFi_Module**: The module responsible for wireless network connectivity (platform-specific: functional on M5 Stack, stub on Linux)
- **Prompt_Rule**: A configuration entry that maps a specific UART prompt string to a response string to be sent automatically
- **Command_Sequence**: An ordered list of Prompt_Rules and Post_Commands that define an automated interaction (e.g., login, password, then a set of commands to execute after login completes)
- **Post_Command**: A command string to be sent to the UART device after all Prompt_Rules in a Command_Sequence have been matched and responded to (e.g., "journalctl -f" to start streaming logs)
- **Platform_Interface**: An abstract C++ class that defines a contract for a module, with platform-specific concrete implementations

## Requirements

### Requirement 1: UART Communication

**User Story:** As a developer, I want the Bridge to communicate with a device over UART, so that I can send commands to and receive logs from the device.

#### Acceptance Criteria

1. WHEN the Bridge starts, THE UART_Module SHALL open the configured UART port at the configured baud rate, data bits, parity, and stop bits
2. WHEN data is received on the UART port, THE UART_Module SHALL make the received bytes available to other modules within 50ms
3. WHEN a module submits bytes for transmission, THE UART_Module SHALL write those bytes to the UART port in the order they were submitted
4. IF the configured UART port cannot be opened, THEN THE Bridge SHALL log an error message containing the port name and the reason for failure, and terminate gracefully
5. IF the UART connection is lost during operation, THEN THE UART_Module SHALL report the error to the Bridge and attempt to reopen the port

### Requirement 2: Telnet Log Forwarding

**User Story:** As a remote operator, I want to connect to the Bridge via Telnet and see UART device logs in real time, so that I can monitor the device without physical access.

#### Acceptance Criteria

1. WHEN the Bridge starts, THE Telnet_Server SHALL listen for incoming TCP connections on a preconfigured port
2. WHEN a Telnet client connects, THE Telnet_Server SHALL forward all bytes received from the UART_Module to the connected Telnet client in real time
3. WHEN multiple Telnet clients are connected, THE Telnet_Server SHALL forward UART data to all connected clients simultaneously
4. IF a Telnet client disconnects, THEN THE Telnet_Server SHALL release resources associated with that client and continue serving remaining clients
5. IF the Telnet listening port is already in use, THEN THE Bridge SHALL log an error message containing the port number and terminate gracefully

### Requirement 3: Telnet Command Input

**User Story:** As a remote operator, I want to type commands into the Telnet session and have them sent to the UART device, so that I can control the device remotely.

#### Acceptance Criteria

1. WHEN a Telnet client sends data, THE Telnet_Server SHALL forward the received bytes to the UART_Module for transmission to the UART device
2. WHEN multiple Telnet clients are connected and one client sends data, THE Telnet_Server SHALL forward only that client's data to the UART_Module (no echo to other clients of the command itself)
3. THE Telnet_Server SHALL preserve the byte order of data received from a Telnet client when forwarding to the UART_Module

### Requirement 4: Automatic Prompt Response

**User Story:** As a developer, I want the Bridge to automatically respond to known prompts from the UART device (e.g., login, password), so that the device can be brought to a usable state without manual intervention.

#### Acceptance Criteria

1. WHEN the UART_Module receives data containing a string that matches a configured Prompt_Rule trigger, THE Auto_Responder SHALL send the corresponding response string to the UART_Module within 500ms
2. WHEN a Command_Sequence is configured, THE Auto_Responder SHALL process Prompt_Rules in the defined order, advancing to the next rule only after the current rule's trigger has been matched and its response sent
3. THE Auto_Responder SHALL support at least 20 Prompt_Rules in a single Command_Sequence
4. WHILE the Auto_Responder is processing a Command_Sequence, THE Telnet_Server SHALL continue to forward UART output to connected Telnet clients
5. IF a Prompt_Rule trigger is not matched within a configurable timeout period, THEN THE Auto_Responder SHALL log a warning containing the unmatched trigger string and skip to the next rule in the sequence
6. WHEN the Auto_Responder sends a response, THE Auto_Responder SHALL append a configurable line ending (default: "\n") to the response string
7. WHEN all Prompt_Rules in a Command_Sequence have been processed, THE Auto_Responder SHALL send each configured Post_Command to the UART_Module in the defined order
8. THE Auto_Responder SHALL wait for a configurable delay (default: 500ms) between sending consecutive Post_Commands, to allow the UART device to process each command
9. THE Auto_Responder SHALL support at least 10 Post_Commands in a single Command_Sequence
10. WHEN a Post_Command includes an optional expected prompt, THE Auto_Responder SHALL wait for that prompt before sending the next Post_Command, falling back to the delay-based approach if the prompt is not matched within the configured timeout

### Requirement 5: HTTP Configuration Server

**User Story:** As a developer, I want to configure the automatic prompt responses via an HTTP API, so that I can update the Bridge behavior at runtime without recompiling or restarting.

#### Acceptance Criteria

1. WHEN the Bridge starts, THE HTTP_Server SHALL listen for incoming HTTP connections on a preconfigured port
2. WHEN a valid HTTP POST request is received at the configuration endpoint, THE HTTP_Server SHALL parse the JSON body and update the Auto_Responder's Prompt_Rules and Command_Sequences
3. WHEN a valid HTTP GET request is received at the configuration endpoint, THE HTTP_Server SHALL return the current Auto_Responder configuration as a JSON response with HTTP status 200
4. IF an HTTP request contains malformed JSON, THEN THE HTTP_Server SHALL respond with HTTP status 400 and a JSON body containing a descriptive error message
5. IF an HTTP request targets an unknown endpoint, THEN THE HTTP_Server SHALL respond with HTTP status 404
6. THE HTTP_Server SHALL accept configuration payloads containing: prompt trigger strings, response strings, line ending preference, prompt timeout value, the ordered list of rules forming a Command_Sequence, and the ordered list of Post_Commands with their optional expected prompts and inter-command delay
7. IF the HTTP listening port is already in use, THEN THE Bridge SHALL log an error message containing the port number and terminate gracefully

### Requirement 6: WiFi Connectivity (M5 Stack)

**User Story:** As a developer deploying on M5 Stack, I want the Bridge to connect to a predefined WiFi access point, so that the Telnet and HTTP servers are reachable over the network.

#### Acceptance Criteria

1. WHEN the Bridge starts on the M5 Stack platform, THE WiFi_Module SHALL attempt to connect to the preconfigured WiFi access point using the preconfigured SSID and password
2. IF the WiFi connection fails, THEN THE WiFi_Module SHALL retry the connection at 5-second intervals up to a configurable maximum number of retries
3. IF the maximum number of WiFi retries is exceeded, THEN THE WiFi_Module SHALL log an error and restart the connection attempt cycle
4. WHILE the WiFi connection is established, THE WiFi_Module SHALL monitor the connection and reconnect automatically if the connection drops
5. WHEN the Bridge starts on a Linux platform, THE WiFi_Module SHALL provide a stub implementation that reports the connection as always established and returns the system's current IP address

### Requirement 7: Status Display (M5 Stack)

**User Story:** As a developer using M5 Stack, I want to see WiFi status, signal strength, IP address, and UART activity on the built-in display, so that I can quickly assess the Bridge's state without a remote connection.

#### Acceptance Criteria

1. WHILE the WiFi connection is established on M5 Stack, THE Display_Module SHALL show the WiFi connection status, signal strength in dBm, and the assigned IP address on the M5 Stack display
2. WHILE the WiFi connection is not established on M5 Stack, THE Display_Module SHALL show the WiFi status as "Disconnected" on the M5 Stack display
3. THE Display_Module SHALL show the current UART RX speed and TX speed in bytes per second on the M5 Stack display, updated at least once per second
4. WHEN 1 minute has elapsed since the last button press on M5 Stack, THE Display_Module SHALL turn off the display backlight
5. WHEN a button is pressed on M5 Stack while the display backlight is off, THE Display_Module SHALL turn on the display backlight and reset the 1-minute timeout
6. WHEN the Bridge runs on a Linux platform, THE Display_Module SHALL print the system IP address and UART RX/TX speed in bytes per second to the console, updated at least once per second

### Requirement 8: Interface-Based Module Architecture

**User Story:** As a developer, I want each major module to be defined by an abstract C++ interface with platform-specific implementations, so that the application can be ported to new platforms by implementing the interfaces without modifying core logic.

#### Acceptance Criteria

1. THE Bridge SHALL define the UART_Module, Telnet_Server, HTTP_Server, Auto_Responder, Display_Module, and WiFi_Module as abstract C++ classes (Platform_Interfaces)
2. THE Bridge SHALL select the correct platform-specific implementation of each Platform_Interface at compile time based on the target platform
3. THE Bridge core logic SHALL depend only on Platform_Interfaces, not on any platform-specific implementation class
4. WHEN a new platform is added, THE Bridge SHALL require only new implementations of the Platform_Interfaces, with no modifications to the core logic or other platform implementations
5. THE Bridge SHALL organize platform-specific implementations in separate directories (one per platform), so that adding a new platform requires only adding a new directory with implementations of the Platform_Interfaces
6. THE Bridge build system SHALL support adding a new target platform by defining a new platform identifier and pointing it to the corresponding implementation directory, without modifying existing platform build definitions
7. EACH Platform_Interface SHALL document its contract (preconditions, postconditions, and expected behavior) in the interface header, so that a developer porting to a new platform has a clear specification to implement against
8. THE Bridge SHALL provide a platform registration or factory mechanism that maps a platform identifier to the set of concrete Platform_Interface implementations, so that new platforms can be integrated by registering their implementations without altering existing code

### Requirement 9: Configuration and Startup

**User Story:** As a developer, I want to configure UART port settings, Telnet port, HTTP port, WiFi credentials, and display timeout via a configuration source, so that I can deploy the Bridge in different environments without code changes.

#### Acceptance Criteria

1. WHEN the Bridge starts, THE Bridge SHALL read configuration values for: UART port name, UART baud rate, UART data bits, UART parity, UART stop bits, Telnet listening port, HTTP listening port, WiFi SSID, WiFi password, WiFi max retries, display backlight timeout, and Auto_Responder prompt timeout
2. IF a required configuration value is missing, THEN THE Bridge SHALL log an error identifying the missing value and terminate gracefully
3. THE Bridge SHALL support reading configuration from a file in JSON format

### Requirement 10: Unit Testing

**User Story:** As a developer, I want the Bridge core logic and modules to be covered by unit tests using Google Test (gtest), so that I can verify correctness and catch regressions early.

#### Acceptance Criteria

1. THE Bridge project SHALL include a test target that builds and runs unit tests using the Google Test (gtest) framework
2. THE Bridge unit tests SHALL test core logic (Auto_Responder, configuration parsing, Command_Sequence processing, Post_Command execution) by injecting mock implementations of Platform_Interfaces
3. EACH Platform_Interface SHALL have a corresponding mock class (using gtest/gmock or manual mocks) that can be used in unit tests to isolate the module under test from hardware and network dependencies
4. THE Auto_Responder unit tests SHALL verify: prompt matching triggers the correct response, Command_Sequence rules are processed in order, Post_Commands are sent after all Prompt_Rules complete, timeout behavior skips unmatched rules, and line endings are appended correctly
5. THE HTTP_Server unit tests SHALL verify: valid JSON payloads update the Auto_Responder configuration, malformed JSON returns HTTP 400, unknown endpoints return HTTP 404, and GET returns the current configuration
6. THE configuration parsing unit tests SHALL verify: valid JSON config files are parsed correctly, missing required values produce an error, and all configuration fields are read
7. THE Bridge build system SHALL support running all unit tests via a single command (e.g., `ctest` or a dedicated test target)
8. THE unit tests SHALL be buildable and runnable on the Linux host platform without requiring ESP32 hardware
