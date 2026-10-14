#include "test_harness.hpp"
#include "alert_engine.hpp"

using namespace wissen;

TEST_CASE(test_alert_engine_level_below_basic) {
    std::vector<PriceObservation> obs = {
        {"2026-09-01", "INFY", 1520.50},
        {"2026-09-02", "INFY", 1534.10},
        {"2026-09-03", "INFY", 1498.00}
    };
    std::vector<Rule> rules = {
        {"r1", "INFY", RuleType::Level, Direction::Below, 1500.0, "1500", 0, 0}
    };

    AlertEngine engine;
    auto alerts = engine.evaluate(obs, rules);

    ASSERT_EQ(alerts.size(), 1UL);
    ASSERT_EQ(alerts[0].date, "2026-09-03");
    ASSERT_EQ(alerts[0].rule_id, "r1");
    ASSERT_EQ(alerts[0].instrument, "INFY");
    ASSERT_EQ(alerts[0].details, "level below 1500");
}

TEST_CASE(test_alert_engine_level_above_basic) {
    std::vector<PriceObservation> obs = {
        {"2026-09-01", "TCS", 3000.0},
        {"2026-09-02", "TCS", 3200.0}
    };
    std::vector<Rule> rules = {
        {"r1", "TCS", RuleType::Level, Direction::Above, 3100.0, "3100", 0, 0}
    };

    AlertEngine engine;
    auto alerts = engine.evaluate(obs, rules);

    ASSERT_EQ(alerts.size(), 1UL);
    ASSERT_EQ(alerts[0].date, "2026-09-02");
    ASSERT_EQ(alerts[0].rule_id, "r1");
    ASSERT_EQ(alerts[0].instrument, "TCS");
    ASSERT_EQ(alerts[0].details, "level above 3100");
}

TEST_CASE(test_alert_engine_no_repeated_alerts_while_condition_remains) {
    // Price crosses below 1500 on day 2, and stays below on day 3 and day 4
    std::vector<PriceObservation> obs = {
        {"2026-09-01", "INFY", 1520.0},
        {"2026-09-02", "INFY", 1480.0}, // Crosses below -> Alert!
        {"2026-09-03", "INFY", 1470.0}, // Stays below -> NO Alert
        {"2026-09-04", "INFY", 1490.0}  // Stays below -> NO Alert
    };
    std::vector<Rule> rules = {
        {"r1", "INFY", RuleType::Level, Direction::Below, 1500.0, "1500", 0, 0}
    };

    AlertEngine engine;
    auto alerts = engine.evaluate(obs, rules);

    ASSERT_EQ(alerts.size(), 1UL);
    ASSERT_EQ(alerts[0].date, "2026-09-02");
}

TEST_CASE(test_alert_engine_re_crossing_fires_again) {
    // Below -> Above -> Below -> Above
    std::vector<PriceObservation> obs = {
        {"2026-09-01", "INFY", 1490.0},
        {"2026-09-02", "INFY", 1510.0}, // Crosses above -> Alert 1
        {"2026-09-03", "INFY", 1480.0}, // Crosses below
        {"2026-09-04", "INFY", 1520.0}  // Crosses above -> Alert 2
    };
    std::vector<Rule> rules = {
        {"r1", "INFY", RuleType::Level, Direction::Above, 1500.0, "1500", 0, 0}
    };

    AlertEngine engine;
    auto alerts = engine.evaluate(obs, rules);

    ASSERT_EQ(alerts.size(), 2UL);
    ASSERT_EQ(alerts[0].date, "2026-09-02");
    ASSERT_EQ(alerts[1].date, "2026-09-04");
}

TEST_CASE(test_alert_engine_equality_boundary_semantics) {
    // Test case 1: Price touches level exactly on day 2, then breaks below on day 3
    {
        std::vector<PriceObservation> obs = {
            {"2026-09-01", "INFY", 1520.0}, // Above
            {"2026-09-02", "INFY", 1500.0}, // Touches level (Equal)
            {"2026-09-03", "INFY", 1490.0}  // Breaks Below -> Must Fire!
        };
        std::vector<Rule> rules = {
            {"r1", "INFY", RuleType::Level, Direction::Below, 1500.0, "1500", 0, 0}
        };

        AlertEngine engine;
        auto alerts = engine.evaluate(obs, rules);

        ASSERT_EQ(alerts.size(), 1UL);
        ASSERT_EQ(alerts[0].date, "2026-09-03");
    }

    // Test case 2: Price touches level and bounces back up without crossing
    {
        std::vector<PriceObservation> obs = {
            {"2026-09-01", "INFY", 1520.0}, // Above
            {"2026-09-02", "INFY", 1500.0}, // Touches level
            {"2026-09-03", "INFY", 1530.0}  // Bounces Above -> No crossing below occurred
        };
        std::vector<Rule> rules = {
            {"r1", "INFY", RuleType::Level, Direction::Below, 1500.0, "1500", 0, 0}
        };

        AlertEngine engine;
        auto alerts = engine.evaluate(obs, rules);

        ASSERT_EQ(alerts.size(), 0UL);
    }
}

