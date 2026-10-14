#pragma once

#include "models.hpp"
#include <vector>

namespace wissen {

class AlertEngine {
public:
    AlertEngine() = default;

    /**
     * Evaluates all rules against the price observations.
     * Groups observations per instrument, sorts them chronologically,
     * computes level crossings and simple moving average crossovers,
     * and returns all generated alerts sorted by date ascending, then rule ID ascending.
     */
    std::vector<Alert> evaluate(
        const std::vector<PriceObservation>& observations,
        const std::vector<Rule>& rules) const;

private:
    enum class Side {
        Unknown,
        Above,
        Below,
        Equal
    };

    static constexpr double EPSILON = 1e-9;

    static Side get_side(double value, double threshold);

    void evaluate_level_rule(
        const Rule& rule,
        const std::vector<PriceObservation>& series,
        std::vector<Alert>& out_alerts) const;

    void evaluate_ma_cross_rule(
        const Rule& rule,
        const std::vector<PriceObservation>& series,
        std::vector<Alert>& out_alerts) const;

    static double compute_sma(
        const std::vector<PriceObservation>& series,
        size_t end_idx,
        int window);
};

} // namespace wissen
