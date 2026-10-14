#include "date_utils.hpp"

#include <cctype>

namespace wissen {

bool is_leap_year(int year) {
    if (year % 4 != 0) return false;
    if (year % 100 != 0) return true;
    return (year % 400 == 0);
}

int days_in_month(int year, int month) {
    if (month < 1 || month > 12) return 0;
    switch (month) {
        case 1:  return 31;
        case 2:  return is_leap_year(year) ? 29 : 28;
        case 3:  return 31;
        case 4:  return 30;
        case 5:  return 31;
        case 6:  return 30;
        case 7:  return 31;
        case 8:  return 31;
        case 9:  return 30;
        case 10: return 31;
        case 11: return 30;
        case 12: return 31;
        default: return 0;
    }
}

bool is_valid_iso_date(std::string_view date_str) {
    if (date_str.size() != 10) {
        return false;
    }

    if (date_str[4] != '-' || date_str[7] != '-') {
        return false;
    }

    for (size_t i = 0; i < 10; ++i) {
        if (i == 4 || i == 7) continue;
        if (!std::isdigit(static_cast<unsigned char>(date_str[i]))) {
            return false;
        }
    }

    int year = (date_str[0] - '0') * 1000 +
               (date_str[1] - '0') * 100 +
               (date_str[2] - '0') * 10 +
               (date_str[3] - '0');

    int month = (date_str[5] - '0') * 10 +
                (date_str[6] - '0');

    int day = (date_str[8] - '0') * 10 +
              (date_str[9] - '0');

    if (year < 1900 || year > 2100) {
        return false;
    }

    if (month < 1 || month > 12) {
        return false;
    }

    int max_days = days_in_month(year, month);
    if (day < 1 || day > max_days) {
        return false;
    }

    return true;
}

} // namespace wissen
