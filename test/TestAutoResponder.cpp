/**
 * @file TestAutoResponder.cpp
 * @brief Property-based tests for AutoResponder.
 *
 * Feature: uart-telnet-bridge
 */

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "core/AutoResponder.h"
#include "core/DataModels.h"

#include <string>
#include <vector>
#include <cstdint>

// ---------------------------------------------------------------------------
// Helper: generate a non-empty alphanumeric string (safe prefix/suffix that
// won't accidentally contain the delimiter-based trigger).
// ---------------------------------------------------------------------------
namespace {

rc::Gen<std::string> genAlphanumeric() {
    return rc::gen::map(
        rc::gen::container<std::vector<char>>(
            rc::gen::inRange('a', static_cast<char>('z' + 1))),
        [](std::vector<char> chars) -> std::string {
            return std::string(chars.begin(), chars.end());
        });
}

rc::Gen<std::string> genNonEmptyAlphanumeric() {
    return rc::gen::suchThat(genAlphanumeric(),
        [](const std::string& s) { return !s.empty(); });
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Helper: collect write-callback outputs as separate messages.
// ---------------------------------------------------------------------------
namespace {

struct MessageCapture {
    std::vector<std::string> messages;

    void callback(const uint8_t* data, size_t len) {
        messages.emplace_back(reinterpret_cast<const char*>(data), len);
    }
};

} // anonymous namespace

// ---------------------------------------------------------------------------
// Property 4: Prompt matching sends correct response with line ending
// Feature: uart-telnet-bridge, Property 4: Prompt matching sends correct response with line ending
// **Validates: Requirements 4.1, 4.6**
// ---------------------------------------------------------------------------

RC_GTEST_PROP(AutoResponderProperties,
              promptMatchSendsResponseWithLineEnding,
              ()) {
    // Generate a non-empty trigger with a unique delimiter so it can't appear
    // in the alphanumeric prefix/suffix.
    const auto triggerBody = *rc::gen::nonEmpty<std::string>();
    const std::string trigger = "::TRIGGER::" + triggerBody + "::END::";

    // Generate a non-empty response string.
    const auto response = *rc::gen::nonEmpty<std::string>();

    // Generate a non-empty line ending.
    const auto lineEnding = *rc::gen::nonEmpty<std::string>();

    // Generate alphanumeric prefix and suffix (guaranteed not to contain "::TRIGGER::").
    const auto prefix = *genAlphanumeric();
    const auto suffix = *genAlphanumeric();

    // Build a CommandSequence with a single rule.
    CommandSequence seq;
    PromptRule rule;
    rule.trigger = trigger;
    rule.response = response;
    seq.rules = {rule};
    seq.lineEnding = lineEnding;
    seq.promptTimeoutMs = 10000;

    // Capture bytes sent via the write callback.
    std::vector<uint8_t> captured;
    AutoResponder responder([&captured](const uint8_t* data, size_t len) {
        captured.insert(captured.end(), data, data + len);
    });

    responder.setConfig(seq);

    // Build UART data: prefix + trigger + suffix
    const std::string uartData = prefix + trigger + suffix;
    responder.process(
        reinterpret_cast<const uint8_t*>(uartData.data()),
        uartData.size());

    // Expected output is exactly response + lineEnding.
    const std::string expected = response + lineEnding;
    const std::string actual(captured.begin(), captured.end());

    RC_ASSERT(actual == expected);
}

// ---------------------------------------------------------------------------
// Property 5: CommandSequence rules processed in order
// Feature: uart-telnet-bridge, Property 5: CommandSequence rules processed in order
// **Validates: Requirements 4.2**
// ---------------------------------------------------------------------------

RC_GTEST_PROP(AutoResponderProperties,
              commandSequenceRulesProcessedInOrder,
              ()) {
    // Generate N rules where N is in [2, 10].
    const auto N = *rc::gen::inRange(2, 11);

    // Build rules with unique delimiter-based triggers and responses.
    CommandSequence seq;
    seq.lineEnding = "\n";
    seq.promptTimeoutMs = 10000;

    for (int i = 0; i < N; ++i) {
        PromptRule rule;
        rule.trigger  = "::RULE_" + std::to_string(i) + "::";
        rule.response = "RESP_" + std::to_string(i);
        seq.rules.push_back(rule);
    }

    // --- Part A: feeding triggers in order produces responses in the same order ---
    {
        MessageCapture capture;
        AutoResponder responder([&capture](const uint8_t* d, size_t l) {
            capture.callback(d, l);
        });
        responder.setConfig(seq);

        for (int i = 0; i < N; ++i) {
            const std::string& trigger = seq.rules[i].trigger;
            responder.process(
                reinterpret_cast<const uint8_t*>(trigger.data()),
                trigger.size());
        }

        // We should have exactly N responses, each matching the rule order.
        RC_ASSERT(static_cast<int>(capture.messages.size()) == N);
        for (int i = 0; i < N; ++i) {
            const std::string expected =
                seq.rules[i].response + seq.lineEnding;
            RC_ASSERT(capture.messages[i] == expected);
        }
    }

    // --- Part B: rule[i+1] is NOT matched before rule[i] ---
    // Feed trigger[1] first (skipping trigger[0]). The responder should still
    // be waiting for rule[0], so no response should be produced.
    {
        MessageCapture capture;
        AutoResponder responder([&capture](const uint8_t* d, size_t l) {
            capture.callback(d, l);
        });
        responder.setConfig(seq);

        // Feed trigger[1] — should NOT match because rule[0] hasn't matched yet.
        const std::string& trigger1 = seq.rules[1].trigger;
        responder.process(
            reinterpret_cast<const uint8_t*>(trigger1.data()),
            trigger1.size());

        RC_ASSERT(capture.messages.empty());

        // The responder should still be on rule index 0.
        RC_ASSERT(responder.getCurrentRuleIndex() == 0u);
    }
}

// ===========================================================================
// Unit Tests for AutoResponder
// Feature: uart-telnet-bridge, Task 3.7
// **Validates: Requirements 10.4**
// ===========================================================================

// ---------------------------------------------------------------------------
// Test fixture for AutoResponder unit tests
// ---------------------------------------------------------------------------
class AutoResponderUnitTest : public ::testing::Test {
protected:
    MessageCapture capture_;
    std::unique_ptr<AutoResponder> responder_;

    void SetUp() override {
        responder_ = std::make_unique<AutoResponder>(
            [this](const uint8_t* data, size_t len) {
                capture_.callback(data, len);
            });
    }

    void feedString(const std::string& s) {
        responder_->process(
            reinterpret_cast<const uint8_t*>(s.data()), s.size());
    }
};

// ---------------------------------------------------------------------------
// 1. PromptMatchTriggersResponse
//    login: trigger → admin response with \n
// ---------------------------------------------------------------------------
TEST_F(AutoResponderUnitTest, PromptMatchTriggersResponse) {
    CommandSequence seq;
    PromptRule rule;
    rule.trigger = "login:";
    rule.response = "admin";
    seq.rules = {rule};
    seq.lineEnding = "\n";
    seq.promptTimeoutMs = 5000;

    responder_->setConfig(seq);
    EXPECT_EQ(responder_->getState(), AutoResponder::State::MATCHING_RULES);

    feedString("Welcome\nlogin:");

    ASSERT_EQ(capture_.messages.size(), 1u);
    EXPECT_EQ(capture_.messages[0], "admin\n");
    EXPECT_EQ(responder_->getCurrentRuleIndex(), 1u);
}

// ---------------------------------------------------------------------------
// 2. LoginPasswordSequence
//    login: → admin, password: → secret123, in order
// ---------------------------------------------------------------------------
TEST_F(AutoResponderUnitTest, LoginPasswordSequence) {
    CommandSequence seq;
    seq.lineEnding = "\n";
    seq.promptTimeoutMs = 5000;

    PromptRule loginRule;
    loginRule.trigger = "login:";
    loginRule.response = "admin";

    PromptRule passRule;
    passRule.trigger = "password:";
    passRule.response = "secret123";

    seq.rules = {loginRule, passRule};

    responder_->setConfig(seq);

    // Feed login trigger
    feedString("login:");
    ASSERT_EQ(capture_.messages.size(), 1u);
    EXPECT_EQ(capture_.messages[0], "admin\n");
    EXPECT_EQ(responder_->getCurrentRuleIndex(), 1u);

    // Feed password trigger
    feedString("password:");
    ASSERT_EQ(capture_.messages.size(), 2u);
    EXPECT_EQ(capture_.messages[1], "secret123\n");
    EXPECT_EQ(responder_->getCurrentRuleIndex(), 2u);

    // All rules done, no post commands → IDLE
    EXPECT_EQ(responder_->getState(), AutoResponder::State::IDLE);
}

// ---------------------------------------------------------------------------
// 3. PostCommandsSentAfterRules
//    After all rules match, PostCommands are sent via tick() with delays
// ---------------------------------------------------------------------------
TEST_F(AutoResponderUnitTest, PostCommandsSentAfterRules) {
    CommandSequence seq;
    seq.lineEnding = "\n";
    seq.promptTimeoutMs = 5000;

    PromptRule rule;
    rule.trigger = "ready>";
    rule.response = "go";
    seq.rules = {rule};

    PostCommand pc1;
    pc1.command = "cmd1";
    pc1.delayMs = 100;
    pc1.expectedPrompt = "";

    PostCommand pc2;
    pc2.command = "cmd2";
    pc2.delayMs = 200;
    pc2.expectedPrompt = "";

    seq.postCommands = {pc1, pc2};

    responder_->setConfig(seq);

    // Match the single rule
    feedString("ready>");
    ASSERT_EQ(capture_.messages.size(), 1u);
    EXPECT_EQ(capture_.messages[0], "go\n");
    EXPECT_EQ(responder_->getState(), AutoResponder::State::SENDING_POST_COMMANDS);

    // Tick less than pc1.delayMs — nothing sent yet
    responder_->tick(50);
    EXPECT_EQ(capture_.messages.size(), 1u);

    // Tick to reach pc1.delayMs — pc1 sent
    responder_->tick(50);
    ASSERT_EQ(capture_.messages.size(), 2u);
    EXPECT_EQ(capture_.messages[1], "cmd1\n");

    // Tick less than pc2.delayMs — nothing new
    responder_->tick(100);
    EXPECT_EQ(capture_.messages.size(), 2u);

    // Tick to reach pc2.delayMs — pc2 sent
    responder_->tick(100);
    ASSERT_EQ(capture_.messages.size(), 3u);
    EXPECT_EQ(capture_.messages[2], "cmd2\n");

    EXPECT_EQ(responder_->getState(), AutoResponder::State::IDLE);
}

// ---------------------------------------------------------------------------
// 4. TimeoutSkipsUnmatchedRule
//    Set promptTimeoutMs, tick past it, verify rule skipped and next rule
//    can match
// ---------------------------------------------------------------------------
TEST_F(AutoResponderUnitTest, TimeoutSkipsUnmatchedRule) {
    CommandSequence seq;
    seq.lineEnding = "\n";
    seq.promptTimeoutMs = 1000;

    PromptRule rule1;
    rule1.trigger = "never_appears";
    rule1.response = "resp1";

    PromptRule rule2;
    rule2.trigger = "hello>";
    rule2.response = "world";

    seq.rules = {rule1, rule2};

    responder_->setConfig(seq);
    EXPECT_EQ(responder_->getCurrentRuleIndex(), 0u);

    // Tick past the timeout for rule1
    responder_->tick(1000);

    // Rule1 should be skipped, now on rule2
    EXPECT_EQ(responder_->getCurrentRuleIndex(), 1u);
    EXPECT_EQ(capture_.messages.size(), 0u); // no response for skipped rule

    // Now feed rule2's trigger
    feedString("hello>");
    ASSERT_EQ(capture_.messages.size(), 1u);
    EXPECT_EQ(capture_.messages[0], "world\n");
    EXPECT_EQ(responder_->getCurrentRuleIndex(), 2u);
}

// ---------------------------------------------------------------------------
// 5. LineEndingsAppended
//    Verify \r\n line ending is appended correctly
// ---------------------------------------------------------------------------
TEST_F(AutoResponderUnitTest, LineEndingsAppended) {
    CommandSequence seq;
    seq.lineEnding = "\r\n";
    seq.promptTimeoutMs = 5000;

    PromptRule rule;
    rule.trigger = "prompt>";
    rule.response = "answer";
    seq.rules = {rule};

    responder_->setConfig(seq);
    feedString("prompt>");

    ASSERT_EQ(capture_.messages.size(), 1u);
    EXPECT_EQ(capture_.messages[0], "answer\r\n");
}

// ---------------------------------------------------------------------------
// 6. Capacity20Rules10PostCommands
//    Create 20 rules and 10 PostCommands, process all, verify all responses
// ---------------------------------------------------------------------------
TEST_F(AutoResponderUnitTest, Capacity20Rules10PostCommands) {
    CommandSequence seq;
    seq.lineEnding = "\n";
    seq.promptTimeoutMs = 10000;

    // Create 20 rules
    for (int i = 0; i < 20; ++i) {
        PromptRule rule;
        rule.trigger = "TRIGGER_" + std::to_string(i);
        rule.response = "RESP_" + std::to_string(i);
        seq.rules.push_back(rule);
    }

    // Create 10 PostCommands with 0 delay for fast testing
    for (int i = 0; i < 10; ++i) {
        PostCommand pc;
        pc.command = "POST_" + std::to_string(i);
        pc.delayMs = 0;
        pc.expectedPrompt = "";
        seq.postCommands.push_back(pc);
    }

    responder_->setConfig(seq);

    // Feed all 20 triggers in order
    for (int i = 0; i < 20; ++i) {
        feedString("TRIGGER_" + std::to_string(i));
    }

    // Should have 20 rule responses
    ASSERT_EQ(capture_.messages.size(), 20u);
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(capture_.messages[i], "RESP_" + std::to_string(i) + "\n");
    }

    EXPECT_EQ(responder_->getState(), AutoResponder::State::SENDING_POST_COMMANDS);

    // Tick to send all 10 PostCommands (delayMs=0, so each tick(0) should send)
    for (int i = 0; i < 10; ++i) {
        responder_->tick(0);
    }

    // Should have 20 rule responses + 10 post commands = 30 total
    ASSERT_EQ(capture_.messages.size(), 30u);
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(capture_.messages[20 + i], "POST_" + std::to_string(i) + "\n");
    }

