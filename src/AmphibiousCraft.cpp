#include "AmphibiousCraft.h"

#include <ostream>
#include <utility>

namespace ReliefOps {
namespace Logistics {

std::size_t AmphibiousCraft::destroyedCount_ = 0;

AmphibiousCraft::AmphibiousCraft(std::string id, double fuelCapacity,
                                 double currentFuel, double maxPayloadKg,
                                 VehicleStatus status, std::string model,
                                 double hullCapacityTonnes,
                                 double waterSpeedKnots, double fuelRatePerKm)
    : ReliefVehicle(id, fuelCapacity, currentFuel, maxPayloadKg, status),
      TransportVehicle(id, fuelCapacity, currentFuel, maxPayloadKg, status,
                       std::move(model)),
      hullCapacityTonnes_(hullCapacityTonnes),
      waterSpeedKnots_(waterSpeedKnots), fuelRatePerKm_(fuelRatePerKm) {
    if (hullCapacityTonnes <= 0.0 || waterSpeedKnots <= 0.0 ||
        fuelRatePerKm < 0.0) {
        throw std::invalid_argument("Invalid amphibious craft specification");
    }
}

AmphibiousCraft::~AmphibiousCraft() {
    ++destroyedCount_;
}

RouteAssessment AmphibiousCraft::evaluateRoute(const std::string& destination,
                                               CorridorStatus corridor) const {
    RouteAssessment assessment = assessRoute(destination, corridor);
    assessment.note += " [flooded roads / water approach]";
    return assessment;
}

std::unique_ptr<ReliefVehicle> AmphibiousCraft::clone() const {
    return std::unique_ptr<ReliefVehicle>(new AmphibiousCraft(*this));
}

void AmphibiousCraft::printDetails(std::ostream& os) const {
    ReliefVehicle::printDetails(os);
    os << " hull=" << hullCapacityTonnes_ << "t speed=" << waterSpeedKnots_
       << "kn";
}

std::size_t AmphibiousCraft::destroyedCount() {
    return destroyedCount_;
}

}  // namespace Logistics
}  // namespace ReliefOps
