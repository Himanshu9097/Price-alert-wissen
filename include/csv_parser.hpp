#pragma once

#include "models.hpp"
#include <string>
#include <vector>
#include <string_view>
#include <cstddef>

namespace wissen {

struct CsvParserConfig {
    size_t max_file_size_bytes{50 * 1024 * 1024}; // 50 MB limit
    size_t max_rows{1'000'000};                   // 1 Million rows limit
};

class CsvParser {
public:
    explicit CsvParser(CsvParserConfig config = CsvParserConfig{})
        : config_(config) {}

    /**
     * Parses the CSV file at the given filepath.
     * Throws std::runtime_error on I/O error, invalid header, malformed row,
     * invalid date, non-finite or non-positive close price, duplicate observation,
     * or when resource limits are exceeded.
     */
    std::vector<PriceObservation> parse_file(const std::string& filepath) const;

    /**
     * Parses CSV content directly from an input stream.
     */
    std::vector<PriceObservation> parse_stream(std::istream& in) const;

private:
    CsvParserConfig config_;

    static std::string trim(std::string_view sv);
    static std::vector<std::string> split_line(std::string_view line, char delimiter = ',');
};

} // namespace wissen
