#ifndef ERLHUB_AMPHIBIOUSCRAFT_H
#define ERLHUB_AMPHIBIOUSCRAFT_H

#include <cstddef>
#include <iosfwd>
#include <memory>
#include <string>

#include "TransportVehicle.h"

namespace ReliefOps {
namespace Logistics {

class AmphibiousCraft : virtual public TransportVehicle {
public:
    AmphibiousCraft(std::string id, double fuelCapacity, double currentFuel,
                    double maxPayloadKg, VehicleStatus status, std::string model,
                    double hullCapacityTonnes = 8.0, double waterSpeedKnots = 18.0,
                    double fuelRatePerKm = 0.34);
    ~AmphibiousCraft() override;

    VehicleKind kind() const override { return VehicleKind::Amphibious; }
    double fuelRatePerKm() const override { return fuelRatePerKm_; }
    RouteAssessment evaluateRoute(const std::string& destination,
                                  CorridorStatus corridor) const override;
    std::unique_ptr<ReliefVehicle> clone() const override;
    void printDetails(std::ostream& os) const override;

    double hullCapacityTonnes() const { return hullCapacityTonnes_; }
    double waterSpeedKnots() const { return waterSpeedKnots_; }
    static std::size_t destroyedCount();

protected:
    double hullCapacityTonnes_;
    double waterSpeedKnots_;
    double fuelRatePerKm_;

private:
    static std::size_t destroyedCount_;
};

}  // namespace Logistics
}  // namespace ReliefOps

#endif  // ERLHUB_AMPHIBIOUSCRAFT_H
