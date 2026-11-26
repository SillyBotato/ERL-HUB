#include "Allocation.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

#include "DistributionDepot.h"
#include "Exceptions.h"
#include "IncidentCommander.h"
#include "ReliefVehicle.h"
#include "Utilities.h"

namespace ReliefOps {
namespace Logistics {

void configureMissionParameters(DispatchRequest& request,
                                const std::string& destination,
                                Inventory::Priority priority, double weightKg,
                                double volumeM3, CorridorStatus corridor) {
    // Call by reference: the caller's request object is modified in place.
    if (destination.empty()) {
        throw std::invalid_argument("Mission destination must not be empty");
    }
    request.destination = destination;
    request.priority = priority;
    request.weightKg = weightKg;
    request.volumeM3 = volumeM3;
    request.corridor = corridor;
    request.createdAt = epochToString(nowEpochSeconds());
}

namespace {

void validateAndFill(Inventory::DistributionDepot& depot, ReliefVehicle& vehicle,
                     const std::string& cargoId, int units,
                     DispatchRequest& request) {
    Inventory::ReliefCargo* cargo = depot.findCargo(cargoId);
    if (cargo == nullptr) {
        throw std::invalid_argument("Cargo " + cargoId + " not found in depot " +
                                    depot.id());
    }
    if (units <= 0) {
        throw std::invalid_argument("Allocation quantity must be positive");
    }
    if (units > cargo->quantity()) {
        throw std::invalid_argument("Depot has only " +
                                    std::to_string(cargo->quantity()) +
                                    " units of " + cargoId);
    }

    const double requestedKg = payloadKg(units, cargo->weightPerUnitKg());
    const double alreadyLoadedKg = vehicle.manifestRef().totalWeightKg();
    const double availableKg = vehicle.maxPayloadKg() - alreadyLoadedKg;
    if (requestedKg > availableKg) {
        throw CargoOverweightException(requestedKg, availableKg);
    }

    request.depotId = depot.id();
    request.vehicleId = vehicle.id();
    request.cargoId = cargo->id();
    request.cargoName = cargo->name();
    request.quantity = units;
    request.weightKg = requestedKg;
    request.volumeM3 = static_cast<double>(units) * cargo->volumePerUnitM3();
}

void finalise(DispatchRequest& request,
              PriorityDispatchQueue<DispatchRequest>& queue,
              std::uint64_t& nextSequence, std::vector<AuditRecord>& audits,
              const std::string& detail) {
    request.id = makeUniqueId("REQ", nextSequence++);
    request.sequence = request.id.empty() ? 0 : nextSequence - 1;
    queue.push(request);  // queue stores a copy; pending state only

    AuditRecord audit;
    audit.id = makeUniqueId("AUD", nextSequence++);
    audit.operation = "ALLOCATE";
    audit.depotId = request.depotId;
    audit.vehicleId = request.vehicleId;
    audit.detail = detail;
    audit.timestamp = epochToString(nowEpochSeconds());
    audits.push_back(audit);
}

}  // namespace

AllocationResult allocateSupply(Inventory::DistributionDepot& depot,
                                ReliefVehicle& vehicle,
                                const std::string& cargoId, double volumeM3,
                                const std::string& destination,
                                Inventory::Priority priority,
                                PriorityDispatchQueue<DispatchRequest>& queue,
                                std::uint64_t& nextSequence,
                                std::vector<AuditRecord>& audits) {
    AllocationResult result;
    Inventory::ReliefCargo* cargo = depot.findCargo(cargoId);
    if (cargo == nullptr) {
        throw std::invalid_argument("Cargo " + cargoId + " not found in depot " +
                                    depot.id());
    }
    if (volumeM3 <= 0.0 || cargo->volumePerUnitM3() <= 0.0) {
        throw std::invalid_argument("Volume allocation requires a positive volume");
    }

    const int units = static_cast<int>(
        std::floor(volumeM3 / cargo->volumePerUnitM3() + 1e-9));
    if (units < 1) {
        throw std::invalid_argument("Requested volume is smaller than one cargo unit");
    }

    DispatchRequest request;
    validateAndFill(depot, vehicle, cargoId, units, request);
    configureMissionParameters(request, destination, priority, request.weightKg,
                               request.volumeM3, CorridorStatus::Clear);

    const RouteAssessment route = vehicle.evaluateRoute(destination, CorridorStatus::Clear);
    request.fuelEstimateL = route.fuelLitres;

    std::ostringstream detail;
    detail << "Volume allocation " << volumeM3 << " m3 -> " << units
           << " units of " << cargoId << " for " << destination;
    finalise(request, queue, nextSequence, audits, detail.str());

    result.accepted = true;
    result.request = request;
    result.message = "Queued " + std::to_string(units) + " units (" +
                     std::to_string(request.weightKg) + " kg) of " +
                     cargo->name() + " for " + destination +
                     ". Priority: " + Inventory::priorityToString(priority);
    return result;
}

AllocationResult allocateSupply(Inventory::DistributionDepot& depot,
                                ReliefVehicle& vehicle,
                                const std::string& cargoId, int quantity,
                                const std::string& destination,
                                Inventory::Priority priority, bool medicalTriage,
                                PriorityDispatchQueue<DispatchRequest>& queue,
                                std::uint64_t& nextSequence,
                                std::vector<AuditRecord>& audits,
                                IncidentCommander* commander) {
    AllocationResult result;

    DispatchRequest request;
    validateAndFill(depot, vehicle, cargoId, quantity, request);

    const Inventory::Priority effectivePriority =
        medicalTriage ? Inventory::Priority::MedicalEmergency : priority;
    configureMissionParameters(request, destination, effectivePriority,
                               request.weightKg, request.volumeM3,
                               CorridorStatus::Clear);

    if (medicalTriage) {
        if (commander != nullptr) {
            commander->applyTriage(request);  // friend access to private fields
        } else {
            request.triageApproved = true;
        }
    }

    const RouteAssessment route = vehicle.evaluateRoute(destination, CorridorStatus::Clear);
    request.fuelEstimateL = route.fuelLitres;

    std::ostringstream detail;
    detail << "Emergency allocation " << quantity << " units of " << cargoId
           << " for " << destination << ", priority "
           << Inventory::priorityToString(effectivePriority);
    finalise(request, queue, nextSequence, audits, detail.str());

    result.accepted = true;
    result.request = request;
    result.message = "Queued " + std::to_string(quantity) + " units (" +
                     std::to_string(request.weightKg) + " kg) of " +
                     request.cargoName + " for " + destination +
                     ". Priority: " +
                     Inventory::priorityToString(effectivePriority);
    return result;
}

}  // namespace Logistics
}  // namespace ReliefOps
