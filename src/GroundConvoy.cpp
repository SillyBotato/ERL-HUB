#include "GroundConvoy.h"

#include <ostream>
#include <utility>

namespace ReliefOps {
namespace Logistics {

std::size_t GroundConvoy::destroyedCount_ = 0;

GroundConvoy::GroundConvoy(std::string id, double fuelCapacity,
                           double currentFuel, double maxPayloadKg,
                           VehicleStatus status, std::string model, int axles,
                           double fuelRatePerKm)
    // Virtual bases are explicitly initialised here: this class can be the
    // most-derived object (and also acts as a diamond branch).
    : ReliefVehicle(id, fuelCapacity, currentFuel, maxPayloadKg, status),
      TransportVehicle(id, fuelCapacity, currentFuel, maxPayloadKg, status,
                       std::move(model)),
      axles_(axles), fuelRatePerKm_(fuelRatePerKm) {
    if (axles <= 0 || fuelRatePerKm < 0.0) {
        throw std::invalid_argument("Invalid ground convoy specification");
    }
}

GroundConvoy::~GroundConvoy() {
    ++destroyedCount_;
}

RouteAssessment GroundConvoy::evaluateRoute(const std::string& destination,
                                            CorridorStatus corridor) const {
    RouteAssessment assessment = assessRoute(destination, corridor);
    assessment.note += " [surface roads, " + std::to_string(axles_) + " axles]";
    return assessment;
}

std::unique_ptr<ReliefVehicle> GroundConvoy::clone() const {
    return std::unique_ptr<ReliefVehicle>(new GroundConvoy(*this));
}

void GroundConvoy::printDetails(std::ostream& os) const {
    ReliefVehicle::printDetails(os);
    os << " axles=" << axles_;
}

std::size_t GroundConvoy::destroyedCount() {
    return destroyedCount_;
}

}  // namespace Logistics
}  // namespace ReliefOps
