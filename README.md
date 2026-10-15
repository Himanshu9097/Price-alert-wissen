# Price Alert CLI Tool (C++17)

Production-quality command-line tool built for the Wissen Technology Campus Assessment. It reads daily closing prices from CSV, applies level-crossing and moving-average crossover rules from JSON, and outputs triggered alerts in chronological and rule-ID order.

---

## 1. How to Run

### Prerequisites
- Any C++17 compiler (`clang++` >= 10 or `g++` >= 9) and `make` (or `cmake` >= 3.16).
- **Zero external package dependencies** (bundled header-only `third_party/nlohmann/json.hpp`).

### Run from a Clean Download (One Command)
```bash
./run.sh sample/prices.csv sample/rules.json
```
*(The script automatically compiles the optimized binary via `make` and executes it).*

### Manual Build & Test Commands
```bash
# Build with Make and run tests
make -j4
make test

# Or build with CMake and run CTest
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
ctest --test-dir build --output-on-failure
```

**Expected Output:**
```text
2026-09-03 r1 INFY level below 1500
2026-09-24 r2 TCS ma_cross 5/20 above
```

---

## 2. How the Assistant Was Used (Prompts in Brief)

- **Architecture Scaffolding**: Prompted the assistant to scaffold a modular C++17 pipeline separating CSV parsing, JSON rule schema validation, alert evaluation, and output formatting.
- **Adversarial Test Vectors**: Prompted the assistant to generate calendar edge cases (century leap years, month boundaries) and corrupted feeds (NaN, negative prices, duplicate dates).
- **Optimization**: Prompted the assistant to improve moving average evaluation from $O(NW)$ window re-summation to $O(N)$ rolling sums with periodic re-anchoring to prevent floating-point drift.

---

## 3. What the Assistant Got Wrong & Corrections Made

During code review and verification, two genuine domain defects were caught in the assistant's initial logic:

1. **Equality Boundary "Touch" Glitch (Level Crossing)**:
   - *Assistant's Mistake*: Used a naive check `prev.close > level && curr.close < level`. When a stock closed exactly on the level ($P = L$) on day $t-1$ and dropped below on day $t$, the alert was lost because $P_{t-1} > L$ was false.
   - *Correction*: Engineered a persistent 3-state transition model (`Above`, `Below`, `Equal`). When price touches $L$, the previous confirmed side is preserved. If it subsequently moves to `Below`, the alert triggers correctly. If it bounces back `Above`, no alert fires. Verified by `test_alert_engine_equality_boundary_semantics`.

2. **Premature MA Crossover on Baseline Day**:
   - *Assistant's Mistake*: Attempted to detect crossovers starting at day index $L-1$ (the first day the long window is complete) against an uninitialized prior state.
   - *Correction*: Day $L-1$ is strictly enforced as the *baseline state*. Day $L$ (trading day $L+1$) is the earliest day an MA crossover can fire. Verified by `test_alert_engine_ma_cross_incomplete_windows`.

---

## 4. Verification Summary

- **Unit & Integration Suite**: 39 automated tests covering calendar validation, header safety, duplicate observation rejection, $O(1)$ rolling SMA numerical stability, and adversarial multi-instrument feeds.
- **CLI Contract Suite**: 9 automated end-to-end tests validating exit codes, `stderr` diagnostic separation, and clean `stdout`.
- **Sanitizers**: Verified with Clang `-fsanitize=address,undefined` (0 memory leaks, 0 undefined behavior).
