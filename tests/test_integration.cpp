#include "test_harness.hpp"
#include "csv_parser.hpp"
#include "json_parser.hpp"
#include "alert_engine.hpp"
#include "output_formatter.hpp"

#include <fstream>
#include <sstream>

using namespace wissen;

static std::string resolve_sample_path(const std::string& rel) {
    if (std::filesystem::exists("sample/" + rel)) {
        return "sample/" + rel;
    }
    if (std::filesystem::exists("../sample/" + rel)) {
        return "../sample/" + rel;
    }
    return rel;
}

TEST_CASE(test_integration_sample_dataset_matches_brief) {
    std::string prices_file = resolve_sample_path("prices.csv");
    std::string rules_file = resolve_sample_path("rules.json");
    std::string expected_file_path = resolve_sample_path("expected_output.txt");

    CsvParser csv_parser;
    auto obs = csv_parser.parse_file(prices_file);

    JsonParser json_parser;
    auto rules = json_parser.parse_file(rules_file);

    AlertEngine engine;
    auto alerts = engine.evaluate(obs, rules);

    std::ostringstream oss;
    OutputFormatter::print_alerts(alerts, oss);
    std::string actual_output = oss.str();

    std::ifstream expected_file(expected_file_path);
    ASSERT_TRUE(expected_file.is_open());
    std::stringstream buffer;
    buffer << expected_file.rdbuf();
    std::string expected_output = buffer.str();

    ASSERT_EQ(actual_output, expected_output);
}

TEST_CASE(test_integration_nonexistent_files) {
    CsvParser csv_parser;
    ASSERT_THROWS(csv_parser.parse_file("sample/nonexistent_prices.csv"), std::runtime_error);

    JsonParser json_parser;
    ASSERT_THROWS(json_parser.parse_file("sample/nonexistent_rules.json"), std::runtime_error);
}
