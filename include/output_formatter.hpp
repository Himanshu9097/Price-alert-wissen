#pragma once

#include "models.hpp"
#include <vector>
#include <ostream>
#include <string>

namespace wissen {

class OutputFormatter {
public:
    /**
     * Formats a single alert into the brief's standard string representation:
     * "<date> <rule_id> <instrument> <details>"
     */
    static std::string format_alert(const Alert& alert);

    /**
     * Writes all alerts to the specified output stream, one per line.
     * If alerts is empty, writes nothing.
     */
    static void print_alerts(const std::vector<Alert>& alerts, std::ostream& out);
};

} // namespace wissen
