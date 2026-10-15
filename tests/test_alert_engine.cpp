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

TEST_CASE(test_alert_engine_level_oscillation_around_boundary) {
    // Level is 100.0
    // Day 1: 90 (Below)
    // Day 2: 100 (Touches level) -> No alert
    // Day 3: 110 (Above) -> Crosses Above (Alert 1: Day 3)
    // Day 4: 100 (Touches level) -> No alert
    // Day 5: 95 (Below) -> Crosses Below (Alert 2: Day 5)
    // Day 6: 100 (Touches level) -> No alert
    // Day 7: 90 (Below) -> Bounced back below without crossing above (No alert)
    std::vector<PriceObservation> obs = {
        {"2026-09-01", "TEST", 90.0},
        {"2026-09-02", "TEST", 100.0},
        {"2026-09-03", "TEST", 110.0},
        {"2026-09-04", "TEST", 100.0},
        {"2026-09-05", "TEST", 95.0},
        {"2026-09-06", "TEST", 100.0},
        {"2026-09-07", "TEST", 90.0}
    };

    std::vector<Rule> rules = {
        {"r_above", "TEST", RuleType::Level, Direction::Above, 100.0, "100", 0, 0},
        {"r_below", "TEST", RuleType::Level, Direction::Below, 100.0, "100", 0, 0}
    };

    AlertEngine engine;
    auto alerts = engine.evaluate(obs, rules);

    ASSERT_EQ(alerts.size(), 2UL);
    ASSERT_EQ(alerts[0].date, "2026-09-03");
    ASSERT_EQ(alerts[0].rule_id, "r_above");

    ASSERT_EQ(alerts[1].date, "2026-09-05");
    ASSERT_EQ(alerts[1].rule_id, "r_below");
}

TEST_CASE(test_alert_engine_ma_cross_below_direction) {
    // 2-day / 4-day MA with direction "below"
    // Day 1: 100
    // Day 2: 100
    // Day 3: 100
    // Day 4: 100 -> Baseline: 2-SMA=100, 4-SMA=100 (Equal)
    // Day 5: 120 -> 2-SMA=(100+120)/2=110, 4-SMA=(100+100+100+120)/4=105 (Above)
    // Day 6: 60  -> 2-SMA=(120+60)/2=90,  4-SMA=(100+100+120+60)/4=95   (Below) -> Crosses Below!
    std::vector<PriceObservation> obs = {
        {"2026-09-01", "MA_TEST", 100.0},
        {"2026-09-02", "MA_TEST", 100.0},
        {"2026-09-03", "MA_TEST", 100.0},
        {"2026-09-04", "MA_TEST", 100.0},
        {"2026-09-05", "MA_TEST", 120.0},
        {"2026-09-06", "MA_TEST", 60.0}
    };

    std::vector<Rule> rules = {
        {"r_death_cross", "MA_TEST", RuleType::MACross, Direction::Below, 0.0, "", 2, 4}
    };

    AlertEngine engine;
    auto alerts = engine.evaluate(obs, rules);

    ASSERT_EQ(alerts.size(), 1UL);
    ASSERT_EQ(alerts[0].date, "2026-09-06");
    ASSERT_EQ(alerts[0].rule_id, "r_death_cross");
    ASSERT_EQ(alerts[0].details, "ma_cross 2/4 below");
}

TEST_CASE(test_alert_engine_rule_id_lexicographical_sort) {
    // 3 rules firing on the exact same date for the same instrument
    std::vector<PriceObservation> obs = {
        {"2026-09-01", "MULTI", 50.0},
        {"2026-09-02", "MULTI", 150.0} // Crosses above 60, 80, 100
    };

    std::vector<Rule> rules = {
        {"rule_C", "MULTI", RuleType::Level, Direction::Above, 100.0, "100", 0, 0},
        {"rule_A", "MULTI", RuleType::Level, Direction::Above, 60.0, "60", 0, 0},
        {"rule_B", "MULTI", RuleType::Level, Direction::Above, 80.0, "80", 0, 0}
    };

    AlertEngine engine;
    auto alerts = engine.evaluate(obs, rules);

    ASSERT_EQ(alerts.size(), 3UL);
    ASSERT_EQ(alerts[0].rule_id, "rule_A");
    ASSERT_EQ(alerts[1].rule_id, "rule_B");
    ASSERT_EQ(alerts[2].rule_id, "rule_C");
}

