#include "csv_parser.hpp"
#include "date_utils.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <set>

namespace wissen {

std::string CsvParser::trim(std::string_view sv) {
    auto start = sv.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) return "";
    auto end = sv.find_last_not_of(" \t\r\n");
    return std::string(sv.substr(start, end - start + 1));
}

std::vector<std::string> CsvParser::split_line(std::string_view line, char delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0;
    while (start <= line.size()) {
        size_t pos = line.find(delimiter, start);
        if (pos == std::string_view::npos) {
            tokens.push_back(trim(line.substr(start)));
            break;
        }
        tokens.push_back(trim(line.substr(start, pos - start)));
        start = pos + 1;
    }
    return tokens;
}

static bool is_valid_instrument_name(std::string_view inst) {
    if (inst.empty() || inst.size() > 64) return false;
    for (char c : inst) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '.' && c != '-' && c != '_') {
            return false;
        }
    }
    return true;
}

static bool parse_double_strict(const std::string& str, double& out_val) {
    if (str.empty()) return false;
    try {
        size_t idx = 0;
        double val = std::stod(str, &idx);
        if (idx != str.size()) {
            return false; // Trailing garbage characters
        }
        if (!std::isfinite(val) || val <= 0.0) {
            return false; // Must be finite and positive
        }
        out_val = val;
        return true;
    } catch (...) {
        return false;
    }
}

std::vector<PriceObservation> CsvParser::parse_file(const std::string& filepath) const {
    std::error_code ec;
    if (!std::filesystem::exists(filepath, ec)) {
        throw std::runtime_error("CSV Error: file does not exist: " + filepath);
    }
    if (!std::filesystem::is_regular_file(filepath, ec)) {
        throw std::runtime_error("CSV Error: path is not a regular file: " + filepath);
    }

    auto file_size = std::filesystem::file_size(filepath, ec);
    if (!ec && file_size > config_.max_file_size_bytes) {
        throw std::runtime_error("CSV Error: file size exceeds maximum permitted limit (" +
                                 std::to_string(config_.max_file_size_bytes) + " bytes): " + filepath);
    }

    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("CSV Error: failed to open file for reading: " + filepath);
    }

    return parse_stream(file);
}

std::vector<PriceObservation> CsvParser::parse_stream(std::istream& in) const {
    std::vector<PriceObservation> observations;
    std::set<std::pair<std::string, std::string>> seen_observations; // {instrument, date}

    std::string line;
    size_t line_number = 0;
    bool header_processed = false;

    while (std::getline(in, line)) {
        ++line_number;
        std::string trimmed = trim(line);
        if (trimmed.empty()) {
            continue; // Skip blank lines
        }

        if (!header_processed) {
            auto cols = split_line(trimmed, ',');
            if (cols.size() != 3) {
                throw std::runtime_error("CSV Error: line " + std::to_string(line_number) +
                                         ": header must contain exactly 3 columns (date,instrument,close)");
            }
            std::string c0 = cols[0];
            std::string c1 = cols[1];
            std::string c2 = cols[2];
            std::transform(c0.begin(), c0.end(), c0.begin(), ::tolower);
            std::transform(c1.begin(), c1.end(), c1.begin(), ::tolower);
            std::transform(c2.begin(), c2.end(), c2.begin(), ::tolower);

            if (c0 != "date" || c1 != "instrument" || c2 != "close") {
                throw std::runtime_error("CSV Error: line " + std::to_string(line_number) +
                                         ": invalid header columns. Expected 'date,instrument,close' but found '" +
                                         cols[0] + "," + cols[1] + "," + cols[2] + "'");
            }
            header_processed = true;
            continue;
        }

        auto cols = split_line(trimmed, ',');
        if (cols.size() != 3) {
            throw std::runtime_error("CSV Error: line " + std::to_string(line_number) +
                                     ": expected 3 comma-separated columns, found " + std::to_string(cols.size()));
        }

        const std::string& date_str = cols[0];
        const std::string& inst_str = cols[1];
        const std::string& close_str = cols[2];

        if (!is_valid_iso_date(date_str)) {
            throw std::runtime_error("CSV Error: line " + std::to_string(line_number) +
                                     ": invalid or malformed ISO-8601 date '" + date_str + "'");
        }

        if (!is_valid_instrument_name(inst_str)) {
            throw std::runtime_error("CSV Error: line " + std::to_string(line_number) +
                                     ": invalid instrument ticker symbol '" + inst_str + "'");
        }

        double close_price = 0.0;
        if (!parse_double_strict(close_str, close_price)) {
            throw std::runtime_error("CSV Error: line " + std::to_string(line_number) +
                                     ": invalid, non-finite, or non-positive close price '" + close_str + "'");
        }

        // Duplicate observation check per (instrument, date)
        if (seen_observations.find({inst_str, date_str}) != seen_observations.end()) {
            throw std::runtime_error("CSV Error: line " + std::to_string(line_number) +
                                     ": duplicate price observation for instrument '" + inst_str +
                                     "' on date '" + date_str + "'");
        }
        seen_observations.insert({inst_str, date_str});

        observations.push_back(PriceObservation{date_str, inst_str, close_price});

        if (observations.size() > config_.max_rows) {
            throw std::runtime_error("CSV Error: exceeded maximum permitted row count (" +
                                     std::to_string(config_.max_rows) + ")");
        }
    }

    if (!header_processed) {
        throw std::runtime_error("CSV Error: file is empty or missing required header 'date,instrument,close'");
    }

    return observations;
}

} // namespace wissen