    EXPECT_EQ(responder_->getState(), AutoResponder::State::IDLE);
}

// ---------------------------------------------------------------------------
// 7. ConfigurableDelayBetweenPostCommands
//    Verify PostCommands respect delayMs timing via tick()
// ---------------------------------------------------------------------------
TEST_F(AutoResponderUnitTest, ConfigurableDelayBetweenPostCommands) {
    CommandSequence seq;
    seq.lineEnding = "\n";
    seq.promptTimeoutMs = 5000;
    // No rules — go straight to post commands
    seq.rules = {};

    PostCommand pc1;
    pc1.command = "fast";
    pc1.delayMs = 100;
    pc1.expectedPrompt = "";

    PostCommand pc2;
    pc2.command = "slow";
    pc2.delayMs = 500;
    pc2.expectedPrompt = "";

    seq.postCommands = {pc1, pc2};

    responder_->setConfig(seq);
    EXPECT_EQ(responder_->getState(), AutoResponder::State::SENDING_POST_COMMANDS);

    // Tick 50ms — not enough for pc1 (100ms)
    responder_->tick(50);
    EXPECT_EQ(capture_.messages.size(), 0u);

    // Tick another 50ms — total 100ms, pc1 should fire
    responder_->tick(50);
    ASSERT_EQ(capture_.messages.size(), 1u);
    EXPECT_EQ(capture_.messages[0], "fast\n");

    // Tick 200ms — not enough for pc2 (500ms)
    responder_->tick(200);
    EXPECT_EQ(capture_.messages.size(), 1u);

    // Tick another 300ms — total 500ms for pc2, should fire
    responder_->tick(300);
    ASSERT_EQ(capture_.messages.size(), 2u);
    EXPECT_EQ(capture_.messages[1], "slow\n");

    EXPECT_EQ(responder_->getState(), AutoResponder::State::IDLE);
}

