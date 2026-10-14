CXX ?= clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O3 -Iinclude -Ithird_party -I.

CORE_SRCS = src/date_utils.cpp \
            src/csv_parser.cpp \
            src/json_parser.cpp \
            src/alert_engine.cpp \
            src/output_formatter.cpp

MAIN_SRCS = src/main.cpp $(CORE_SRCS)

TEST_SRCS = tests/test_main.cpp \
            tests/test_date_utils.cpp \
            tests/test_csv_parser.cpp \
            tests/test_json_parser.cpp \
            tests/test_alert_engine.cpp \
            tests/test_integration.cpp \
            $(CORE_SRCS)

all: price_alert run_tests

price_alert: $(MAIN_SRCS)
	$(CXX) $(CXXFLAGS) $(MAIN_SRCS) -o price_alert

run_tests: $(TEST_SRCS)
	$(CXX) $(CXXFLAGS) -Itests $(TEST_SRCS) -o run_tests

test: run_tests price_alert
	./run_tests
	./tests/test_cli.sh

clean:
	rm -rf price_alert run_tests build/

.PHONY: all test clean
