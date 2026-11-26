#include "DistributionDepot.h"

#include <iomanip>
#include <iostream>
#include <stdexcept>

#include "Manipulators.h"

namespace ReliefOps {
namespace Inventory {

DistributionDepot::DistributionDepot()
    : id_("DEPOT-0"), name_("Unnamed depot"), location_("Unknown") {}

DistributionDepot::DistributionDepot(std::string id, std::string name,
                                     std::string location)
    : id_(std::move(id)), name_(std::move(name)), location_(std::move(location)) {
    if (id_.empty() || name_.empty()) {
        throw std::invalid_argument("Depot ID and name must not be empty");
    }
}

void DistributionDepot::addCargo(const ReliefCargo& cargo) {
    if (findCargo(cargo.id()) != nullptr) {
        throw std::invalid_argument("Duplicate cargo ID in depot: " + cargo.id());
    }
    stock_.push_back(cargo);  // operator== based duplicate detection above
}

void DistributionDepot::restock(const std::string& cargoId, int units) {
    if (units <= 0) {
        throw std::invalid_argument("Restock quantity must be positive");
    }
    ReliefCargo& target = cargoRef(cargoId);  // return by reference, then mutate
    target += units;                          // UNIT II: overloaded += used here
}

bool DistributionDepot::deduct(const std::string& cargoId, int units) {
    ReliefCargo* target = findCargo(cargoId);
    if (target == nullptr || units <= 0 || units > target->quantity()) {
        return false;
    }
    *target -= units;                         // UNIT II: overloaded -= used here
    return true;
}

const ReliefCargo* DistributionDepot::findCargo(const std::string& cargoId) const {
    for (const ReliefCargo& cargo : stock_) {
        if (cargo.id() == cargoId) {
            return &cargo;
        }
    }
    return nullptr;
}

ReliefCargo* DistributionDepot::findCargo(const std::string& cargoId) {
    for (ReliefCargo& cargo : stock_) {
        if (cargo.id() == cargoId) {
            return &cargo;
        }
    }
    return nullptr;
}

ReliefCargo& DistributionDepot::cargoRef(const std::string& cargoId) {
    ReliefCargo* cargo = findCargo(cargoId);
    if (cargo == nullptr) {
        throw std::out_of_range("Cargo not found in depot: " + cargoId);
    }
    return *cargo;
}

const ReliefCargo& DistributionDepot::cargoRef(const std::string& cargoId) const {
    const ReliefCargo* cargo = findCargo(cargoId);
    if (cargo == nullptr) {
        throw std::out_of_range("Cargo not found in depot: " + cargoId);
    }
    return *cargo;
}

std::ostream& operator<<(std::ostream& os, const DistributionDepot& depot) {
    os << "Depot " << depot.id_ << " - " << depot.name_
       << " (" << depot.location_ << "), cargo lines: " << depot.stock_.size();
    return os;
}

}  // namespace Inventory
}  // namespace ReliefOps
