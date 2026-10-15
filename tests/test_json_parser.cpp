#include "test_harness.hpp"
#include "json_parser.hpp"

#include <sstream>
#include <stdexcept>

using namespace wissen;

TEST_CASE(test_json_parser_valid_rules) {
    std::string json_str = R"([
        {"id": "r1", "instrument": "INFY", "type": "level", "direction": "below", "level": 1500},
        {"id": "r2", "instrument": "TCS", "type": "ma_cross", "short": 5, "long": 20, "direction": "above"}
    ])";

    std::istringstream iss(json_str);
    JsonParser parser;
    auto rules = parser.parse_stream(iss);

    ASSERT_EQ(rules.size(), 2UL);

    ASSERT_EQ(rules[0].id, "r1");
    ASSERT_EQ(rules[0].instrument, "INFY");
    ASSERT_TRUE(rules[0].type == RuleType::Level);
    ASSERT_TRUE(rules[0].direction == Direction::Below);
    ASSERT_EQ(rules[0].level, 1500.0);
    ASSERT_EQ(rules[0].raw_level_str, "1500");

    ASSERT_EQ(rules[1].id, "r2");
    ASSERT_EQ(rules[1].instrument, "TCS");
    ASSERT_TRUE(rules[1].type == RuleType::MACross);
    ASSERT_TRUE(rules[1].direction == Direction::Above);
    ASSERT_EQ(rules[1].short_window, 5);
    ASSERT_EQ(rules[1].long_window, 20);
}

TEST_CASE(test_json_parser_empty_array) {
    std::string json_str = "[]";
    std::istringstream iss(json_str);
    JsonParser parser;
    auto rules = parser.parse_stream(iss);
    ASSERT_EQ(rules.size(), 0UL);
}

TEST_CASE(test_json_parser_non_array_root) {
    std::string json_str = R"({"id": "r1"})";
    std::istringstream iss(json_str);
    JsonParser parser;
    ASSERT_THROWS(parser.parse_stream(iss), std::runtime_error);
}

TEST_CASE(test_json_parser_duplicate_rule_id) {
    std::string json_str = R"([
        {"id": "r1", "instrument": "INFY", "type": "level", "direction": "below", "level": 1500},
        {"id": "r1", "instrument": "TCS", "type": "level", "direction": "above", "level": 3000}
    ])";
    std::istringstream iss(json_str);
    JsonParser parser;
    ASSERT_THROWS(parser.parse_stream(iss), std::runtime_error);
}

TEST_CASE(test_json_parser_invalid_direction) {
    std::string json_str = R"([
        {"id": "r1", "instrument": "INFY", "type": "level", "direction": "sideways", "level": 1500}
    ])";
    std::istringstream iss(json_str);
    JsonParser parser;
    ASSERT_THROWS(parser.parse_stream(iss), std::runtime_error);
}

TEST_CASE(test_json_parser_unsupported_type) {
    std::string json_str = R"([
        {"id": "r1", "instrument": "INFY", "type": "rsi", "direction": "below", "level": 30}
    ])";
    std::istringstream iss(json_str);
    JsonParser parser;
    ASSERT_THROWS(parser.parse_stream(iss), std::runtime_error);
}

TEST_CASE(test_json_parser_invalid_level_values) {
    // Negative level
    {
        std::string json_str = R"([{"id": "r1", "instrument": "INFY", "type": "level", "direction": "below", "level": -100}])";
        std::istringstream iss(json_str);
        ASSERT_THROWS(JsonParser{}.parse_stream(iss), std::runtime_error);
    }
    // Zero level
    {
        std::string json_str = R"([{"id": "r1", "instrument": "INFY", "type": "level", "direction": "below", "level": 0}])";
        std::istringstream iss(json_str);
        ASSERT_THROWS(JsonParser{}.parse_stream(iss), std::runtime_error);
    }
    // String level
    {
        std::string json_str = R"([{"id": "r1", "instrument": "INFY", "type": "level", "direction": "below", "level": "1500"}])";
        std::istringstream iss(json_str);
        ASSERT_THROWS(JsonParser{}.parse_stream(iss), std::runtime_error);
    }
}

TEST_CASE(test_json_parser_invalid_ma_windows) {
    // short == long
    {
        std::string json_str = R"([{"id": "r1", "instrument": "TCS", "type": "ma_cross", "short": 10, "long": 10, "direction": "above"}])";
        std::istringstream iss(json_str);
        ASSERT_THROWS(JsonParser{}.parse_stream(iss), std::runtime_error);
    }
    // short > long
    {
        std::string json_str = R"([{"id": "r1", "instrument": "TCS", "type": "ma_cross", "short": 20, "long": 5, "direction": "above"}])";
        std::istringstream iss(json_str);
        ASSERT_THROWS(JsonParser{}.parse_stream(iss), std::runtime_error);
    }
    // short <= 0
    {
        std::string json_str = R"([{"id": "r1", "instrument": "TCS", "type": "ma_cross", "short": 0, "long": 10, "direction": "above"}])";
        std::istringstream iss(json_str);
        ASSERT_THROWS(JsonParser{}.parse_stream(iss), std::runtime_error);
    }
    // Extremely large positive integer window exceeding limits
    {
        std::string json_str = R"([{"id": "r1", "instrument": "TCS", "type": "ma_cross", "short": 5, "long": 999999999999999999, "direction": "above"}])";
        std::istringstream iss(json_str);
        ASSERT_THROWS(JsonParser{}.parse_stream(iss), std::runtime_error);
    }
    // Extremely large negative integer window
    {
        std::string json_str = R"([{"id": "r1", "instrument": "TCS", "type": "ma_cross", "short": -999999999999999999, "long": 20, "direction": "above"}])";
        std::istringstream iss(json_str);
        ASSERT_THROWS(JsonParser{}.parse_stream(iss), std::runtime_error);
    }
    // Window exceeding max_ma_window (10,000)
    {
        std::string json_str = R"([{"id": "r1", "instrument": "TCS", "type": "ma_cross", "short": 5, "long": 20000, "direction": "above"}])";
        std::istringstream iss(json_str);
        ASSERT_THROWS(JsonParser{}.parse_stream(iss), std::runtime_error);
    }
}

TEST_CASE(test_json_parser_unexpected_field) {
    std::string json_str = R"([
        {"id": "r1", "instrument": "INFY", "type": "level", "direction": "below", "level": 1500, "extra": "unexpected"}
    ])";
    std::istringstream iss(json_str);
    JsonParser parser;
    ASSERT_THROWS(parser.parse_stream(iss), std::runtime_error);
}

TEST_CASE(test_json_parser_syntax_error) {
    std::string json_str = "[ {id: missing_quotes} ]";
    std::istringstream iss(json_str);
    JsonParser parser;
    ASSERT_THROWS(parser.parse_stream(iss), std::runtime_error);
}
