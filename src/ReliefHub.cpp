#include "ReliefHub.h"

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "AerialTransport.h"
#include "AmphibiousCraft.h"
#include "AmphibiousMedicalVehicle.h"
#include "DistributionDepot.h"
#include "Exceptions.h"
#include "GroundConvoy.h"
#include "Manipulators.h"
#include "TransportVehicle.h"
#include "Utilities.h"

namespace ReliefOps {
namespace Logistics {
namespace {

CargoLineRecord toRecord(const Inventory::ReliefCargo& cargo) {
    CargoLineRecord record;
    record.id = cargo.id();
    record.name = cargo.name();
    record.category = cargo.category();
    record.quantity = cargo.quantity();
    record.weightPerUnitKg = cargo.weightPerUnitKg();
    record.volumePerUnitM3 = cargo.volumePerUnitM3();
    record.priority = cargo.priority();
    record.destination = cargo.destination();
    return record;
}

Inventory::ReliefCargo fromRecord(const CargoLineRecord& record) {
    return Inventory::ReliefCargo(record.id, record.name, record.category,
                                  record.quantity, record.weightPerUnitKg,
                                  record.volumePerUnitM3, record.priority,
                                  record.destination);
}

Inventory::TransportManifest manifestFromRecords(
    const std::vector<CargoLineRecord>& records) {
    Inventory::TransportManifest manifest;
    for (const CargoLineRecord& record : records) {
        manifest += fromRecord(record);  // operator+= genuinely used
    }
    return manifest;
}

std::vector<CargoLineRecord> recordsFromManifest(
    const Inventory::TransportManifest& manifest) {
    std::vector<CargoLineRecord> records;
    for (const Inventory::ReliefCargo& cargo : manifest.snapshot()) {
        records.push_back(toRecord(cargo));
    }
    return records;
}

std::unique_ptr<ReliefVehicle> makeVehicleFromSpec(const VehicleSpec& spec) {
    const VehicleStatus status = spec.operational ? VehicleStatus::Available
                                                  : VehicleStatus::Refuelling;
    const GPSBeacon beacon(spec.beaconId.empty() ? "GPS-" + spec.id : spec.beaconId,
                           spec.latitude, spec.longitude, spec.beaconActive);
    const double fuelRate = spec.fuelRate > 0.0 ? spec.fuelRate : 0.0;

    switch (spec.kind) {
        case VehicleKind::Ground:
            return std::unique_ptr<ReliefVehicle>(new GroundConvoy(
                spec.id, spec.fuelCapacity, spec.currentFuel, spec.maxPayloadKg,
                status, spec.model, spec.axles,
                fuelRate > 0.0 ? fuelRate : 0.18));
        case VehicleKind::Aerial:
            return std::unique_ptr<ReliefVehicle>(new AerialTransport(
                spec.id, spec.fuelCapacity, spec.currentFuel, spec.maxPayloadKg,
                status, spec.model, spec.crewCount, spec.rotorCraft,
                fuelRate > 0.0 ? fuelRate : 0.85));
        case VehicleKind::Amphibious:
            return std::unique_ptr<ReliefVehicle>(new AmphibiousCraft(
                spec.id, spec.fuelCapacity, spec.currentFuel, spec.maxPayloadKg,
                status, spec.model,
                spec.hullCapacityTonnes > 0.0 ? spec.hullCapacityTonnes : 8.0,
                spec.waterSpeedKnots > 0.0 ? spec.waterSpeedKnots : 18.0,
                fuelRate > 0.0 ? fuelRate : 0.34));
        case VehicleKind::AmphibiousMedical: {
            EmergencyLifeSupportUnit unit(
                spec.lifeSupportId.empty() ? "ELS-" + spec.id : spec.lifeSupportId,
                spec.oxygenUnits, spec.surgicalReady);
            return std::unique_ptr<ReliefVehicle>(new AmphibiousMedicalVehicle(
                spec.id, spec.fuelCapacity, spec.currentFuel, spec.maxPayloadKg,
                status, spec.model,
                spec.hullCapacityTonnes > 0.0 ? spec.hullCapacityTonnes : 8.0,
                spec.waterSpeedKnots > 0.0 ? spec.waterSpeedKnots : 18.0,
                spec.axles, spec.triageBeds, unit));
        }
    }
    throw std::invalid_argument("Unknown vehicle kind");
}

}  // namespace

ReliefHub::ReliefHub(std::string dataFile) : fileManager_(std::move(dataFile)) {
    std::error_code error;
    const std::filesystem::path parent =
        std::filesystem::path(fileManager_.path()).parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, error);
    }
}

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------

