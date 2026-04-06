/**
 * @file AutoResponder.h
 * @brief Platform-agnostic automatic prompt responder for UART command sequences.
 *
 * Monitors incoming UART data for configured prompt triggers and sends
 * preconfigured responses. Processes PromptRules in order, then executes
 * PostCommands with configurable delays and optional expected-prompt waiting.
 *
 * @see Requirement 4.1, 4.2, 4.3, 4.5, 4.6, 4.7, 4.8, 4.9, 4.10
 */

#ifndef CORE_AUTO_RESPONDER_H
#define CORE_AUTO_RESPONDER_H

#include "core/DataModels.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

/**
 * @brief Automatic prompt responder that processes UART data against configured rules.
 *
 * The AutoResponder maintains a rolling buffer of recent UART output and matches
 * incoming data against an ordered sequence of PromptRules. When a trigger is found,
 * the corresponding response (plus a configurable line ending) is sent to the UART
 * device via a write callback. After all rules are processed, PostCommands are
 * executed in order with configurable inter-command delays.
 *
 * State machine:
 * - IDLE: No config set or all rules and PostCommands processed.
 * - MATCHING_RULES: Processing PromptRules in order.
 * - SENDING_POST_COMMANDS: All rules done; sending PostCommands one at a time.
 *
 * @see Requirement 4.1, 4.2, 4.3, 4.5, 4.6, 4.7, 4.8, 4.9, 4.10
 */
class AutoResponder {
public:
    /// @brief State machine states for the AutoResponder.
    enum class State {
        IDLE,                   ///< No config or all processing complete.
        MATCHING_RULES,         ///< Processing PromptRules in order.
        SENDING_POST_COMMANDS   ///< Sending PostCommands after all rules complete.
    };

    /// @brief Write callback type: sends bytes to the UART device.
    using WriteCallback = std::function<void(const uint8_t*, size_t)>;

    /**
     * @brief Construct an AutoResponder with a UART write callback.
     * @param writeCallback Function used to send data to the UART device.
     *        Must not be null.
     */
    explicit AutoResponder(WriteCallback writeCallback);

    /**
     * @brief Process incoming UART data for prompt matching.
     *
     * Appends data to the rolling buffer and checks for trigger matches
     * against the current PromptRule or PostCommand expectedPrompt.
     *
     * @param data Pointer to the received byte buffer.
     * @param length Number of bytes received.
     * @pre data is not null when length > 0.
     */
    void process(const uint8_t* data, size_t length);

    /**
     * @brief Advance timeouts and delays by the given elapsed time.
     *
     * Called periodically to handle prompt timeouts and inter-command delays.
     * When a prompt times out, logs a warning and skips to the next rule.
     * When all rules are done, starts sending PostCommands with delays.
     *
     * @param elapsedMs Milliseconds elapsed since the last tick call.
     */
    void tick(uint32_t elapsedMs);

    /**
     * @brief Replace the current command sequence configuration.
     *
     * Resets internal state to begin processing from rule[0] of the new config.
     *
     * @param seq The new CommandSequence to process.
     * @see Requirement 4.3
     */
    void setConfig(const CommandSequence& seq);

    /**
     * @brief Get the current command sequence configuration.
     * @return A const reference to the current CommandSequence.
     */
    const CommandSequence& getConfig() const;

    /**
     * @brief Get the current state of the AutoResponder.
     * @return The current State enum value.
     */
    State getState() const;

    /**
     * @brief Get the index of the current rule being matched.
     * @return Index into the rules vector, or rules.size() if all rules processed.
     */
    size_t getCurrentRuleIndex() const;

    /**
     * @brief Get the index of the current PostCommand being sent.
     * @return Index into the postCommands vector.
     */
    size_t getCurrentPostCommandIndex() const;

private:
    /// Maximum rolling buffer size in bytes.
    static constexpr size_t kBufferSize = 4096;

    /// @brief Send a string to UART via the write callback.
    void sendToUart(const std::string& str);

    /// @brief Send the current PostCommand to UART and advance state.
    void sendCurrentPostCommand();

    /// @brief Reset internal processing state (indices, timers, buffer).
    void resetState();

    WriteCallback writeCallback_;           ///< Callback to write data to UART.
    CommandSequence config_;                 ///< Current command sequence configuration.
    State state_ = State::IDLE;             ///< Current state machine state.
    size_t currentRuleIndex_ = 0;           ///< Index of the current PromptRule.
    size_t currentPostCmdIndex_ = 0;        ///< Index of the current PostCommand.
    std::string buffer_;                    ///< Rolling buffer of recent UART output.
    uint32_t elapsedSinceRuleStart_ = 0;    ///< Ms elapsed since current rule started matching.
    uint32_t postCmdDelayAccum_ = 0;        ///< Ms accumulated for PostCommand delay.
    bool waitingForPostPrompt_ = false;     ///< True if waiting for a PostCommand expectedPrompt.
    bool postCmdSent_ = false;              ///< True if current PostCommand has been sent.
};

#endif // CORE_AUTO_RESPONDER_H
