#ifndef ERLHUB_DEPOTREGISTRY_H
#define ERLHUB_DEPOTREGISTRY_H

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

#include "DistributionDepot.h"

// Supporting example for CIC-211 Unit III: AGGREGATION.
//
// A DepotRegistry refers to DistributionDepot objects that are owned and
// lifetime-controlled by somebody else (in the main application: ReliefHub).
// It stores non-owning pointers, never new/delete, so a depot can exist before,
// after, and independently of the registry. The caller must keep a registered
// depot alive for as long as it remains registered; unregister()/clear() remove
// references before the objects go away.
namespace ReliefOps {
namespace Demos {

class DepotRegistry {
public:
    DepotRegistry() = default;
    DepotRegistry(const DepotRegistry&) = default;
    DepotRegistry& operator=(const DepotRegistry&) = default;
    ~DepotRegistry() = default;  // no delete: aggregation, not ownership

    // Registers an existing depot by non-owning pointer. Returns false when the
    // ID is already registered (duplicate IDs are rejected).
    bool registerDepot(Inventory::DistributionDepot& depot);

    // Removes a reference; never destroys the depot. Returns true if found.
    bool unregister(const std::string& depotId);

    Inventory::DistributionDepot* find(const std::string& depotId);
    const Inventory::DistributionDepot* find(const std::string& depotId) const;

    std::size_t size() const noexcept { return depots_.size(); }
    bool empty() const noexcept { return depots_.empty(); }
    void clear() noexcept { depots_.clear(); }

    // Sorted snapshot of registered IDs (uses iterators + std::sort).
    std::vector<std::string> registeredIds() const;

    std::string report() const;

    friend std::ostream& operator<<(std::ostream& os, const DepotRegistry& registry);

private:
    // AGGREGATION: plain observing pointers. Contrast with ReliefHub's
    // vector<unique_ptr<ReliefVehicle>> (composition/ownership) and with
    // DistributionDepot's vector<ReliefCargo> (composition by value).
    std::vector<Inventory::DistributionDepot*> depots_;
};

// Short explanation used by the test and viva notes.
std::string aggregationExplanation();

}  // namespace Demos
}  // namespace ReliefOps

#endif  // ERLHUB_DEPOTREGISTRY_H
