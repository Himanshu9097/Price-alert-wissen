#include "json_parser.hpp"
#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <cmath>
#include <filesystem>
#include <cctype>

namespace wissen {

static bool is_valid_instrument(const std::string& inst) {
    if (inst.empty() || inst.size() > 64) return false;
    for (char c : inst) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '.' && c != '-' && c != '_') {
            return false;
        }
    }
    return true;
}

std::vector<Rule> JsonParser::parse_file(const std::string& filepath) const {
    std::error_code ec;
    if (!std::filesystem::exists(filepath, ec)) {
        throw std::runtime_error("JSON Error: file does not exist: " + filepath);
    }
    if (!std::filesystem::is_regular_file(filepath, ec)) {
        throw std::runtime_error("JSON Error: path is not a regular file: " + filepath);
    }

    auto file_size = std::filesystem::file_size(filepath, ec);
    if (ec) {
        throw std::runtime_error("JSON Error: unable to query file size: " + ec.message() + " (" + filepath + ")");
    }
    if (file_size > config_.max_file_size_bytes) {
        throw std::runtime_error("JSON Error: file size exceeds maximum permitted limit (" +
                                 std::to_string(config_.max_file_size_bytes) + " bytes): " + filepath);
    }

    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("JSON Error: failed to open file for reading: " + filepath);
    }

    return parse_stream(file);
}

std::vector<Rule> JsonParser::parse_stream(std::istream& in) const {
    nlohmann::json j;
    try {
        in >> j;
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("JSON Error: syntax error while parsing JSON: " + std::string(e.what()));
    }

    if (!j.is_array()) {
        throw std::runtime_error("JSON Error: root element must be a JSON array of rules");
    }

    if (j.size() > config_.max_rules) {
        throw std::runtime_error("JSON Error: exceeded maximum permitted rules count (" +
                                 std::to_string(config_.max_rules) + ")");
    }

    std::vector<Rule> rules;
    rules.reserve(j.size());
    std::unordered_set<std::string> seen_rule_ids;

    for (size_t idx = 0; idx < j.size(); ++idx) {
        const auto& item = j[idx];
        std::string rule_loc = "rule at index " + std::to_string(idx);

        if (!item.is_object()) {
            throw std::runtime_error("JSON Error: " + rule_loc + " must be a JSON object");
        }

        // Required common field: id
        if (!item.contains("id") || !item["id"].is_string()) {
            throw std::runtime_error("JSON Error: " + rule_loc + " missing required string field 'id'");
        }
        std::string id = item["id"].get<std::string>();
        if (id.empty()) {
            throw std::runtime_error("JSON Error: " + rule_loc + " has empty 'id'");
        }
        if (seen_rule_ids.find(id) != seen_rule_ids.end()) {
            throw std::runtime_error("JSON Error: duplicate rule id '" + id + "' detected");
        }
        seen_rule_ids.insert(id);

        rule_loc = "rule '" + id + "'";

        // Required common field: instrument
        if (!item.contains("instrument") || !item["instrument"].is_string()) {
            throw std::runtime_error("JSON Error: " + rule_loc + " missing required string field 'instrument'");
        }
        std::string instrument = item["instrument"].get<std::string>();
        if (!is_valid_instrument(instrument)) {
            throw std::runtime_error("JSON Error: " + rule_loc + " has invalid instrument ticker '" + instrument + "'");
        }

        // Required common field: type
        if (!item.contains("type") || !item["type"].is_string()) {
            throw std::runtime_error("JSON Error: " + rule_loc + " missing required string field 'type'");
        }
        std::string type_str = item["type"].get<std::string>();
        auto rule_type_opt = parse_rule_type(type_str);
        if (!rule_type_opt) {
            throw std::runtime_error("JSON Error: " + rule_loc + " has unsupported rule type '" + type_str +
                                     "'. Supported types are 'level' and 'ma_cross'");
        }
        RuleType rule_type = *rule_type_opt;

        // Required common field: direction
        if (!item.contains("direction") || !item["direction"].is_string()) {
            throw std::runtime_error("JSON Error: " + rule_loc + " missing required string field 'direction'");
        }
        std::string dir_str = item["direction"].get<std::string>();
        auto dir_opt = parse_direction(dir_str);
        if (!dir_opt) {
            throw std::runtime_error("JSON Error: " + rule_loc + " has invalid direction '" + dir_str +
                                     "'. Must be 'above' or 'below'");
        }
        Direction direction = *dir_opt;

        Rule rule;
        rule.id = id;
        rule.instrument = instrument;
        rule.type = rule_type;
        rule.direction = direction;

        if (rule_type == RuleType::Level) {
            // Validate expected keys for Level rule
            for (auto it = item.begin(); it != item.end(); ++it) {
                const std::string& k = it.key();
                if (k != "id" && k != "instrument" && k != "type" && k != "direction" && k != "level") {
                    throw std::runtime_error("JSON Error: " + rule_loc + " contains unexpected field '" + k + "'");
                }
            }

            if (!item.contains("level") || !item["level"].is_number()) {
                throw std::runtime_error("JSON Error: " + rule_loc + " missing required numeric field 'level'");
            }

            double level_val = item["level"].get<double>();
            if (!std::isfinite(level_val) || level_val <= 0.0) {
                throw std::runtime_error("JSON Error: " + rule_loc + " has non-finite or non-positive level: " +
                                         item["level"].dump());
            }

            rule.level = level_val;
            rule.raw_level_str = item["level"].dump();
        } else if (rule_type == RuleType::MACross) {
            // Validate expected keys for MACross rule
            for (auto it = item.begin(); it != item.end(); ++it) {
                const std::string& k = it.key();
                if (k != "id" && k != "instrument" && k != "type" && k != "direction" && k != "short" && k != "long") {
                    throw std::runtime_error("JSON Error: " + rule_loc + " contains unexpected field '" + k + "'");
                }
            }

            if (!item.contains("short") || !item["short"].is_number_integer()) {
                throw std::runtime_error("JSON Error: " + rule_loc + " missing required integer field 'short'");
            }
            if (!item.contains("long") || !item["long"].is_number_integer()) {
                throw std::runtime_error("JSON Error: " + rule_loc + " missing required integer field 'long'");
            }

            int short_win = item["short"].get<int>();
            int long_win = item["long"].get<int>();

            if (short_win < 1) {
                throw std::runtime_error("JSON Error: " + rule_loc + " 'short' window must be at least 1");
            }
            if (long_win < 1) {
                throw std::runtime_error("JSON Error: " + rule_loc + " 'long' window must be at least 1");
            }
            if (short_win > config_.max_ma_window || long_win > config_.max_ma_window) {
                throw std::runtime_error("JSON Error: " + rule_loc + " moving average window exceeds maximum limit (" +
                                         std::to_string(config_.max_ma_window) + ")");
            }
            if (short_win >= long_win) {
                throw std::runtime_error("JSON Error: " + rule_loc + " 'short' window (" +
                                         std::to_string(short_win) + ") must be strictly less than 'long' window (" +
                                         std::to_string(long_win) + ")");
            }

            rule.short_window = short_win;
            rule.long_window = long_win;
        }

        rules.push_back(rule);
    }

    return rules;
}

} // namespace wissen
