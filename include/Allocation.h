#ifndef ERLHUB_ALLOCATION_H
#define ERLHUB_ALLOCATION_H

#include <cstdint>
#include <string>
#include <vector>

#include "PriorityDispatchQueue.h"
#include "Records.h"
#include "Types.h"

namespace ReliefOps {
namespace Inventory {
class DistributionDepot;
}
namespace Logistics {

class ReliefVehicle;
class IncidentCommander;

struct AllocationResult {
    bool accepted = false;
    DispatchRequest request;
    std::string message;
};

// UNIT I: overloaded allocateSupply() — volume-based allocation.
// Quantity is derived from the cargo's per-unit volume; inventory is NOT changed
// until a confirmed dispatch (so cancellation has no side effects).
AllocationResult allocateSupply(Inventory::DistributionDepot& depot,
                                ReliefVehicle& vehicle,
                                const std::string& cargoId,
                                double volumeM3,
                                const std::string& destination,
                                Inventory::Priority priority,
                                PriorityDispatchQueue<DispatchRequest>& queue,
                                std::uint64_t& nextSequence,
                                std::vector<AuditRecord>& audits);

// UNIT I: overloaded allocateSupply() — emergency medical priority allocation.
// Explicit quantity, plus a triage flag that can be authorised by the commander.
AllocationResult allocateSupply(Inventory::DistributionDepot& depot,
                                ReliefVehicle& vehicle,
                                const std::string& cargoId,
                                int quantity,
                                const std::string& destination,
                                Inventory::Priority priority,
                                bool medicalTriage,
                                PriorityDispatchQueue<DispatchRequest>& queue,
                                std::uint64_t& nextSequence,
                                std::vector<AuditRecord>& audits,
                                IncidentCommander* commander = nullptr);

// UNIT I: call by reference — mission parameters are mutated in place.
void configureMissionParameters(DispatchRequest& request,
                                const std::string& destination,
                                Inventory::Priority priority,
                                double weightKg,
                                double volumeM3,
                                CorridorStatus corridor);

}  // namespace Logistics
}  // namespace ReliefOps

#endif  // ERLHUB_ALLOCATION_H
