#pragma once

#include "models.hpp"
#include <string>
#include <vector>
#include <cstddef>
#include <istream>

namespace wissen {

struct JsonParserConfig {
    size_t max_file_size_bytes{10 * 1024 * 1024}; // 10 MB limit
    size_t max_rules{10'000};                     // 10,000 rules limit
    int max_ma_window{10'000};                    // Maximum window size
};

class JsonParser {
public:
    explicit JsonParser(JsonParserConfig config = JsonParserConfig{})
        : config_(config) {}

    /**
     * Parses the JSON alert rules file at the given filepath.
     * Throws std::runtime_error on I/O error, JSON syntax error, schema error,
     * invalid rule parameters, duplicate rule IDs, or when limits are exceeded.
     */
    std::vector<Rule> parse_file(const std::string& filepath) const;

    /**
     * Parses JSON content directly from an input stream.
     */
    std::vector<Rule> parse_stream(std::istream& in) const;

private:
    JsonParserConfig config_;
};

} // namespace wissen