std::string ReliefHub::addDepot(const std::string& id, const std::string& name,
                                const std::string& location) {
    if (id.empty()) {
        throw std::invalid_argument("Depot ID must not be empty");
    }
    if (depotIndex_.find(id) != depotIndex_.end()) {
        throw std::invalid_argument("Duplicate depot ID: " + id);
    }
    depots_.emplace_back(id, name, location);
    rebuildIndexes();
    appendAudit("REGISTER_DEPOT", id, "", name + " / " + location);
    return "Depot " + id + " registered at " + location;
}

std::string ReliefHub::addCargo(const std::string& depotId,
                                const std::string& cargoId,
                                const std::string& name,
                                Inventory::SupplyCategory category, int quantity,
                                double weightPerUnitKg, double volumePerUnitM3,
                                Inventory::Priority priority) {
    Inventory::DistributionDepot& depot = depotRef(depotId);
    depot.addCargo(Inventory::ReliefCargo(cargoId, name, category, quantity,
                                          weightPerUnitKg, volumePerUnitM3,
                                          priority));
    appendAudit("ADD_CARGO", depotId, "", cargoId + " x" + std::to_string(quantity));
    return "Added cargo " + cargoId + " (" + name + ") x" +
           std::to_string(quantity) + " to depot " + depotId;
}

std::string ReliefHub::restock(const std::string& depotId,
                               const std::string& cargoId, int units) {
    depotRef(depotId).restock(cargoId, units);  // calls overloaded +=
    appendAudit("RESTOCK", depotId, "", cargoId + " +" + std::to_string(units));
    return "Restocked " + cargoId + " by " + std::to_string(units) + " units";
}

std::string ReliefHub::registerVehicle(const VehicleSpec& spec) {
    if (spec.id.empty()) {
        throw std::invalid_argument("Vehicle ID must not be empty");
    }
    if (vehicleIndex_.find(spec.id) != vehicleIndex_.end()) {
        throw std::invalid_argument("Duplicate vehicle ID: " + spec.id);
    }
    std::unique_ptr<ReliefVehicle> vehicle = makeVehicleFromSpec(spec);
    vehicle->setBeacon(GPSBeacon(spec.beaconId.empty() ? "GPS-" + spec.id
                                                       : spec.beaconId,
                                 spec.latitude, spec.longitude, spec.beaconActive));
    const std::string description = vehicleKindToString(vehicle->kind()) + " " +
                                    spec.id + " (" + spec.model + ")";
    fleet_.push_back(std::move(vehicle));
    rebuildIndexes();
    appendAudit("REGISTER_VEHICLE", "", spec.id, description);
    return "Registered " + description;
}

AllocationResult ReliefHub::allocateVolume(const std::string& depotId,
                                           const std::string& vehicleId,
                                           const std::string& cargoId,
                                           double volumeM3,
                                           const std::string& destination,
                                           Inventory::Priority priority) {
    validateAllocationTargets(depotId, vehicleId);
    return allocateSupply(depotRef(depotId), vehicleRef(vehicleId), cargoId,
                          volumeM3, destination, priority, queue_, nextSequence_,
                          audits_);
}

