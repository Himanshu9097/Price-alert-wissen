#include "test_harness.hpp"
#include "csv_parser.hpp"

#include <sstream>
#include <stdexcept>

using namespace wissen;

TEST_CASE(test_csv_parser_valid_stream) {
    std::string data =
        "date,instrument,close\n"
        "2026-09-01, INFY , 1520.50\n"
        "2026-09-01,TCS,3890.00\r\n"
        "\n"
        "2026-09-02,INFY,1534.10\n";

    std::istringstream iss(data);
    CsvParser parser;
    auto obs = parser.parse_stream(iss);

    ASSERT_EQ(obs.size(), 3UL);
    ASSERT_EQ(obs[0].date, "2026-09-01");
    ASSERT_EQ(obs[0].instrument, "INFY");
    ASSERT_EQ(obs[0].close, 1520.50);

    ASSERT_EQ(obs[1].date, "2026-09-01");
    ASSERT_EQ(obs[1].instrument, "TCS");
    ASSERT_EQ(obs[1].close, 3890.00);

    ASSERT_EQ(obs[2].date, "2026-09-02");
    ASSERT_EQ(obs[2].instrument, "INFY");
    ASSERT_EQ(obs[2].close, 1534.10);
}

TEST_CASE(test_csv_parser_missing_header) {
    std::string data =
        "2026-09-01,INFY,1520.50\n"
        "2026-09-02,INFY,1534.10\n";

    std::istringstream iss(data);
    CsvParser parser;
    ASSERT_THROWS(parser.parse_stream(iss), std::runtime_error);
}

TEST_CASE(test_csv_parser_invalid_header_columns) {
    std::string data =
        "timestamp,instrument,close\n"
        "2026-09-01,INFY,1520.50\n";

    std::istringstream iss(data);
    CsvParser parser;
    ASSERT_THROWS(parser.parse_stream(iss), std::runtime_error);
}

TEST_CASE(test_csv_parser_invalid_column_count) {
    std::string data =
        "date,instrument,close\n"
        "2026-09-01,INFY\n";

    std::istringstream iss(data);
    CsvParser parser;
    ASSERT_THROWS(parser.parse_stream(iss), std::runtime_error);
}

TEST_CASE(test_csv_parser_invalid_close_prices) {
    // Non-numeric price
    {
        std::string data = "date,instrument,close\n2026-09-01,INFY,abc\n";
        std::istringstream iss(data);
        ASSERT_THROWS(CsvParser{}.parse_stream(iss), std::runtime_error);
    }
    // Trailing chars
    {
        std::string data = "date,instrument,close\n2026-09-01,INFY,1500.5xyz\n";
        std::istringstream iss(data);
        ASSERT_THROWS(CsvParser{}.parse_stream(iss), std::runtime_error);
    }
    // Negative price
    {
        std::string data = "date,instrument,close\n2026-09-01,INFY,-1500.0\n";
        std::istringstream iss(data);
        ASSERT_THROWS(CsvParser{}.parse_stream(iss), std::runtime_error);
    }
    // Zero price
    {
        std::string data = "date,instrument,close\n2026-09-01,INFY,0.0\n";
        std::istringstream iss(data);
        ASSERT_THROWS(CsvParser{}.parse_stream(iss), std::runtime_error);
    }
    // NaN
    {
        std::string data = "date,instrument,close\n2026-09-01,INFY,NaN\n";
        std::istringstream iss(data);
        ASSERT_THROWS(CsvParser{}.parse_stream(iss), std::runtime_error);
    }
}

TEST_CASE(test_csv_parser_duplicate_observation_rejected) {
    std::string data =
        "date,instrument,close\n"
        "2026-09-01,INFY,1520.50\n"
        "2026-09-01,INFY,1522.00\n"; // Duplicate INFY on 2026-09-01

    std::istringstream iss(data);
    CsvParser parser;
    ASSERT_THROWS(parser.parse_stream(iss), std::runtime_error);
}

TEST_CASE(test_csv_parser_empty_input) {
    std::string data = "";
    std::istringstream iss(data);
    CsvParser parser;
    ASSERT_THROWS(parser.parse_stream(iss), std::runtime_error);
}

TEST_CASE(test_csv_parser_max_rows_limit) {
    std::string data =
        "date,instrument,close\n"
        "2026-09-01,INFY,1520.50\n"
        "2026-09-02,INFY,1530.00\n";

    std::istringstream iss(data);
    CsvParserConfig cfg;
    cfg.max_rows = 1; // Limit to 1 row
    CsvParser parser(cfg);
    ASSERT_THROWS(parser.parse_stream(iss), std::runtime_error);
}
