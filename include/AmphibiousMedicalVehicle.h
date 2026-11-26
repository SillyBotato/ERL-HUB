#ifndef ERLHUB_AMPHIBIOUSMEDICALVEHICLE_H
#define ERLHUB_AMPHIBIOUSMEDICALVEHICLE_H

#include <cstddef>
#include <iosfwd>
#include <memory>
#include <string>

#include "AmphibiousCraft.h"
#include "EmergencyLifeSupportUnit.h"
#include "GroundConvoy.h"

// UNIT III: hybrid inheritance / diamond problem resolution.
namespace ReliefOps {
namespace Logistics {

class AmphibiousMedicalVehicle : public AmphibiousCraft, public GroundConvoy {
public:
    AmphibiousMedicalVehicle(std::string id, double fuelCapacity, double currentFuel,
                             double maxPayloadKg, VehicleStatus status,
                             std::string model, double hullCapacityTonnes,
                             double waterSpeedKnots, int axles, int triageBeds,
                             EmergencyLifeSupportUnit lifeSupport =
                                 EmergencyLifeSupportUnit());
    ~AmphibiousMedicalVehicle() override;

    VehicleKind kind() const override { return VehicleKind::AmphibiousMedical; }
    double fuelRatePerKm() const override;
    RouteAssessment evaluateRoute(const std::string& destination,
                                  CorridorStatus corridor) const override;
    std::unique_ptr<ReliefVehicle> clone() const override;
    void printDetails(std::ostream& os) const override;

    int triageBeds() const { return triageBeds_; }
    const EmergencyLifeSupportUnit& lifeSupport() const { return lifeSupport_; }
    EmergencyLifeSupportUnit& lifeSupportRef() { return lifeSupport_; }
    static std::size_t destroyedCount();

private:
    EmergencyLifeSupportUnit lifeSupport_;  // containership
    int triageBeds_;
    static std::size_t destroyedCount_;
};

}  // namespace Logistics
}  // namespace ReliefOps

#endif  // ERLHUB_AMPHIBIOUSMEDICALVEHICLE_H