AllocationResult ReliefHub::allocateQuantity(const std::string& depotId,
                                             const std::string& vehicleId,
                                             const std::string& cargoId,
                                             int quantity,
                                             const std::string& destination,
                                             Inventory::Priority priority,
                                             bool medicalTriage) {
    validateAllocationTargets(depotId, vehicleId);
    return allocateSupply(depotRef(depotId), vehicleRef(vehicleId), cargoId,
                          quantity, destination, priority, medicalTriage, queue_,
                          nextSequence_, audits_, &commander_);
}

DispatchPreview ReliefHub::previewNextDispatch() const {
    DispatchPreview preview;
    if (queue_.empty()) {
        preview.message = "No emergency requests are pending.";
        return preview;
    }
    const DispatchRequest& request = queue_.peek();
    const auto depotIt = depotIndex_.find(request.depotId);
    const Inventory::ReliefCargo* cargo =
        depotIt == depotIndex_.end()
            ? nullptr
            : depots_[depotIt->second].findCargo(request.cargoId);
    ReliefVehicle* vehicle = findVehicle(request.vehicleId);

    if (cargo == nullptr || cargo->quantity() < request.quantity) {
        preview.message = "Top request cannot be dispatched: depot stock changed.";
        return preview;
    }
    if (vehicle == nullptr) {
        preview.message = "Top request cannot be dispatched: vehicle missing.";
        return preview;
    }

    RouteAssessment route;
    try {
        route = vehicle->evaluateRoute(request.destination, request.corridor);
    } catch (const DamagedCorridorException& error) {
        preview.message = std::string("Route blocked: ") + error.what();
        return preview;
    }
    if (!route.feasible) {
        preview.message = "Insufficient fuel for the top request: need " +
                          std::to_string(route.fuelLitres) + " L, have " +
                          std::to_string(vehicle->currentFuel()) + " L.";
        return preview;
    }

    std::ostringstream oss;
    oss << reportTitle("TOP PRIORITY DISPATCH PREVIEW")
        << reportField("Request", request.id)
        << reportField("Assigned vehicle", request.vehicleId)
        << reportField("Depot", request.depotId)
        << reportField("Cargo", request.cargoName + " (ID " + request.cargoId + ")")
        << reportField("Quantity", std::to_string(request.quantity) + " units")
        << reportField("Destination", request.destination)
        << reportField("Priority", Inventory::priorityToString(request.priority) +
                                       (request.triageApproved ? " [triage approved]"
                                                               : ""))
        << reportField("Total payload", std::to_string(request.weightKg) + " kg")
        << reportField("Estimated fuel", std::to_string(route.fuelLitres) + " L")
        << reportField("Current fuel", std::to_string(vehicle->currentFuel()) + " L")
        << reportField("Route", route.note)
        << sectionRule;

    preview.available = true;
    preview.request = request;
    preview.details = oss.str();
    preview.message = "Dispatch is ready for confirmation.";
    return preview;
}

