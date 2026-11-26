#ifndef ERLHUB_RECORDS_H
#define ERLHUB_RECORDS_H

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

#include "Types.h"

namespace ReliefOps {
namespace Logistics {

class IncidentCommander;

// Plain data record for a pending allocation. Friendship enables triage override.
struct DispatchRequest {
    std::string id;
    std::string depotId;
    std::string vehicleId;
    std::string cargoId;
    std::string cargoName;
    std::string destination;
    int quantity = 0;
    double weightKg = 0.0;
    double volumeM3 = 0.0;
    double fuelEstimateL = 0.0;
    Inventory::Priority priority = Inventory::Priority::Routine;
    CorridorStatus corridor = CorridorStatus::Clear;
    bool triageApproved = false;
    std::uint64_t sequence = 0;
    std::string createdAt;

    friend class IncidentCommander;
};

struct DispatchRecord {
    std::string id;
    std::string timestamp;
    std::string depotId;
    std::string vehicleId;
    std::string cargoId;
    std::string cargoName;
    std::string destination;
    int quantity = 0;
    double weightKg = 0.0;
    double fuelUsedL = 0.0;
    Inventory::Priority priority = Inventory::Priority::Routine;
    std::string note;
};

struct AuditRecord {
    std::string id;
    std::string operation;
    std::string depotId;
    std::string vehicleId;
    std::string detail;
    std::string timestamp;
};

struct CargoLineRecord {
    std::string id;
    std::string name;
    Inventory::SupplyCategory category = Inventory::SupplyCategory::Rations;
    int quantity = 0;
    double weightPerUnitKg = 0.0;
    double volumePerUnitM3 = 0.0;
    Inventory::Priority priority = Inventory::Priority::Routine;
    std::string destination;
};

struct VehicleRecord {
    VehicleKind kind = VehicleKind::Ground;
    std::string id;
    std::string model;
    std::string destination;
    double fuelCapacity = 0.0;
    double currentFuel = 0.0;
    double maxPayloadKg = 0.0;
    VehicleStatus status = VehicleStatus::Available;
    std::string beaconId;
    double latitude = 0.0;
    double longitude = 0.0;
    bool beaconActive = true;
    // Type-specific fields; unused fields keep documented defaults.
    int axles = 4;
    int crewCount = 3;
    bool rotorCraft = true;
    double hullCapacityTonnes = 0.0;
    double waterSpeedKnots = 0.0;
    int triageBeds = 0;
    std::string lifeSupportId;
    int oxygenUnits = 0;
    bool surgicalReady = false;
    double fuelRatePerKm = 0.0;
    std::vector<CargoLineRecord> manifest;
};

struct DepotRecord {
    std::string id;
    std::string name;
    std::string location;
    std::vector<CargoLineRecord> stock;
};

// Everything needed to persist / restore the hub, without any object pointers.
struct HubSnapshot {
    std::vector<DepotRecord> depots;
    std::vector<VehicleRecord> vehicles;
    std::vector<DispatchRecord> history;
    std::vector<DispatchRequest> pending;
    std::vector<AuditRecord> audits;
    double totalDispatchedTonnes = 0.0;
    std::uint64_t nextSequence = 1;
};

std::uint64_t nowEpochSeconds();
std::string epochToString(std::uint64_t epochSeconds);
std::string makeUniqueId(const std::string& prefix, std::uint64_t sequence);

std::ostream& operator<<(std::ostream& os, const DispatchRequest& request);
std::ostream& operator<<(std::ostream& os, const DispatchRecord& record);

}  // namespace Logistics
}  // namespace ReliefOps

#endif  // ERLHUB_RECORDS_H
