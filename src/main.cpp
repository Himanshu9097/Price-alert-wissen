#include "csv_parser.hpp"
#include "json_parser.hpp"
#include "alert_engine.hpp"
#include "output_formatter.hpp"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    // Fast I/O
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    if (argc != 3) {
        std::cerr << "Usage: price_alert <prices.csv> <rules.json>\n"
                  << "Description: Reads daily prices CSV and rules JSON, printing triggered alerts.\n";
        return 1;
    }

    const std::string prices_path = argv[1];
    const std::string rules_path = argv[2];

    try {
        wissen::CsvParser csv_parser;
        auto observations = csv_parser.parse_file(prices_path);

        wissen::JsonParser json_parser;
        auto rules = json_parser.parse_file(rules_path);

        wissen::AlertEngine engine;
        auto alerts = engine.evaluate(observations, rules);

        wissen::OutputFormatter::print_alerts(alerts, std::cout);

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Fatal Error: unknown error occurred\n";
        return 1;
    }
}