std::string ReliefHub::dispatchNext(bool confirmed) {
    if (queue_.empty()) {
        return "No emergency requests are pending.";
    }
    DispatchRequest request = queue_.peek();

    if (!confirmed) {
        queue_.pop();
        appendAudit("CANCEL_DISPATCH", request.depotId, request.vehicleId,
                    request.id + " cancelled by operator");
        return "Dispatch " + request.id + " cancelled. No inventory, fuel, "
               "statistics or history was changed.";
    }

    const auto depotIt = depotIndex_.find(request.depotId);
    if (depotIt == depotIndex_.end()) {
        throw std::runtime_error("Dispatch depot no longer exists");
    }
    Inventory::DistributionDepot& depot = depots_[depotIt->second];
    Inventory::ReliefCargo* cargo = depot.findCargo(request.cargoId);
    ReliefVehicle* vehicle = findVehicle(request.vehicleId);
    if (cargo == nullptr || vehicle == nullptr) {
        throw std::runtime_error("Dispatch target no longer exists");
    }
    if (cargo->quantity() < request.quantity) {
        throw std::invalid_argument("Depot stock is insufficient for " + request.id);
    }
    const RouteAssessment route =
        vehicle->evaluateRoute(request.destination, request.corridor);
    if (!route.feasible) {
        throw FuelDepletionException(route.fuelLitres, vehicle->currentFuel());
    }

    // Transaction snapshot for rollback if any later step fails.
    const int stockBefore = cargo->quantity();
    const double fuelBefore = vehicle->currentFuel();
    const VehicleStatus statusBefore = vehicle->status();
    const std::string destinationBefore = vehicle->destination();
    const std::size_t historyBefore = history_.size();
    const std::size_t auditsBefore = audits_.size();
    const double tonnesBefore = TransportVehicle::totalDispatchedTonnes();

    try {
        if (!depot.deduct(request.cargoId, request.quantity)) {
            throw std::runtime_error("Failed to deduct cargo exactly once");
        }
        vehicle->consumeFuel(route.fuelLitres);  // may throw FuelDepletionException
        vehicle->setStatus(VehicleStatus::OnMission);
        vehicle->setDestination(request.destination);
        TransportVehicle::addDispatchedTonnes(
            kilogramsToTonnes(request.weightKg));

        DispatchRecord record;
        record.id = makeUniqueId("DSP", nextSequence_++);
        record.timestamp = epochToString(nowEpochSeconds());
        record.depotId = request.depotId;
        record.vehicleId = request.vehicleId;
        record.cargoId = request.cargoId;
        record.cargoName = request.cargoName;
        record.destination = request.destination;
        record.quantity = request.quantity;
        record.weightKg = request.weightKg;
        record.fuelUsedL = route.fuelLitres;
        record.priority = request.priority;
        record.note = route.note;
        history_.push_back(record);
        appendAudit("DISPATCH", request.depotId, request.vehicleId,
                    record.id + " -> " + request.destination);
        queue_.pop();

        return "Dispatch " + record.id + " completed: " +
               std::to_string(request.quantity) + " units of " +
               request.cargoName + " delivered by " + request.vehicleId +
               " to " + request.destination + ". Fuel used: " +
               std::to_string(route.fuelLitres) + " L (no automatic refuel).";
    } catch (...) {
        // Rollback: restore every mutated component, then rethrow.
        Inventory::ReliefCargo* current = depot.findCargo(request.cargoId);
        if (current != nullptr && current->quantity() < stockBefore) {
            *current += (stockBefore - current->quantity());
        }
        // Only refuel when fuel was actually consumed: refuel(0) is invalid by
        // design, and calling it here could mask the original exception.
        const double fuelToRestore = fuelBefore - vehicle->currentFuel();
        if (fuelToRestore > 0.0) {
            vehicle->refuel(fuelToRestore);
        }
        vehicle->setStatus(statusBefore);
        vehicle->setDestination(destinationBefore);
        history_.resize(historyBefore);
        audits_.resize(auditsBefore);
        TransportVehicle::setTotalDispatchedTonnes(tonnesBefore);
        throw;
    }
}

std::string ReliefHub::refuel(const std::string& vehicleId, double litres) {
    ReliefVehicle& vehicle = vehicleRef(vehicleId);
    const std::size_t auditsBefore = audits_.size();
    // The vehicle trims the request to the remaining tank capacity; it reports
    // how much was really added so the receipt can be honest about both values.
    const double actuallyAdded = vehicle.refuel(litres);
    const double finalFuel = vehicle.currentFuel();
    appendAudit("REFUEL", "", vehicleId,
                "requested " + std::to_string(litres) + " L, added " +
                    std::to_string(actuallyAdded) + " L");

    if (!save()) {  // updated fuel level must be persisted
        vehicle.consumeFuel(actuallyAdded);
        audits_.resize(auditsBefore);
        throw std::runtime_error("Refuel could not be persisted; change rolled back");
    }
    return "Vehicle " + vehicleId + " refuel: requested " +
           std::to_string(litres) + " L, actually added " +
           std::to_string(actuallyAdded) + " L, final fuel " +
           std::to_string(finalFuel) + " L. Saved.";
}