TEST_CASE(test_alert_engine_ma_cross_incomplete_windows) {
    // 5-day / 10-day MA.
    // Day 1 to 9: long MA undefined -> 0 alerts
    // Day 10: first day long MA defined -> baseline established, NO alert yet
    // Day 11: first possible day to alert
    std::vector<PriceObservation> obs;
    for (int i = 1; i <= 10; ++i) {
        std::string d = (i < 10) ? ("2026-09-0" + std::to_string(i)) : ("2026-09-" + std::to_string(i));
        obs.push_back({d, "TEST", 100.0});
    }

    std::vector<Rule> rules = {
        {"r1", "TEST", RuleType::MACross, Direction::Above, 0.0, "", 5, 10}
    };

    AlertEngine engine;
    auto alerts = engine.evaluate(obs, rules);

    ASSERT_EQ(alerts.size(), 0UL); // Exactly 10 days, baseline day, no alerts
}

TEST_CASE(test_alert_engine_ma_cross_fires_on_day_11) {
    // 3-day / 5-day MA
    // Day 1 to 5: prices 100, 100, 100, 100, 90
    // At Day 5: 3-day SMA = (100+100+90)/3 = 96.67
    //           5-day SMA = (100+100+100+100+90)/5 = 98.00
    //           3-day SMA < 5-day SMA (Below)
    // Day 6: price jumps to 150
    // At Day 6: 3-day SMA = (100+90+150)/3 = 113.33
    //           5-day SMA = (100+100+100+90+150)/5 = 108.00
    //           3-day SMA > 5-day SMA (Above) -> Crosses Above!
    std::vector<PriceObservation> obs = {
        {"2026-09-01", "XYZ", 100.0},
        {"2026-09-02", "XYZ", 100.0},
        {"2026-09-03", "XYZ", 100.0},
        {"2026-09-04", "XYZ", 100.0},
        {"2026-09-05", "XYZ", 90.0},
        {"2026-09-06", "XYZ", 150.0}
    };

    std::vector<Rule> rules = {
        {"r1", "XYZ", RuleType::MACross, Direction::Above, 0.0, "", 3, 5}
    };

    AlertEngine engine;
    auto alerts = engine.evaluate(obs, rules);

    ASSERT_EQ(alerts.size(), 1UL);
    ASSERT_EQ(alerts[0].date, "2026-09-06");
    ASSERT_EQ(alerts[0].rule_id, "r1");
    ASSERT_EQ(alerts[0].instrument, "XYZ");
    ASSERT_EQ(alerts[0].details, "ma_cross 3/5 above");
}

TEST_CASE(test_alert_engine_unsorted_multiple_instruments_and_ordering) {
    // Multiple instruments interleaved out of chronological order
    std::vector<PriceObservation> obs = {
        {"2026-09-03", "INFY", 1490.0},
        {"2026-09-01", "INFY", 1520.0},
        {"2026-09-02", "INFY", 1510.0},
        {"2026-09-01", "TCS", 3000.0},
        {"2026-09-03", "TCS", 3200.0},
        {"2026-09-02", "TCS", 3050.0}
    };

    // Both rules fire on 2026-09-03:
    // r1: INFY level below 1500 (1510 -> 1490)
    // r2: TCS level above 3100 (3050 -> 3200)
    std::vector<Rule> rules = {
        {"r2", "TCS", RuleType::Level, Direction::Above, 3100.0, "3100", 0, 0},
        {"r1", "INFY", RuleType::Level, Direction::Below, 1500.0, "1500", 0, 0}
    };

    AlertEngine engine;
    auto alerts = engine.evaluate(obs, rules);

    ASSERT_EQ(alerts.size(), 2UL);
    // Ordered by date ascending, then rule_id ascending ("r1" before "r2")
    ASSERT_EQ(alerts[0].date, "2026-09-03");
    ASSERT_EQ(alerts[0].rule_id, "r1");
    ASSERT_EQ(alerts[0].instrument, "INFY");

    ASSERT_EQ(alerts[1].date, "2026-09-03");
    ASSERT_EQ(alerts[1].rule_id, "r2");
    ASSERT_EQ(alerts[1].instrument, "TCS");
}

TEST_CASE(test_alert_engine_instrument_not_in_data) {
    std::vector<PriceObservation> obs = {
        {"2026-09-01", "INFY", 1520.0},
        {"2026-09-02", "INFY", 1480.0}
    };
    std::vector<Rule> rules = {
        {"r_apple", "AAPL", RuleType::Level, Direction::Below, 100.0, "100", 0, 0}
    };

    AlertEngine engine;
    auto alerts = engine.evaluate(obs, rules);
    ASSERT_EQ(alerts.size(), 0UL);
}
