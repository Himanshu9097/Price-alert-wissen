#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <tuple>

namespace wissen {

enum class Direction {
    Above,
    Below
};

inline std::string direction_to_string(Direction d) {
    switch (d) {
        case Direction::Above: return "above";
        case Direction::Below: return "below";
    }
    return "unknown";
}

inline std::optional<Direction> parse_direction(std::string_view s) {
    if (s == "above") return Direction::Above;
    if (s == "below") return Direction::Below;
    return std::nullopt;
}

enum class RuleType {
    Level,
    MACross
};

inline std::string rule_type_to_string(RuleType t) {
    switch (t) {
        case RuleType::Level: return "level";
        case RuleType::MACross: return "ma_cross";
    }
    return "unknown";
}

inline std::optional<RuleType> parse_rule_type(std::string_view s) {
    if (s == "level") return RuleType::Level;
    if (s == "ma_cross") return RuleType::MACross;
    return std::nullopt;
}

struct PriceObservation {
    std::string date;        // YYYY-MM-DD
    std::string instrument;  // Ticker symbol (e.g., INFY, TCS)
    double close;            // Positive finite closing price
};

struct Rule {
    std::string id;          // Unique rule ID (e.g., r1, r2)
    std::string instrument;  // Target instrument
    RuleType type;           // Level or MACross
    Direction direction;     // Above or Below

    // Level rule specifics
    double level{0.0};
    std::string raw_level_str; // Exact textual representation for clean formatting

    // MA cross rule specifics
    int short_window{0};
    int long_window{0};
};

struct Alert {
    std::string date;        // Date the alert fired (YYYY-MM-DD)
    std::string rule_id;     // Rule identifier
    std::string instrument;  // Instrument ticker
    RuleType rule_type;      // Level or MACross
    Direction direction;     // Above or Below
    std::string details;     // e.g. "level below 1500" or "ma_cross 5/20 above"

    // Primary sort by date ascending, secondary sort by rule_id ascending
    bool operator<(const Alert& other) const {
        if (date != other.date) {
            return date < other.date;
        }
        return rule_id < other.rule_id;
    }
};

} // namespace wissen
