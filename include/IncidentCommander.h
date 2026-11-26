#ifndef ERLHUB_INCIDENTCOMMANDER_H
#define ERLHUB_INCIDENTCOMMANDER_H

#include <string>

#include "Records.h"
#include "Types.h"

namespace ReliefOps {
namespace Inventory {
class DistributionDepot;
}

namespace Logistics {

class ReliefVehicle;

// UNIT I: friend class with privileged access to critical logistics objects.
class IncidentCommander {
public:
    explicit IncidentCommander(std::string name = "Duty Incident Commander");

    const std::string& name() const { return name_; }

    // Privileged emergency refuel: touches ReliefVehicle's private currentFuel_.
    void authorizeEmergencyRefuel(ReliefVehicle& vehicle) const;

    // Privileged corridor override: touches ReliefVehicle's private corridorOverride_.
    void authorizeDamagedCorridor(ReliefVehicle& vehicle, bool allow) const;

    // Privileged triage: touches DispatchRequest's private priority/triage fields.
    void applyTriage(DispatchRequest& request) const;

    // Privileged stock override: touches DistributionDepot's private stock_ vector.
    void emergencyRestock(Inventory::DistributionDepot& depot,
                          const std::string& cargoId, int units) const;

    // Read-only inspection of restricted fields (demonstrates const friendship).
    std::string inspectRestricted(const ReliefVehicle& vehicle) const;

private:
    std::string name_;
};

}  // namespace Logistics
}  // namespace ReliefOps

#endif  // ERLHUB_INCIDENTCOMMANDER_H
