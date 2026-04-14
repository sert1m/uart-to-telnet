/**
 * @file AutoResponder.cpp
 * @brief Implementation of the AutoResponder class.
 *
 * @see AutoResponder.h for class documentation.
 */

#include "core/AutoResponder.h"

#include <algorithm>
#include <iostream>

AutoResponder::AutoResponder(WriteCallback writeCallback)
    : writeCallback_(std::move(writeCallback)) {
    buffer_.reserve(kBufferSize);
}

void AutoResponder::process(const uint8_t* data, size_t length) {
    if (length == 0 || data == nullptr) {
        return;
    }

    // Append to rolling buffer, trimming from front if over capacity
    buffer_.append(reinterpret_cast<const char*>(data), length);
    if (buffer_.size() > kBufferSize) {
        buffer_.erase(0, buffer_.size() - kBufferSize);
    }

    if (state_ == State::MATCHING_RULES) {
        const auto& rules = config_.rules;
        if (currentRuleIndex_ < rules.size()) {
            const auto& trigger = rules[currentRuleIndex_].trigger;
            auto pos = buffer_.find(trigger);
            if (pos != std::string::npos) {
                // Match found — send response + lineEnding
                const auto& response = rules[currentRuleIndex_].response;
                sendToUart(response + config_.lineEnding);

                // Clear buffer up to and including the match
                buffer_.erase(0, pos + trigger.size());

                // Advance to next rule
                currentRuleIndex_++;
                elapsedSinceRuleStart_ = 0;

                if (currentRuleIndex_ >= rules.size()) {
                    // All rules processed — transition
                    if (!config_.postCommands.empty()) {
                        state_ = State::SENDING_POST_COMMANDS;
                        currentPostCmdIndex_ = 0;
                        postCmdDelayAccum_ = 0;
                        postCmdSent_ = false;
                        waitingForPostPrompt_ = false;
                    } else {
                        state_ = State::IDLE;
                    }
                }
            }
        }
    } else if (state_ == State::SENDING_POST_COMMANDS) {
        // Check if we're waiting for an expectedPrompt from a PostCommand
        if (waitingForPostPrompt_ && currentPostCmdIndex_ < config_.postCommands.size()) {
            const auto& expectedPrompt = config_.postCommands[currentPostCmdIndex_].expectedPrompt;
            if (!expectedPrompt.empty()) {
                auto pos = buffer_.find(expectedPrompt);
                if (pos != std::string::npos) {
                    // Prompt matched — advance to next PostCommand
                    buffer_.erase(0, pos + expectedPrompt.size());
                    currentPostCmdIndex_++;
                    postCmdDelayAccum_ = 0;
                    postCmdSent_ = false;
                    waitingForPostPrompt_ = false;

                    if (currentPostCmdIndex_ >= config_.postCommands.size()) {
                        state_ = State::IDLE;
                    }
                }
            }
        }
    }
}

void AutoResponder::tick(uint32_t elapsedMs) {
    if (state_ == State::MATCHING_RULES) {
        if (config_.promptTimeoutMs > 0 && currentRuleIndex_ < config_.rules.size()) {
            elapsedSinceRuleStart_ += elapsedMs;
            if (elapsedSinceRuleStart_ >= config_.promptTimeoutMs) {
                // Timeout — log warning and skip to next rule
                std::cerr << "AutoResponder: prompt timeout waiting for trigger \""
                          << config_.rules[currentRuleIndex_].trigger << "\"" << std::endl;

                currentRuleIndex_++;
                elapsedSinceRuleStart_ = 0;

                if (currentRuleIndex_ >= config_.rules.size()) {
                    if (!config_.postCommands.empty()) {
                        state_ = State::SENDING_POST_COMMANDS;
                        currentPostCmdIndex_ = 0;
                        postCmdDelayAccum_ = 0;
                        postCmdSent_ = false;
                        waitingForPostPrompt_ = false;
                    } else {
                        state_ = State::IDLE;
                    }
                }
            }
        }
    } else if (state_ == State::SENDING_POST_COMMANDS) {
        if (currentPostCmdIndex_ >= config_.postCommands.size()) {
            state_ = State::IDLE;
            return;
        }

        const auto& postCmd = config_.postCommands[currentPostCmdIndex_];

        if (!postCmdSent_) {
            // Accumulate delay before sending
            postCmdDelayAccum_ += elapsedMs;
            if (postCmdDelayAccum_ >= postCmd.delayMs) {
                sendCurrentPostCommand();
            }
        } else if (waitingForPostPrompt_) {
            // Waiting for expectedPrompt — check timeout (0 = wait forever)
            if (config_.promptTimeoutMs > 0) {
                postCmdDelayAccum_ += elapsedMs;
                if (postCmdDelayAccum_ >= config_.promptTimeoutMs) {
                    std::cerr << "AutoResponder: prompt timeout waiting for expected prompt \""
                              << postCmd.expectedPrompt << "\"" << std::endl;

                    currentPostCmdIndex_++;
                    postCmdDelayAccum_ = 0;
                    postCmdSent_ = false;
                    waitingForPostPrompt_ = false;

                    if (currentPostCmdIndex_ >= config_.postCommands.size()) {
                        state_ = State::IDLE;
                    }
                }
            }
            // else: timeout=0, wait forever for the prompt (handled in process())
        }
    }
}

void AutoResponder::setConfig(const CommandSequence& seq) {
    config_ = seq;
    resetState();

    if (!config_.rules.empty()) {
        state_ = State::MATCHING_RULES;
    } else if (!config_.postCommands.empty()) {
        state_ = State::SENDING_POST_COMMANDS;
    } else {
        state_ = State::IDLE;
    }
}

const CommandSequence& AutoResponder::getConfig() const {
    return config_;
}

AutoResponder::State AutoResponder::getState() const {
    return state_;
}

size_t AutoResponder::getCurrentRuleIndex() const {
    return currentRuleIndex_;
}

size_t AutoResponder::getCurrentPostCommandIndex() const {
    return currentPostCmdIndex_;
}

void AutoResponder::sendToUart(const std::string& str) {
    if (writeCallback_ && !str.empty()) {
        writeCallback_(reinterpret_cast<const uint8_t*>(str.data()), str.size());
    }
}

void AutoResponder::sendCurrentPostCommand() {
    if (currentPostCmdIndex_ >= config_.postCommands.size()) {
        return;
    }

    const auto& postCmd = config_.postCommands[currentPostCmdIndex_];
    sendToUart(postCmd.command + config_.lineEnding);
    postCmdSent_ = true;

    if (!postCmd.expectedPrompt.empty()) {
        // Wait for expected prompt (timeout=0 means wait forever)
        waitingForPostPrompt_ = true;
        postCmdDelayAccum_ = 0;
    } else {
        // No expected prompt — advance immediately
        currentPostCmdIndex_++;
        postCmdDelayAccum_ = 0;
        postCmdSent_ = false;
        waitingForPostPrompt_ = false;

        if (currentPostCmdIndex_ >= config_.postCommands.size()) {
            state_ = State::IDLE;
        }
    }
}

void AutoResponder::resetState() {
    currentRuleIndex_ = 0;
    currentPostCmdIndex_ = 0;
    elapsedSinceRuleStart_ = 0;
    postCmdDelayAccum_ = 0;
    postCmdSent_ = false;
    waitingForPostPrompt_ = false;
    buffer_.clear();
}