TEST_CASE(test_alert_engine_rolling_sma_matches_reference_implementation) {
    // Generate 600 synthetic prices with varying values spanning multiple re-anchoring cycles
    std::vector<PriceObservation> series;
    series.reserve(600);
    double price = 1000.0;
    for (int i = 0; i < 600; ++i) {
        price += ((i % 2 == 0) ? 1.75 : -1.25) + (i % 7) * 0.10;
        series.push_back({"2026-01-01", "TEST_SMA", price});
    }

    int window = 20;
    // Compute moving averages using reference implementation and compare against rolling sum
    double rolling_sum = 0.0;
    for (size_t i = 0; i < static_cast<size_t>(window); ++i) {
        rolling_sum += series[i].close;
    }

    constexpr size_t REANCHOR_INTERVAL = 256;
    size_t steps_since_reanchor = 0;

    for (size_t t = static_cast<size_t>(window); t < series.size(); ++t) {
        rolling_sum += series[t].close - series[t - static_cast<size_t>(window)].close;
        ++steps_since_reanchor;

        if (steps_since_reanchor >= REANCHOR_INTERVAL) {
            double recomputed = 0.0;
            for (size_t j = t - static_cast<size_t>(window) + 1; j <= t; ++j) {
                recomputed += series[j].close;
            }
            rolling_sum = recomputed;
            steps_since_reanchor = 0;
        }

        double rolling_sma = rolling_sum / static_cast<double>(window);
        double ref_sma = AlertEngine::compute_sma_reference(series, t, window);

        // Difference must be smaller than 1e-11 (demonstrating zero cumulative drift)
        double diff = std::abs(rolling_sma - ref_sma);
        ASSERT_TRUE(diff < 1e-11);
    }
}

TEST_CASE(test_alert_engine_ma_equality_touch_and_diverge) {
    // 2-day vs 4-day moving average where MAs originate Below, touch Equal, and diverge Above
    // Day 1 to 3: 100.0
    // Day 4: 80.0 -> Baseline (Day 4): 2-SMA = 90.0, 4-SMA = 95.0 (Side::Below)
    // Day 5: 120.0 -> Day 5: 2-SMA = (80+120)/2 = 100.0, 4-SMA = (100+100+80+120)/4 = 100.0 (Side::Equal touch)
    // Day 6: 130.0 -> Day 6: 2-SMA = (120+130)/2 = 125.0, 4-SMA = (100+80+120+130)/4 = 107.5 (Side::Above) -> Fires!
    std::vector<PriceObservation> obs = {
        {"2026-09-01", "MA_EQ", 100.0},
        {"2026-09-02", "MA_EQ", 100.0},
        {"2026-09-03", "MA_EQ", 100.0},
        {"2026-09-04", "MA_EQ", 80.0},
        {"2026-09-05", "MA_EQ", 120.0},
        {"2026-09-06", "MA_EQ", 130.0}
    };

    std::vector<Rule> rules = {
        {"r_ma_eq", "MA_EQ", RuleType::MACross, Direction::Above, 0.0, "", 2, 4}
    };

    AlertEngine engine;
    auto alerts = engine.evaluate(obs, rules);

    ASSERT_EQ(alerts.size(), 1UL);
    ASSERT_EQ(alerts[0].date, "2026-09-06");
    ASSERT_EQ(alerts[0].rule_id, "r_ma_eq");
}

TEST_CASE(test_alert_engine_crossover_immediately_on_day_after_long_window) {
    // 2-day / 3-day MA
    // Day 1: 100
    // Day 2: 100
    // Day 3: 80 -> Baseline (Day 3): 2-SMA = (100+80)/2 = 90, 3-SMA = (100+100+80)/3 = 93.33 (Below)
    // Day 4: 150 -> Day 4: 2-SMA = (80+150)/2 = 115, 3-SMA = (100+80+150)/3 = 110 (Above) -> Fires Day 4!
    std::vector<PriceObservation> obs = {
        {"2026-09-01", "MA_EARLY", 100.0},
        {"2026-09-02", "MA_EARLY", 100.0},
        {"2026-09-03", "MA_EARLY", 80.0},
        {"2026-09-04", "MA_EARLY", 150.0}
    };

    std::vector<Rule> rules = {
        {"r_early", "MA_EARLY", RuleType::MACross, Direction::Above, 0.0, "", 2, 3}
    };

    AlertEngine engine;
    auto alerts = engine.evaluate(obs, rules);

    ASSERT_EQ(alerts.size(), 1UL);
    ASSERT_EQ(alerts[0].date, "2026-09-04");
    ASSERT_EQ(alerts[0].rule_id, "r_early");
}
