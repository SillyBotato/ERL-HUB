#ifndef ERLHUB_DISTRIBUTIONDEPOT_H
#define ERLHUB_DISTRIBUTIONDEPOT_H

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

#include "ReliefCargo.h"

namespace ReliefOps {
namespace Logistics {
class IncidentCommander;  // forward declaration for friendship
}

namespace Inventory {

class DistributionDepot {
public:
    DistributionDepot();
    DistributionDepot(std::string id, std::string name, std::string location);

    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }
    const std::string& location() const { return location_; }

    void addCargo(const ReliefCargo& cargo);
    void restock(const std::string& cargoId, int units);   // uses ReliefCargo::operator+=
    bool deduct(const std::string& cargoId, int units);    // uses ReliefCargo::operator-=
    const ReliefCargo* findCargo(const std::string& cargoId) const;
    ReliefCargo* findCargo(const std::string& cargoId);

    // UNIT I: return by reference (live objects, not copies).
    ReliefCargo& cargoRef(const std::string& cargoId);
    const ReliefCargo& cargoRef(const std::string& cargoId) const;
    const std::vector<ReliefCargo>& stockRef() const { return stock_; }
    std::vector<ReliefCargo> stockSnapshot() const { return stock_; }

    std::size_t cargoCount() const { return stock_.size(); }
    bool empty() const { return stock_.empty(); }

    friend std::ostream& operator<<(std::ostream& os, const DistributionDepot& depot);

private:
    std::string id_;
    std::string name_;
    std::string location_;
    std::vector<ReliefCargo> stock_;

    friend class ReliefOps::Logistics::IncidentCommander;
};

}  // namespace Inventory
}  // namespace ReliefOps

#endif  // ERLHUB_DISTRIBUTIONDEPOT_H
