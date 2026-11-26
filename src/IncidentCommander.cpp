#include "IncidentCommander.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "DistributionDepot.h"
#include "ReliefVehicle.h"

namespace ReliefOps {
namespace Logistics {

IncidentCommander::IncidentCommander(std::string name) : name_(std::move(name)) {}

void IncidentCommander::authorizeEmergencyRefuel(ReliefVehicle& vehicle) const {
    // Friendship grants access to the otherwise private fuel tank.
    vehicle.currentFuel_ = vehicle.fuelCapacity_;
    vehicle.setStatus(VehicleStatus::Available);
}

void IncidentCommander::authorizeDamagedCorridor(ReliefVehicle& vehicle,
                                                 bool allow) const {
    vehicle.corridorOverride_ = allow;  // private member of ReliefVehicle
}

void IncidentCommander::applyTriage(DispatchRequest& request) const {
    // Private members of DispatchRequest are reachable through friendship.
    request.priority = Inventory::Priority::MedicalEmergency;
    request.triageApproved = true;
}

void IncidentCommander::emergencyRestock(Inventory::DistributionDepot& depot,
                                         const std::string& cargoId,
                                         int units) const {
    if (units <= 0) {
        throw std::invalid_argument("Emergency restock quantity must be positive");
    }
    for (Inventory::ReliefCargo& cargo : depot.stock_) {  // private vector
        if (cargo.id() == cargoId) {
            cargo += units;
            return;
        }
    }
    throw std::invalid_argument("Cargo not found for emergency restock: " + cargoId);
}

std::string IncidentCommander::inspectRestricted(
    const ReliefVehicle& vehicle) const {
    std::ostringstream oss;
    oss << "Commander " << name_ << " inspecting " << vehicle.id_
        << ": fuel=" << std::fixed << std::setprecision(2) << vehicle.currentFuel_
        << "/" << vehicle.fuelCapacity_ << " L, corridorOverride="
        << (vehicle.corridorOverride_ ? "ON" : "OFF");
    return oss.str();
}

}  // namespace Logistics
}  // namespace ReliefOps
