# Price Alert CLI Tool (C++17)

Production-grade command-line tool built for the Wissen Technology Campus Assessment. It monitors daily closing prices across multiple financial instruments and detects level crossings and moving-average crossovers based on declarative JSON rules.

---

## 1. Quick Start & Build Instructions

### Prerequisites
- Any C++17 compatible compiler (`clang++` $\ge 10$ or `g++` $\ge 9$)
- Standard `make` (or CMake $\ge 3.16$)
- **Zero external package dependencies** (bundled header-only `nlohmann/json.hpp`).

### Run with One Command
From a clean download/clone, execute:
```bash
./run.sh sample/prices.csv sample/rules.json
```
*(The script automatically compiles the optimized binary on first run and executes it).*

### Manual Build Options
- **Via Make**:
  ```bash
  make -j4
  ./price_alert sample/prices.csv sample/rules.json
  ```
- **Via CMake**:
  ```bash
  cmake -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build
  ./build/price_alert sample/prices.csv sample/rules.json
  ```

### Running Tests
```bash
make test
# or with CMake:
ctest --test-dir build --output-on-failure
```

---

## 2. How the AI Assistant Was Used (Prompts in Brief)

1. **Architecture Scaffolding**: Prompted the assistant to draft a modular single-responsibility C++17 structure (`csv_parser`, `json_parser`, `alert_engine`, `output_formatter`) adhering to strict RAII, const-correctness, and zero global mutable state.
2. **Adversarial Test Generation**: Prompted the assistant to generate edge cases for date parsing (century leap years, month limits), numerical parsing (non-finite doubles, negative prices, trailing characters), and malformed JSON schemas.
3. **Domain Logic Review**: Prompted the assistant to audit crossover conditions against financial market semantics and trading-day window constraints.

---

## 3. What the Assistant Got Wrong & Corrections Made

During code review and verification, two critical domain defects were identified in the assistant's initial logic:

1. **Equality Boundary "Touch" Glitch (Level Crossing)**:
   - *Assistant's Mistake*: Used a naive immediate 1-day check: `prev.close > level && curr.close < level`. When a stock closed exactly on the level ($P = L$) on day $t-1$ and dropped below ($P < L$) on day $t$, the alert was silently lost because $P_{t-1} > L$ was false.
   - *Correction*: Engineered a persistent 3-state transition model (`Above`, `Below`, `Equal`). When price touches $L$, the previous confirmed side is preserved. If it subsequently moves to `Below`, the alert triggers correctly. If it bounces back `Above`, no alert fires. Verified by `test_alert_engine_equality_boundary_semantics`.

2. **Premature MA Crossover on Baseline Day**:
   - *Assistant's Mistake*: Began crossover evaluation at day index $L-1$ (the first day the long window is complete). Because day $L-2$ had no defined long MA, the assistant attempted to compare against an uninitialized state or falsely flagged day $L-1$ as a crossover.
   - *Correction*: Day $L-1$ is strictly designated as the *baseline state*. Day $L$ (trading day $L+1$) is enforced as the earliest possible day an MA crossover can fire. Verified by `test_alert_engine_ma_cross_incomplete_windows` and `test_alert_engine_ma_cross_fires_on_day_11`.

---

## 4. Verification & Testing Summary

- **Unit & Integration Suite**: 31 unit tests covering ISO calendar rules, header/format validation, duplicate rejection, incomplete MA windows, and multi-instrument sorting.
- **CLI Contract Suite**: 9 automated end-to-end process execution tests verifying exit codes, stderr error routing, and clean stdout.
- **Memory & Sanitizer Safety**: Tested with Clang `-fsanitize=address,undefined` (0 leaks, 0 errors).
- **Exact Brief Compliance**: Output on sample files matches the assessment brief verbatim.
