#include "output_formatter.hpp"

namespace wissen {

std::string OutputFormatter::format_alert(const Alert& alert) {
    return alert.date + " " + alert.rule_id + " " + alert.instrument + " " + alert.details;
}

void OutputFormatter::print_alerts(const std::vector<Alert>& alerts, std::ostream& out) {
    for (const auto& alert : alerts) {
        out << format_alert(alert) << "\n";
    }
}

} // namespace wissen
