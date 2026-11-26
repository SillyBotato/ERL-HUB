#ifndef ERLHUB_MANIPULATORS_H
#define ERLHUB_MANIPULATORS_H

#include <iomanip>
#include <ostream>
#include <string>

// UNIT IV: user-defined stream manipulators for tabular/report formatting.
namespace ReliefOps {
namespace Logistics {

// Stateless manipulators: use as  os << manifestTableHeader << ...
inline std::ostream& manifestTableHeader(std::ostream& os) {
    os << "\n  +-------------------+--------------------------------+------------------"
          "+--------+-------+-----------+\n"
       << "  | " << std::left << std::setw(17) << "Cargo ID"
       << " | " << std::setw(30) << "Cargo name"
       << " | " << std::setw(16) << "Category"
       << " | " << std::setw(6) << "Qty"
       << " | " << std::setw(5) << "Prio"
       << " | " << std::setw(9) << "Weight kg" << " |\n"
       << "  +-------------------+--------------------------------+------------------"
          "+--------+-------+-----------+\n";
    return os;
}

inline std::ostream& manifestTableFooter(std::ostream& os) {
    os << "  +-------------------+--------------------------------+------------------"
          "+--------+-------+-----------+\n";
    return os;
}

inline std::ostream& dispatchReportHeader(std::ostream& os) {
    os << "\n  ==================== DISPATCH REPORT ====================\n"
       << "  " << std::left << std::setw(16) << "Request"
       << std::setw(18) << "Vehicle"
       << std::setw(15) << "Destination"
       << std::setw(22) << "Cargo"
       << std::setw(8) << "Qty"
       << std::setw(12) << "Priority"
       << std::setw(11) << "Fuel L\n"
       << "  ------------------------------------------------------------"
          "------------------------------\n";
    return os;
}

inline std::ostream& dispatchReportFooter(std::ostream& os) {
    os << "  ------------------------------------------------------------"
          "------------------------------\n";
    return os;
}

inline std::ostream& sectionRule(std::ostream& os) {
    os << "\n  ------------------------------------------------------------\n";
    return os;
}

// Parameterised manipulator: use as  os << reportTitle("My title")
class reportTitle {
public:
    explicit reportTitle(std::string title) : title_(std::move(title)) {}

    friend std::ostream& operator<<(std::ostream& os, const reportTitle& manip) {
        os << "\n  >>> " << manip.title_ << " <<<\n"
           << "  ============================================================\n";
        return os;
    }

private:
    std::string title_;
};

// Parameterised manipulator for aligned key/value lines in previews.
class reportField {
public:
    reportField(std::string label, std::string value)
        : label_(std::move(label)), value_(std::move(value)) {}

    friend std::ostream& operator<<(std::ostream& os, const reportField& manip) {
        os << "  " << std::left << std::setw(28) << manip.label_ << ": "
           << manip.value_ << '\n';
        return os;
    }

private:
    std::string label_;
    std::string value_;
};

}  // namespace Logistics
}  // namespace ReliefOps

#endif  // ERLHUB_MANIPULATORS_H
