#include "AmphibiousMedicalVehicle.h"

#include <ostream>
#include <utility>

namespace ReliefOps {
namespace Logistics {

std::size_t AmphibiousMedicalVehicle::destroyedCount_ = 0;

AmphibiousMedicalVehicle::AmphibiousMedicalVehicle(
    std::string id, double fuelCapacity, double currentFuel,
    double maxPayloadKg, VehicleStatus status, std::string model,
    double hullCapacityTonnes, double waterSpeedKnots, int axles,
    int triageBeds, EmergencyLifeSupportUnit lifeSupport)
    // The most-derived class must initialise BOTH virtual bases exactly once.
    : ReliefVehicle(id, fuelCapacity, currentFuel, maxPayloadKg, status),
      TransportVehicle(id, fuelCapacity, currentFuel, maxPayloadKg, status,
                       model),
      AmphibiousCraft(id, fuelCapacity, currentFuel, maxPayloadKg, status,
                      model, hullCapacityTonnes, waterSpeedKnots, 0.30),
      GroundConvoy(id, fuelCapacity, currentFuel, maxPayloadKg, status,
                   model, axles, 0.16),
      lifeSupport_(std::move(lifeSupport)), triageBeds_(triageBeds) {
    if (triageBeds <= 0) {
        throw std::invalid_argument("A medical vehicle needs at least one triage bed");
    }
}

AmphibiousMedicalVehicle::~AmphibiousMedicalVehicle() {
    ++destroyedCount_;
}

double AmphibiousMedicalVehicle::fuelRatePerKm() const {
    // Single effective rate; avoids the two shadowed rate fields of the branches.
    return (AmphibiousCraft::fuelRatePerKm() + GroundConvoy::fuelRatePerKm()) / 2.0;
}

RouteAssessment AmphibiousMedicalVehicle::evaluateRoute(
    const std::string& destination, CorridorStatus corridor) const {
    RouteAssessment assessment = assessRoute(destination, corridor);
    assessment.note += " [medical convoy, " + std::to_string(triageBeds_) +
                       " triage beds, " + lifeSupport_.provideCare() + "]";
    return assessment;
}

std::unique_ptr<ReliefVehicle> AmphibiousMedicalVehicle::clone() const {
    return std::unique_ptr<ReliefVehicle>(new AmphibiousMedicalVehicle(*this));
}

void AmphibiousMedicalVehicle::printDetails(std::ostream& os) const {
    ReliefVehicle::printDetails(os);
    os << " beds=" << triageBeds_ << " hull=" << hullCapacityTonnes()
       << "t ELSU=" << lifeSupport_.unitId();
}

std::size_t AmphibiousMedicalVehicle::destroyedCount() {
    return destroyedCount_;
}

}  // namespace Logistics
}  // namespace ReliefOps
