#ifndef ERLHUB_EMERGENCYLIFESUPPORTUNIT_H
#define ERLHUB_EMERGENCYLIFESUPPORTUNIT_H

#include <iosfwd>
#include <string>

// UNIT III: containership — composed inside AmphibiousMedicalVehicle only.
namespace ReliefOps {
namespace Logistics {

class EmergencyLifeSupportUnit {
public:
    EmergencyLifeSupportUnit();
    EmergencyLifeSupportUnit(std::string unitId, int oxygenUnits, bool surgicalReady);

    const std::string& unitId() const { return unitId_; }
    int oxygenUnits() const { return oxygenUnits_; }
    bool surgicalReady() const { return surgicalReady_; }

    void addOxygen(int units);
    bool consumeOxygen(int units);
    void setSurgicalReady(bool ready) { surgicalReady_ = ready; }

    // Demonstrates a meaningful member function of the contained object.
    std::string provideCare() const;

    friend std::ostream& operator<<(std::ostream& os,
                                    const EmergencyLifeSupportUnit& unit);

private:
    std::string unitId_;
    int oxygenUnits_;
    bool surgicalReady_;
};

}  // namespace Logistics
}  // namespace ReliefOps

#endif  // ERLHUB_EMERGENCYLIFESUPPORTUNIT_H
