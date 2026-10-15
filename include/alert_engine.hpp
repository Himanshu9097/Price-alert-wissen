#pragma once

#include "models.hpp"
#include <vector>

namespace wissen {

/**
 * AlertEngine evaluates price observation time-series against alert rules.
 *
 * Crossing & Equality Semantics:
 * 1. Price/MA state relative to threshold is classified using a strict 3-state model:
 *    - Above: (value - threshold) > EPSILON
 *    - Below: (value - threshold) < -EPSILON
 *    - Equal: |value - threshold| <= EPSILON
 *    EPSILON is set to 1e-9 to absorb IEEE-754 binary floating-point representation
 *    artifacts while remaining orders of magnitude below real market price ticks (0.01).
 *
 * 2. Level Crossing:
 *    - Fires on day t when price enters the target side having originated from the
 *      opposite side.
 *    - If price touches the exact boundary (Equal), the previously confirmed side is
 *      preserved. A breakthrough to the opposite side triggers an alert; a bounce back
 *      does not.
 *    - Consecutive days on the same side do not generate repeated alerts.
 *
 * 3. Moving Average Crossover:
 *    - SMAs require a full window before they are defined.
 *    - For windows S and L (S < L), day (L - 1) is the first day both MAs exist and
 *      establishes the baseline state. Day L is the earliest day an alert can fire.
 *    - Performance: Evaluated using O(1) rolling sums with periodic re-anchoring
 *      every 256 steps to eliminate cumulative floating-point roundoff drift.
 */
class AlertEngine {
public:
    AlertEngine() = default;

    /**
     * Evaluates all rules against the price observations.
     * Observations are grouped by instrument and sorted chronologically.
     * Returns triggered alerts sorted by date ascending, then rule ID ascending.
     */
    std::vector<Alert> evaluate(
        const std::vector<PriceObservation>& observations,
        const std::vector<Rule>& rules) const;

    /**
     * Reference implementation computing SMA via direct window summation O(W).
     * Used for correctness testing and periodic re-anchoring of rolling sums.
     */
    static double compute_sma_reference(
        const std::vector<PriceObservation>& series,
        size_t end_idx,
        int window);

private:
    enum class Side {
        Unknown,
        Above,
        Below,
        Equal
    };

    static constexpr double EPSILON = 1e-9;
    static constexpr size_t REANCHOR_INTERVAL = 256;

    static Side get_side(double value, double threshold);

    void evaluate_level_rule(
        const Rule& rule,
        const std::vector<PriceObservation>& series,
        std::vector<Alert>& out_alerts) const;

    void evaluate_ma_cross_rule(
        const Rule& rule,
        const std::vector<PriceObservation>& series,
        std::vector<Alert>& out_alerts) const;

    static double compute_window_sum(
        const std::vector<PriceObservation>& series,
        size_t end_idx,
        size_t window);
};

} // namespace wissen