std::string ReliefHub::emergencyRestock(const std::string& depotId,
                                        const std::string& cargoId, int units) {
    Inventory::DistributionDepot& depot = depotRef(depotId);
    commander_.emergencyRestock(depot, cargoId, units);  // friend access
    appendAudit("COMMANDER_RESTOCK", depotId, "", cargoId + " +" +
                                                     std::to_string(units));
    return "Incident commander authorised emergency restock of " + cargoId +
           " by " + std::to_string(units) + " units at " + depotId;
}

bool ReliefHub::save() {
    return fileManager_.save(snapshot());
}

bool ReliefHub::load() {
    HubSnapshot restored;
    if (!fileManager_.load(restored)) {
        return false;
    }
    restore(restored);
    return true;
}

void ReliefHub::clear() {
    depots_.clear();
    fleet_.clear();
    queue_.clear();
    history_.clear();
    audits_.clear();
    nextSequence_ = 1;
    rebuildIndexes();
    TransportVehicle::setTotalDispatchedTonnes(0.0);
}

// ---------------------------------------------------------------------------
// Reporting
// ---------------------------------------------------------------------------

void ReliefHub::displayInventory(std::ostream& os) const {
    os << reportTitle("AVAILABLE INVENTORY");
    if (depots_.empty()) {
        os << "  No depots registered.\n";
        return;
    }
    for (const Inventory::DistributionDepot& depot : depots_) {
        os << "\n  Depot " << depot.id() << " | " << depot.name() << " | "
           << depot.location() << " | lines: " << depot.cargoCount() << '\n';
        os << manifestTableHeader;
        for (const Inventory::ReliefCargo& cargo : depot.stockRef()) {
            os << "  | " << std::left << std::setw(17) << cargo.id() << " | "
               << std::setw(30) << cargo.name() << " | " << std::setw(16)
               << Inventory::categoryToString(cargo.category()) << " | "
               << std::right << std::setw(6) << cargo.quantity() << " | "
               << std::setw(5) << static_cast<int>(cargo.priority()) << " | "
               << std::fixed << std::setprecision(1) << std::setw(9)
               << cargo.totalWeightKg() << " |\n";
        }
        os << manifestTableFooter;
    }
}

void ReliefHub::displayFleet(std::ostream& os) const {
    os << reportTitle("REGISTERED TRANSPORT FLEET")
       << "  " << std::left << std::setw(20) << "Vehicle ID"
       << std::setw(26) << "Type"
       << std::setw(14) << "Status"
       << std::setw(18) << "Fuel (now/cap)"
       << std::setw(16) << "Payload cap" << "Extras\n"
       << "  ------------------------------------------------------------------------"
          "------------------------------------\n";
    std::for_each(fleet_.begin(), fleet_.end(),
                  [&os](const std::unique_ptr<ReliefVehicle>& vehicle) {
                      vehicle->printDetails(os);  // virtual dispatch
                      os << '\n';
                  });
    if (fleet_.empty()) {
        os << "  No vehicles registered.\n";
    }
}

void ReliefHub::displayQueue(std::ostream& os) const {
    os << reportTitle("EMERGENCY PRIORITY QUEUE");
    if (queue_.empty()) {
        os << "  No pending emergency requests.\n";
        return;
    }
    const std::vector<DispatchRequest> ordered = queue_.priorityOrdered();
    os << dispatchReportHeader;
    std::for_each(ordered.begin(), ordered.end(),
                  [&os](const DispatchRequest& request) {
                      os << "  " << std::left << std::setw(16) << request.id
                         << std::setw(18) << request.vehicleId
                         << std::setw(15) << request.destination
                         << std::setw(22) << request.cargoName
                         << std::setw(8) << request.quantity
                         << std::setw(12)
                         << Inventory::priorityToString(request.priority)
                         << std::setw(11) << std::fixed << std::setprecision(1)
                         << request.fuelEstimateL << '\n';
                  });
    os << dispatchReportFooter;
}

