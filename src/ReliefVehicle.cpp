#include "ReliefVehicle.h"

#include <iomanip>
#include <ostream>
#include <sstream>
#include <utility>

namespace ReliefOps {
namespace Logistics {

std::size_t ReliefVehicle::destructorCalls_ = 0;

ReliefVehicle::ReliefVehicle(std::string id, double fuelCapacity,
                             double currentFuel, double maxPayloadKg,
                             VehicleStatus status, GPSBeacon beacon)
    : id_(std::move(id)), fuelCapacity_(fuelCapacity),
      currentFuel_(currentFuel), maxPayloadKg_(maxPayloadKg), status_(status),
      destination_(), corridorOverride_(false), beacon_(std::move(beacon)) {
    if (id_.empty() || fuelCapacity < 0.0 || currentFuel < 0.0 ||
        maxPayloadKg < 0.0) {
        throw std::invalid_argument("Invalid vehicle construction parameters");
    }
    if (currentFuel > fuelCapacity) {
        throw std::invalid_argument("Initial fuel exceeds fuel capacity");
    }
}

ReliefVehicle::~ReliefVehicle() {
    ++destructorCalls_;
}

void ReliefVehicle::consumeFuel(double litres) {
    if (litres < 0.0) {
        throw std::invalid_argument("Fuel consumption cannot be negative");
    }
    if (litres > currentFuel_) {
        throw FuelDepletionException(litres, currentFuel_);
    }
    currentFuel_ -= litres;
}

double ReliefVehicle::refuel(double litres) {
    if (litres <= 0.0) {
        throw std::invalid_argument("Refuel quantity must be positive");
    }
    // Automatic trimming: only the free capacity can be added. This is a
    // deliberate change from the earlier overfill rejection policy.
    const double freeSpace = fuelCapacity_ - currentFuel_;
    const double added = clampValue(litres, 0.0, freeSpace);
    currentFuel_ += added;
    return added;
}

void ReliefVehicle::assignManifest(const Inventory::TransportManifest& manifest) {
    manifest_ = manifest;  // deep copy through manifest copy assignment
}

RouteAssessment ReliefVehicle::assessRoute(const std::string& destination,
                                           CorridorStatus corridor) const {
    if (destination.empty()) {
        throw std::invalid_argument("Route destination must not be empty");
    }
    if (corridor == CorridorStatus::Damaged && !corridorOverride_) {
        throw DamagedCorridorException(destination);
    }

    RouteAssessment assessment;
    assessment.corridor = corridor;
    assessment.distanceKm = Logistics::distanceKm(id_, destination) +
                            (corridor == CorridorStatus::Congested ? 18.0 : 0.0);
    assessment.fuelLitres = assessment.distanceKm * fuelRatePerKm();
    assessment.feasible = true;

    std::ostringstream note;
    note << "Route " << id_ << " -> " << destination << " via "
         << corridorToString(corridor) << " corridor, "
         << std::fixed << std::setprecision(1) << assessment.distanceKm << " km";
    if (corridorOverride_) {
        note << " [command override active]";
    }
    if (assessment.fuelLitres > currentFuel_) {
        assessment.feasible = false;
        note << "; INSUFFICIENT FUEL (need " << std::setprecision(2)
             << assessment.fuelLitres << " L, have " << currentFuel_ << " L)";
    }
    assessment.note = note.str();
    return assessment;
}

void ReliefVehicle::printDetails(std::ostream& os) const {
    os << std::left << std::setw(20) << id_
       << std::setw(26) << vehicleKindToString(kind())
       << std::setw(14) << vehicleStatusToString(status_) << std::setw(12)
       << std::fixed << std::setprecision(1) << currentFuel_ << "/"
       << std::setprecision(1) << fuelCapacity_ << " L"
       << std::setw(14) << maxPayloadKg_ << " kg";
}

std::size_t ReliefVehicle::destructorCalls() {
    return destructorCalls_;
}

void ReliefVehicle::resetDestructorTrace() {
    destructorCalls_ = 0;
}

}  // namespace Logistics
}  // namespace ReliefOps
