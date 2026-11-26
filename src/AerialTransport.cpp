#include "AerialTransport.h"

#include <ostream>
#include <utility>

namespace ReliefOps {
namespace Logistics {

std::size_t AerialTransport::destroyedCount_ = 0;

AerialTransport::AerialTransport(std::string id, double fuelCapacity,
                                 double currentFuel, double maxPayloadKg,
                                 VehicleStatus status, std::string model,
                                 int crewCount, bool rotorCraft,
                                 double fuelRatePerKm)
    : ReliefVehicle(id, fuelCapacity, currentFuel, maxPayloadKg, status),
      TransportVehicle(id, fuelCapacity, currentFuel, maxPayloadKg, status,
                       std::move(model)),
      crewCount_(crewCount), rotorCraft_(rotorCraft),
      fuelRatePerKm_(fuelRatePerKm) {
    if (crewCount <= 0 || fuelRatePerKm < 0.0) {
        throw std::invalid_argument("Invalid aerial transport specification");
    }
}

AerialTransport::~AerialTransport() {
    ++destroyedCount_;
}

RouteAssessment AerialTransport::evaluateRoute(const std::string& destination,
                                               CorridorStatus corridor) const {
    RouteAssessment assessment = assessRoute(destination, corridor);
    assessment.note += rotorCraft_ ? " [rotorcraft air corridor]"
                                   : " [fixed-wing air corridor]";
    return assessment;
}

std::unique_ptr<ReliefVehicle> AerialTransport::clone() const {
    return std::unique_ptr<ReliefVehicle>(new AerialTransport(*this));
}

void AerialTransport::printDetails(std::ostream& os) const {
    ReliefVehicle::printDetails(os);
    os << " crew=" << crewCount_ << (rotorCraft_ ? " rotor" : " fixed-wing");
}

std::size_t AerialTransport::destroyedCount() {
    return destroyedCount_;
}

}  // namespace Logistics
}  // namespace ReliefOps
