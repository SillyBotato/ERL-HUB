#include "TransportVehicle.h"

#include <utility>

namespace ReliefOps {
namespace Logistics {

std::size_t TransportVehicle::operationalVehicles_ = 0;
double TransportVehicle::totalDispatchedTonnes_ = 0.0;

TransportVehicle::TransportVehicle(std::string id, double fuelCapacity,
                                   double currentFuel, double maxPayloadKg,
                                   VehicleStatus status, std::string model)
    : ReliefVehicle(id, fuelCapacity, currentFuel, maxPayloadKg, status),
      model_(std::move(model)) {
    ++operationalVehicles_;
}

TransportVehicle::TransportVehicle(const TransportVehicle& other)
    : ReliefVehicle(other), model_(other.model_) {
    ++operationalVehicles_;
}

TransportVehicle& TransportVehicle::operator=(const TransportVehicle& other) {
    if (this != &other) {
        // The virtual base is assigned by the most-derived class.
        model_ = other.model_;
    }
    return *this;
}

TransportVehicle::~TransportVehicle() {
    --operationalVehicles_;
}

std::size_t TransportVehicle::totalOperationalVehicles() {
    return operationalVehicles_;
}

double TransportVehicle::totalDispatchedTonnes() {
    return totalDispatchedTonnes_;
}

void TransportVehicle::addDispatchedTonnes(double tonnes) {
    if (tonnes < 0.0) {
        return;
    }
    totalDispatchedTonnes_ += tonnes;
}

void TransportVehicle::setTotalDispatchedTonnes(double tonnes) {
    totalDispatchedTonnes_ = tonnes < 0.0 ? 0.0 : tonnes;
}

}  // namespace Logistics
}  // namespace ReliefOps
