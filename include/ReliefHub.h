#ifndef ERLHUB_RELIEFHUB_H
#define ERLHUB_RELIEFHUB_H

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Allocation.h"
#include "EmergencyLifeSupportUnit.h"
#include "FileManager.h"
#include "IncidentCommander.h"
#include "DistributionDepot.h"
#include "PriorityDispatchQueue.h"
#include "Records.h"
#include "ReliefVehicle.h"
#include "Types.h"

namespace ReliefOps {
namespace Logistics {

// Value object collecting all parameters needed to build any vehicle kind.
struct VehicleSpec {
    VehicleKind kind = VehicleKind::Ground;
    std::string id;
    std::string model;
    double fuelCapacity = 0.0;
    double currentFuel = 0.0;
    double maxPayloadKg = 0.0;
    bool operational = true;
    int axles = 4;
    int crewCount = 3;
    bool rotorCraft = true;
    double hullCapacityTonnes = 0.0;
    double waterSpeedKnots = 0.0;
    double fuelRate = 0.0;
    int triageBeds = 0;
    std::string lifeSupportId;
    int oxygenUnits = 0;
    bool surgicalReady = false;
    std::string beaconId;
    double latitude = 0.0;
    double longitude = 0.0;
    bool beaconActive = true;
};

struct DispatchPreview {
    bool available = false;
    std::string message;
    DispatchRequest request;
    std::string details;
};

// Application service. Owns all domain objects; the UI only talks to this class.
class ReliefHub {
public:
    explicit ReliefHub(std::string dataFile = "data/erlhub.dat");

    // -- mutating commands (return a human-readable receipt/error) ----------
    std::string addDepot(const std::string& id, const std::string& name,
                         const std::string& location);
    std::string addCargo(const std::string& depotId, const std::string& cargoId,
                         const std::string& name, Inventory::SupplyCategory category,
                         int quantity, double weightPerUnitKg,
                         double volumePerUnitM3, Inventory::Priority priority);
    std::string restock(const std::string& depotId, const std::string& cargoId,
                        int units);
    std::string registerVehicle(const VehicleSpec& spec);
    AllocationResult allocateVolume(const std::string& depotId,
                                    const std::string& vehicleId,
                                    const std::string& cargoId, double volumeM3,
                                    const std::string& destination,
                                    Inventory::Priority priority);
    AllocationResult allocateQuantity(const std::string& depotId,
                                      const std::string& vehicleId,
                                      const std::string& cargoId, int quantity,
                                      const std::string& destination,
                                      Inventory::Priority priority,
                                      bool medicalTriage);
    DispatchPreview previewNextDispatch() const;
    std::string dispatchNext(bool confirmed);
    std::string refuel(const std::string& vehicleId, double litres);
    std::string emergencyRestock(const std::string& depotId,
                                 const std::string& cargoId, int units);
    bool save();
    bool load();
    void clear();

    // -- reporting ----------------------------------------------------------
    void displayInventory(std::ostream& os) const;
    void displayFleet(std::ostream& os) const;
    void displayQueue(std::ostream& os) const;
    void displayHistory(std::ostream& os) const;
    void displayStatistics(std::ostream& os) const;
    void displayAudits(std::ostream& os) const;
    void displayRouteEvaluation(std::ostream& os) const;

    // -- reference accessors (UNIT I) ---------------------------------------
    Inventory::DistributionDepot& depotRef(const std::string& depotId);
    ReliefVehicle& vehicleRef(const std::string& vehicleId);
    const std::vector<DispatchRecord>& historyRef() const { return history_; }
    const PriorityDispatchQueue<DispatchRequest>& queueRef() const { return queue_; }
    IncidentCommander& commanderRef() { return commander_; }

    // Non-owning view used for std::vector<ReliefVehicle*> polymorphism demos.
    std::vector<ReliefVehicle*> fleetView() const;

    // -- persistence / state snapshots --------------------------------------
    HubSnapshot snapshot() const;
    void restore(const HubSnapshot& snapshot);

    std::size_t depotCount() const { return depots_.size(); }
    std::size_t vehicleCount() const { return fleet_.size(); }
    std::size_t pendingCount() const { return queue_.size(); }
    std::size_t historyCount() const { return history_.size(); }
    double totalCargoDispatchedKg() const;
    std::size_t availableVehicleCount() const;

private:
    std::size_t depotIndex(const std::string& depotId) const;
    std::size_t vehicleIndex(const std::string& vehicleId) const;
    ReliefVehicle* findVehicle(const std::string& vehicleId) const;
    void rebuildIndexes();
    void appendAudit(const std::string& operation, const std::string& depotId,
                     const std::string& vehicleId, const std::string& detail);
    std::unique_ptr<ReliefVehicle> makeVehicle(const VehicleRecord& record) const;
    void validateAllocationTargets(const std::string& depotId,
                                   const std::string& vehicleId) const;

    std::vector<Inventory::DistributionDepot> depots_;
    std::vector<std::unique_ptr<ReliefVehicle>> fleet_;
    std::unordered_map<std::string, std::size_t> depotIndex_;
    std::unordered_map<std::string, std::size_t> vehicleIndex_;
    PriorityDispatchQueue<DispatchRequest> queue_;
    std::vector<DispatchRecord> history_;
    std::vector<AuditRecord> audits_;
    std::uint64_t nextSequence_ = 1;
    FileManager fileManager_;
    IncidentCommander commander_;
};

}  // namespace Logistics
}  // namespace ReliefOps

#endif  // ERLHUB_RELIEFHUB_H