void ReliefHub::displayHistory(std::ostream& os) const {
    os << reportTitle("DISPATCH HISTORY");
    if (history_.empty()) {
        os << "  No completed dispatches recorded.\n";
        return;
    }
    os << "  " << std::left << std::setw(12) << "Record"
       << std::setw(20) << "Timestamp"
       << std::setw(16) << "Vehicle"
       << std::setw(18) << "Destination"
       << std::setw(22) << "Cargo"
       << std::setw(7) << "Qty"
       << std::setw(10) << "Fuel L" << '\n'
       << "  ------------------------------------------------------------------------"
          "----------------------------------\n";
    std::for_each(history_.begin(), history_.end(),
                  [&os](const DispatchRecord& record) {
                      os << "  " << std::left << std::setw(12) << record.id
                         << std::setw(20) << record.timestamp
                         << std::setw(16) << record.vehicleId
                         << std::setw(18) << record.destination
                         << std::setw(22) << record.cargoName
                         << std::setw(7) << record.quantity
                         << std::setw(10) << std::fixed << std::setprecision(1)
                         << record.fuelUsedL << '\n';
                  });
}

void ReliefHub::displayStatistics(std::ostream& os) const {
    os << reportTitle("LOGISTICS STATISTICS")
       << reportField("Total operational vehicles",
                      std::to_string(TransportVehicle::totalOperationalVehicles()))
       << reportField("Total dispatched tonnage",
                      std::to_string(TransportVehicle::totalDispatchedTonnes()) +
                          " tonnes")
       << reportField("Total cargo dispatched",
                      std::to_string(totalCargoDispatchedKg()) + " kg")
       << reportField("Pending emergency requests",
                      std::to_string(queue_.size()))
       << reportField("Vehicle availability",
                      std::to_string(availableVehicleCount()) + " / " +
                          std::to_string(fleet_.size()) + " available")
       << reportField("Registered depots", std::to_string(depots_.size()))
       << reportField("Completed dispatches", std::to_string(history_.size()));
}

void ReliefHub::displayAudits(std::ostream& os) const {
    os << reportTitle("MANIFEST / OPERATION AUDIT RECORDS");
    if (audits_.empty()) {
        os << "  No audit records.\n";
        return;
    }
    std::for_each(audits_.begin(), audits_.end(),
                  [&os](const AuditRecord& audit) {
                      os << "  " << audit.id << " [" << audit.operation << "] "
                         << audit.depotId << " / " << audit.vehicleId << " : "
                         << audit.detail << " (" << audit.timestamp << ")\n";
                  });
}

void ReliefHub::displayRouteEvaluation(std::ostream& os) const {
    os << reportTitle("RUNTIME POLYMORPHIC ROUTE EVALUATION")
       << "  Iterating std::vector<ReliefVehicle*> through base pointers:\n";
    const std::vector<ReliefVehicle*> view = fleetView();
    std::for_each(view.begin(), view.end(), [&os](ReliefVehicle* vehicle) {
        try {
            const RouteAssessment assessment =
                vehicle->evaluateRoute("Field Hospital Delta", CorridorStatus::Clear);
            os << "  [" << vehicleKindToString(vehicle->kind()) << "] "
               << assessment.note << " | fuel estimate "
               << std::fixed << std::setprecision(2) << assessment.fuelLitres
               << " L | feasible="
               << (assessment.feasible ? "yes" : "no") << '\n';
        } catch (const std::exception& error) {
            os << "  [" << vehicle->id() << "] " << error.what() << '\n';
        }
    });
    if (view.empty()) {
        os << "  No vehicles to evaluate.\n";
    }
}

// ---------------------------------------------------------------------------
// Accessors, snapshots and internals
// ---------------------------------------------------------------------------

std::vector<ReliefVehicle*> ReliefHub::fleetView() const {
    std::vector<ReliefVehicle*> view;
    view.reserve(fleet_.size());
    for (const std::unique_ptr<ReliefVehicle>& vehicle : fleet_) {
        view.push_back(vehicle.get());  // non-owning observers
    }
    return view;
}

