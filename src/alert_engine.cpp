#include "alert_engine.hpp"

#include <map>
#include <algorithm>
#include <cmath>

namespace wissen {

AlertEngine::Side AlertEngine::get_side(double value, double threshold) {
    double diff = value - threshold;
    if (diff > EPSILON) {
        return Side::Above;
    }
    if (diff < -EPSILON) {
        return Side::Below;
    }
    return Side::Equal;
}

double AlertEngine::compute_sma(
    const std::vector<PriceObservation>& series,
    size_t end_idx,
    int window) {
    double sum = 0.0;
    size_t start_idx = end_idx - static_cast<size_t>(window) + 1;
    for (size_t i = start_idx; i <= end_idx; ++i) {
        sum += series[i].close;
    }
    return sum / static_cast<double>(window);
}

void AlertEngine::evaluate_level_rule(
    const Rule& rule,
    const std::vector<PriceObservation>& series,
    std::vector<Alert>& out_alerts) const {
    if (series.size() < 2) {
        // A crossover requires at least 2 consecutive observations
        return;
    }

    Side prev_side = get_side(series[0].close, rule.level);

    for (size_t t = 1; t < series.size(); ++t) {
        Side curr_side = get_side(series[t].close, rule.level);

        if (rule.direction == Direction::Above) {
            if (curr_side == Side::Above) {
                if (prev_side == Side::Below) {
                    out_alerts.push_back(Alert{
                        series[t].date,
                        rule.id,
                        rule.instrument,
                        RuleType::Level,
                        Direction::Above,
                        "level above " + rule.raw_level_str
                    });
                }
                prev_side = Side::Above;
            } else if (curr_side == Side::Below) {
                prev_side = Side::Below;
            }
            // If Side::Equal, prev_side remains unchanged (boundary touch)
        } else if (rule.direction == Direction::Below) {
            if (curr_side == Side::Below) {
                if (prev_side == Side::Above) {
                    out_alerts.push_back(Alert{
                        series[t].date,
                        rule.id,
                        rule.instrument,
                        RuleType::Level,
                        Direction::Below,
                        "level below " + rule.raw_level_str
                    });
                }
                prev_side = Side::Below;
            } else if (curr_side == Side::Above) {
                prev_side = Side::Above;
            }
            // If Side::Equal, prev_side remains unchanged (boundary touch)
        }
    }
}

void AlertEngine::evaluate_ma_cross_rule(
    const Rule& rule,
    const std::vector<PriceObservation>& series,
    std::vector<Alert>& out_alerts) const {
    size_t long_win = static_cast<size_t>(rule.long_window);
    size_t short_win = static_cast<size_t>(rule.short_window);

    // Both moving averages need a full window before they are defined.
    // To detect a crossover, we need at least (long_window + 1) trading days:
    // day (long_window - 1) establishes the baseline state,
    // and day (long_window) is the first day that can trigger a crossover.
    if (series.size() <= long_win) {
        return;
    }

    // Baseline at index (long_window - 1): first day both MAs exist
    size_t baseline_idx = long_win - 1;
    double short_sma_base = compute_sma(series, baseline_idx, static_cast<int>(short_win));
    double long_sma_base = compute_sma(series, baseline_idx, static_cast<int>(long_win));
    Side prev_side = get_side(short_sma_base, long_sma_base);

    std::string ma_tag = std::to_string(rule.short_window) + "/" + std::to_string(rule.long_window);

    for (size_t t = long_win; t < series.size(); ++t) {
        double short_sma = compute_sma(series, t, static_cast<int>(short_win));
        double long_sma = compute_sma(series, t, static_cast<int>(long_win));
        Side curr_side = get_side(short_sma, long_sma);

        if (rule.direction == Direction::Above) {
            if (curr_side == Side::Above) {
                if (prev_side == Side::Below) {
                    out_alerts.push_back(Alert{
                        series[t].date,
                        rule.id,
                        rule.instrument,
                        RuleType::MACross,
                        Direction::Above,
                        "ma_cross " + ma_tag + " above"
                    });
                }
                prev_side = Side::Above;
            } else if (curr_side == Side::Below) {
                prev_side = Side::Below;
            }
            // If Side::Equal, prev_side remains unchanged (boundary touch)
        } else if (rule.direction == Direction::Below) {
            if (curr_side == Side::Below) {
                if (prev_side == Side::Above) {
                    out_alerts.push_back(Alert{
                        series[t].date,
                        rule.id,
                        rule.instrument,
                        RuleType::MACross,
                        Direction::Below,
                        "ma_cross " + ma_tag + " below"
                    });
                }
                prev_side = Side::Below;
            } else if (curr_side == Side::Above) {
                prev_side = Side::Above;
            }
            // If Side::Equal, prev_side remains unchanged (boundary touch)
        }
    }
}

std::vector<Alert> AlertEngine::evaluate(
    const std::vector<PriceObservation>& observations,
    const std::vector<Rule>& rules) const {
    // Group observations by instrument
    std::map<std::string, std::vector<PriceObservation>> inst_series;
    for (const auto& obs : observations) {
        inst_series[obs.instrument].push_back(obs);
    }

    // Sort observations chronologically per instrument
    for (auto& [inst, series] : inst_series) {
        std::sort(series.begin(), series.end(), [](const PriceObservation& a, const PriceObservation& b) {
            return a.date < b.date;
        });
    }

    // Group rules by instrument
    std::map<std::string, std::vector<Rule>> inst_rules;
    for (const auto& rule : rules) {
        inst_rules[rule.instrument].push_back(rule);
    }

    std::vector<Alert> alerts;

    // Evaluate each instrument's rules independently
    for (const auto& [inst, rules_for_inst] : inst_rules) {
        auto it = inst_series.find(inst);
        if (it == inst_series.end()) {
            // No price observations for this instrument
            continue;
        }

        const auto& series = it->second;

        for (const auto& rule : rules_for_inst) {
            if (rule.type == RuleType::Level) {
                evaluate_level_rule(rule, series, alerts);
            } else if (rule.type == RuleType::MACross) {
                evaluate_ma_cross_rule(rule, series, alerts);
            }
        }
    }

    // Sort all alerts: primary by date ascending, secondary by rule_id ascending
    std::sort(alerts.begin(), alerts.end());

    return alerts;
}

} // namespace wissen
