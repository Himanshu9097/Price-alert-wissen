#pragma once

#include <string_view>

namespace wissen {

/**
 * Checks if a string conforms strictly to ISO-8601 YYYY-MM-DD date format
 * and represents a valid calendar date, accounting for month lengths and leap years.
 */
bool is_valid_iso_date(std::string_view date_str);

/**
 * Returns true if the given year is a leap year according to Gregorian calendar rules.
 */
bool is_leap_year(int year);

/**
 * Returns the number of days in the specified month of the specified year.
 * Returns 0 if year or month is invalid.
 */
int days_in_month(int year, int month);

} // namespace wissen
