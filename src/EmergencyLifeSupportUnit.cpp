#include "EmergencyLifeSupportUnit.h"

#include <ostream>
#include <stdexcept>

namespace ReliefOps {
namespace Logistics {

EmergencyLifeSupportUnit::EmergencyLifeSupportUnit()
    : unitId_("ELS-000"), oxygenUnits_(0), surgicalReady_(false) {}

EmergencyLifeSupportUnit::EmergencyLifeSupportUnit(std::string unitId,
                                                   int oxygenUnits,
                                                   bool surgicalReady)
    : unitId_(std::move(unitId)), oxygenUnits_(oxygenUnits),
      surgicalReady_(surgicalReady) {
    if (oxygenUnits < 0) {
        throw std::invalid_argument("Oxygen units must be non-negative");
    }
}

void EmergencyLifeSupportUnit::addOxygen(int units) {
    if (units <= 0) {
        throw std::invalid_argument("Oxygen top-up must be positive");
    }
    oxygenUnits_ += units;
}

bool EmergencyLifeSupportUnit::consumeOxygen(int units) {
    if (units <= 0 || units > oxygenUnits_) {
        return false;
    }
    oxygenUnits_ -= units;
    return true;
}

std::string EmergencyLifeSupportUnit::provideCare() const {
    if (oxygenUnits_ <= 0) {
        return "No oxygen reserve available";
    }
    return surgicalReady_ ? "Emergency surgery and oxygen support available"
                          : "Oxygen support available (no surgical team)";
}

std::ostream& operator<<(std::ostream& os, const EmergencyLifeSupportUnit& unit) {
    os << unit.unitId_ << " (O2=" << unit.oxygenUnits_ << " units, "
       << (unit.surgicalReady_ ? "surgical ready" : "basic care") << ")";
    return os;
}

}  // namespace Logistics
}  // namespace ReliefOps
