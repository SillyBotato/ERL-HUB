#include "DepotRegistry.h"

#include <algorithm>
#include <ostream>
#include <sstream>

namespace ReliefOps {
namespace Demos {

bool DepotRegistry::registerDepot(Inventory::DistributionDepot& depot) {
    if (find(depot.id()) != nullptr) {
        return false;
    }
    depots_.push_back(&depot);  // non-owning reference
    return true;
}

bool DepotRegistry::unregister(const std::string& depotId) {
    const auto it = std::find_if(depots_.begin(), depots_.end(),
                                 [&depotId](const Inventory::DistributionDepot* d) {
                                     return d != nullptr && d->id() == depotId;
                                 });
    if (it == depots_.end()) {
        return false;
    }
    depots_.erase(it);
    return true;
}

Inventory::DistributionDepot* DepotRegistry::find(const std::string& depotId) {
    const auto it = std::find_if(depots_.begin(), depots_.end(),
                                 [&depotId](const Inventory::DistributionDepot* d) {
                                     return d != nullptr && d->id() == depotId;
                                 });
    return it == depots_.end() ? nullptr : *it;
}

const Inventory::DistributionDepot* DepotRegistry::find(
    const std::string& depotId) const {
    const auto it = std::find_if(depots_.begin(), depots_.end(),
                                 [&depotId](const Inventory::DistributionDepot* d) {
                                     return d != nullptr && d->id() == depotId;
                                 });
    return it == depots_.end() ? nullptr : *it;
}

std::vector<std::string> DepotRegistry::registeredIds() const {
    std::vector<std::string> ids;
    ids.reserve(depots_.size());
    for (const Inventory::DistributionDepot* depot : depots_) {
        if (depot != nullptr) {
            ids.push_back(depot->id());
        }
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

std::string DepotRegistry::report() const {
    std::ostringstream oss;
    oss << "Aggregation demo: registry holds " << depots_.size()
        << " non-owning depot reference(s); the depots remain owned by their "
           "creator.\n";
    for (const Inventory::DistributionDepot* depot : depots_) {
        if (depot != nullptr) {
            oss << "  " << depot->id() << " -> " << depot->location() << " ("
                << depot->cargoCount() << " cargo lines)\n";
        }
    }
    return oss.str();
}

std::ostream& operator<<(std::ostream& os, const DepotRegistry& registry) {
    return os << registry.report();
}

std::string aggregationExplanation() {
    return "Composition: the whole owns its parts (DistributionDepot owns "
           "ReliefCargo by value; ReliefVehicle owns GPSBeacon by value). "
           "Aggregation: the whole only refers to independently owned parts "
           "(DepotRegistry keeps non-owning pointers to DistributionDepot).";
}

}  // namespace Demos
}  // namespace ReliefOps