std::size_t ReliefHub::depotIndex(const std::string& depotId) const {
    const auto it = depotIndex_.find(depotId);
    if (it == depotIndex_.end()) {
        throw std::out_of_range("Depot not found: " + depotId);
    }
    return it->second;
}

std::size_t ReliefHub::vehicleIndex(const std::string& vehicleId) const {
    const auto it = vehicleIndex_.find(vehicleId);
    if (it == vehicleIndex_.end()) {
        throw std::out_of_range("Vehicle not found: " + vehicleId);
    }
    return it->second;
}

ReliefVehicle* ReliefHub::findVehicle(const std::string& vehicleId) const {
    const auto it = vehicleIndex_.find(vehicleId);
    if (it == vehicleIndex_.end()) {
        return nullptr;
    }
    return fleet_[it->second].get();
}

Inventory::DistributionDepot& ReliefHub::depotRef(const std::string& depotId) {
    return depots_[depotIndex(depotId)];
}

ReliefVehicle& ReliefHub::vehicleRef(const std::string& vehicleId) {
    ReliefVehicle* vehicle = findVehicle(vehicleId);
    if (vehicle == nullptr) {
        throw std::out_of_range("Vehicle not found: " + vehicleId);
    }
    return *vehicle;
}

void ReliefHub::rebuildIndexes() {
    depotIndex_.clear();
    vehicleIndex_.clear();
    for (std::size_t i = 0; i < depots_.size(); ++i) {
        depotIndex_[depots_[i].id()] = i;
    }
    for (std::size_t i = 0; i < fleet_.size(); ++i) {
        vehicleIndex_[fleet_[i]->id()] = i;
    }
}

void ReliefHub::appendAudit(const std::string& operation,
                            const std::string& depotId,
                            const std::string& vehicleId,
                            const std::string& detail) {
    AuditRecord audit;
    audit.id = makeUniqueId("AUD", nextSequence_++);
    audit.operation = operation;
    audit.depotId = depotId;
    audit.vehicleId = vehicleId;
    audit.detail = detail;
    audit.timestamp = epochToString(nowEpochSeconds());
    audits_.push_back(audit);
}

void ReliefHub::validateAllocationTargets(const std::string& depotId,
                                          const std::string& vehicleId) const {
    if (depotIndex_.find(depotId) == depotIndex_.end()) {
        throw std::out_of_range("Depot not found: " + depotId);
    }
    if (vehicleIndex_.find(vehicleId) == vehicleIndex_.end()) {
        throw std::out_of_range("Vehicle not found: " + vehicleId);
    }
}

double ReliefHub::totalCargoDispatchedKg() const {
    double total = 0.0;
    for (const DispatchRecord& record : history_) {
        total += record.weightKg;
    }
    return total;
}

std::size_t ReliefHub::availableVehicleCount() const {
    return static_cast<std::size_t>(std::count_if(
        fleet_.begin(), fleet_.end(),
        [](const std::unique_ptr<ReliefVehicle>& vehicle) {
            return vehicle->status() == VehicleStatus::Available;
        }));
}