// ---------------------------------------------------------------------------
// 8. RuntimeConfigReplacement
//    Call setConfig twice, verify second config takes effect and getConfig
//    returns it
// ---------------------------------------------------------------------------
TEST_F(AutoResponderUnitTest, RuntimeConfigReplacement) {
    // First config
    CommandSequence seq1;
    seq1.lineEnding = "\n";
    seq1.promptTimeoutMs = 5000;
    PromptRule rule1;
    rule1.trigger = "old_trigger";
    rule1.response = "old_response";
    seq1.rules = {rule1};

    responder_->setConfig(seq1);
    EXPECT_EQ(responder_->getConfig().rules.size(), 1u);
    EXPECT_EQ(responder_->getConfig().rules[0].trigger, "old_trigger");

    // Replace with second config
    CommandSequence seq2;
    seq2.lineEnding = "\r\n";
    seq2.promptTimeoutMs = 3000;
    PromptRule rule2;
    rule2.trigger = "new_trigger";
    rule2.response = "new_response";
    seq2.rules = {rule2};

    responder_->setConfig(seq2);

    // Verify getConfig returns the new config
    const auto& cfg = responder_->getConfig();
    ASSERT_EQ(cfg.rules.size(), 1u);
    EXPECT_EQ(cfg.rules[0].trigger, "new_trigger");
    EXPECT_EQ(cfg.rules[0].response, "new_response");
    EXPECT_EQ(cfg.lineEnding, "\r\n");
    EXPECT_EQ(cfg.promptTimeoutMs, 3000u);

    // State should be reset to MATCHING_RULES for the new config
    EXPECT_EQ(responder_->getState(), AutoResponder::State::MATCHING_RULES);
    EXPECT_EQ(responder_->getCurrentRuleIndex(), 0u);

    // Old trigger should NOT match
    feedString("old_trigger");
    EXPECT_EQ(capture_.messages.size(), 0u);

    // New trigger should match
    feedString("new_trigger");
    ASSERT_EQ(capture_.messages.size(), 1u);
    EXPECT_EQ(capture_.messages[0], "new_response\r\n");
}
