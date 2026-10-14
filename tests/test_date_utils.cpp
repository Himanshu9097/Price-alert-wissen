#include "test_harness.hpp"
#include "date_utils.hpp"

using namespace wissen;

TEST_CASE(test_date_utils_valid_dates) {
    ASSERT_TRUE(is_valid_iso_date("2026-09-01"));
    ASSERT_TRUE(is_valid_iso_date("2026-12-31"));
    ASSERT_TRUE(is_valid_iso_date("2024-02-29")); // Leap year
    ASSERT_TRUE(is_valid_iso_date("2000-02-29")); // Century leap year
    ASSERT_TRUE(is_valid_iso_date("2023-01-01"));
    ASSERT_TRUE(is_valid_iso_date("2026-04-30")); // 30-day month
}

TEST_CASE(test_date_utils_invalid_dates) {
    ASSERT_FALSE(is_valid_iso_date("2023-02-29")); // Not a leap year
    ASSERT_FALSE(is_valid_iso_date("1900-02-29")); // 1900 not leap year
    ASSERT_FALSE(is_valid_iso_date("2026-04-31")); // April has 30 days
    ASSERT_FALSE(is_valid_iso_date("2026-09-31")); // September has 30 days
    ASSERT_FALSE(is_valid_iso_date("2026-02-30"));
    ASSERT_FALSE(is_valid_iso_date("2026-00-15")); // Month 0
    ASSERT_FALSE(is_valid_iso_date("2026-13-01")); // Month 13
    ASSERT_FALSE(is_valid_iso_date("2026-09-00")); // Day 0
    ASSERT_FALSE(is_valid_iso_date("2026-09-32")); // Day 32
    ASSERT_FALSE(is_valid_iso_date("2026/09/01")); // Wrong delimiter
    ASSERT_FALSE(is_valid_iso_date("2026-9-1"));   // Missing leading zeros
    ASSERT_FALSE(is_valid_iso_date("2026-09-01a"));// Trailing char
    ASSERT_FALSE(is_valid_iso_date(""));           // Empty
    ASSERT_FALSE(is_valid_iso_date("abcd-ef-gh")); // Non-digits
    ASSERT_FALSE(is_valid_iso_date("1850-01-01")); // Year out of range (< 1900)
    ASSERT_FALSE(is_valid_iso_date("2150-01-01")); // Year out of range (> 2100)
}