HubSnapshot ReliefHub::snapshot() const {
    HubSnapshot result;
    for (const Inventory::DistributionDepot& depot : depots_) {
        DepotRecord record;
        record.id = depot.id();
        record.name = depot.name();
        record.location = depot.location();
        for (const Inventory::ReliefCargo& cargo : depot.stockRef()) {
            record.stock.push_back(toRecord(cargo));
        }
        result.depots.push_back(record);
    }

    for (const std::unique_ptr<ReliefVehicle>& vehicle : fleet_) {
        VehicleRecord record;
        record.id = vehicle->id();
        record.destination = vehicle->destination();
        record.fuelCapacity = vehicle->fuelCapacity();
        record.currentFuel = vehicle->currentFuel();
        record.maxPayloadKg = vehicle->maxPayloadKg();
        record.status = vehicle->status();
        record.beaconId = vehicle->beacon().beaconId();
        record.latitude = vehicle->beacon().latitude();
        record.longitude = vehicle->beacon().longitude();
        record.beaconActive = vehicle->beacon().active();
        record.manifest = recordsFromManifest(vehicle->manifest());
        record.kind = vehicle->kind();

        if (const auto* ground = dynamic_cast<const GroundConvoy*>(vehicle.get())) {
            record.axles = ground->axles();
        }
        if (const auto* aerial = dynamic_cast<const AerialTransport*>(vehicle.get())) {
            record.crewCount = aerial->crewCount();
            record.rotorCraft = aerial->rotorCraft();
        }
        if (const auto* amphibious =
                dynamic_cast<const AmphibiousCraft*>(vehicle.get())) {
            record.hullCapacityTonnes = amphibious->hullCapacityTonnes();
            record.waterSpeedKnots = amphibious->waterSpeedKnots();
        }
        if (const auto* medical =
                dynamic_cast<const AmphibiousMedicalVehicle*>(vehicle.get())) {
            record.triageBeds = medical->triageBeds();
            record.lifeSupportId = medical->lifeSupport().unitId();
            record.oxygenUnits = medical->lifeSupport().oxygenUnits();
            record.surgicalReady = medical->lifeSupport().surgicalReady();
        }
        result.vehicles.push_back(record);
    }

    result.history = history_;
    result.pending = queue_.rawRef();
    result.audits = audits_;
    result.totalDispatchedTonnes = TransportVehicle::totalDispatchedTonnes();
    result.nextSequence = nextSequence_;
    return result;
}

void ReliefHub::restore(const HubSnapshot& snapshot) {
    clear();
    for (const DepotRecord& record : snapshot.depots) {
        Inventory::DistributionDepot depot(record.id, record.name, record.location);
        for (const CargoLineRecord& cargo : record.stock) {
            depot.addCargo(fromRecord(cargo));
        }
        depots_.push_back(depot);
    }
    for (const VehicleRecord& record : snapshot.vehicles) {
        std::unique_ptr<ReliefVehicle> vehicle = makeVehicle(record);
        vehicle->setStatus(record.status);
        vehicle->setDestination(record.destination);
        vehicle->assignManifest(manifestFromRecords(record.manifest));
        fleet_.push_back(std::move(vehicle));
    }
    rebuildIndexes();
    for (const DispatchRequest& request : snapshot.pending) {
        queue_.push(request);
    }
    history_ = snapshot.history;
    audits_ = snapshot.audits;
    nextSequence_ = snapshot.nextSequence;
    TransportVehicle::setTotalDispatchedTonnes(snapshot.totalDispatchedTonnes);
}

std::unique_ptr<ReliefVehicle> ReliefHub::makeVehicle(
    const VehicleRecord& record) const {
    VehicleSpec spec;
    spec.kind = record.kind;
    spec.id = record.id;
    spec.model = record.model;
    spec.fuelCapacity = record.fuelCapacity;
    spec.currentFuel = record.currentFuel;
    spec.maxPayloadKg = record.maxPayloadKg;
    spec.operational = record.status != VehicleStatus::Retired;
    spec.axles = record.axles;
    spec.crewCount = record.crewCount;
    spec.rotorCraft = record.rotorCraft;
    spec.hullCapacityTonnes = record.hullCapacityTonnes;
    spec.waterSpeedKnots = record.waterSpeedKnots;
    spec.triageBeds = record.triageBeds;
    spec.lifeSupportId = record.lifeSupportId;
    spec.oxygenUnits = record.oxygenUnits;
    spec.surgicalReady = record.surgicalReady;
    spec.fuelRate = record.fuelRatePerKm;
    spec.beaconId = record.beaconId;
    spec.latitude = record.latitude;
    spec.longitude = record.longitude;
    spec.beaconActive = record.beaconActive;
    return makeVehicleFromSpec(spec);
}

}  // namespace Logistics
}  // namespace ReliefOps
