#include "ReliefCargo.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "Utilities.h"

namespace ReliefOps {
namespace Inventory {

ReliefCargo::ReliefCargo()
    : id_("UNKNOWN"), name_("Unspecified"), category_(SupplyCategory::Rations),
      quantity_(0), weightPerUnitKg_(0.0), volumePerUnitM3_(0.0),
      priority_(Priority::Routine), destination_() {}

ReliefCargo::ReliefCargo(std::string id, std::string name, SupplyCategory category,
                         int quantity, double weightPerUnitKg,
                         double volumePerUnitM3, Priority priority,
                         std::string destination)
    : id_(std::move(id)), name_(std::move(name)), category_(category),
      quantity_(quantity), weightPerUnitKg_(weightPerUnitKg),
      volumePerUnitM3_(volumePerUnitM3), priority_(priority),
      destination_(std::move(destination)) {
    if (quantity < 0 || weightPerUnitKg < 0.0 || volumePerUnitM3 < 0.0) {
        throw std::invalid_argument("Cargo quantity/weight/volume must be non-negative");
    }
}

ReliefCargo& ReliefCargo::operator+=(int units) {
    if (units <= 0) {
        throw std::invalid_argument("Restock quantity must be positive");
    }
    quantity_ += units;
    return *this;
}

ReliefCargo& ReliefCargo::operator-=(int units) {
    if (units <= 0) {
        throw std::invalid_argument("Deduction quantity must be positive");
    }
    if (units > quantity_) {
        throw std::invalid_argument("Cannot deduct more cargo than is available");
    }
    quantity_ -= units;
    return *this;
}

bool ReliefCargo::operator==(const ReliefCargo& other) const {
    return id_ == other.id_;
}

double ReliefCargo::totalWeightKg() const {
    return Logistics::payloadKg(quantity_, weightPerUnitKg_);
}

double ReliefCargo::totalVolumeM3() const {
    return static_cast<double>(quantity_) * volumePerUnitM3_;
}

std::ostream& operator<<(std::ostream& os, const ReliefCargo& cargo) {
    os << cargo.id_ << ' ' << cargo.name_ << ' '
       << static_cast<int>(cargo.category_) << ' ' << cargo.quantity_ << ' '
       << cargo.weightPerUnitKg_ << ' ' << cargo.volumePerUnitM3_ << ' '
       << static_cast<int>(cargo.priority_) << ' ' << cargo.destination_;
    return os;
}

std::istream& operator>>(std::istream& is, ReliefCargo& cargo) {
    std::string id;
    std::string name;
    int category = 0;
    int quantity = 0;
    double weight = 0.0;
    double volume = 0.0;
    int priority = 0;
    std::string destination;

    if (!(is >> id >> name >> category >> quantity >> weight >> volume >> priority)) {
        is.setstate(std::ios::failbit);
        return is;
    }
    // Destination is the remainder of the current line (it may contain spaces
    // and may be empty). Do NOT use `>> std::ws` here: it would skip the newline
    // and consume the next manifest line's cargo ID.
    std::string remainder;
    std::getline(is, remainder);
    if (!is && remainder.empty()) {
        return is;
    }
    if (!remainder.empty() && remainder.front() == ' ') {
        remainder.erase(remainder.begin());
    }
    destination = remainder;

    ReliefCargo parsed(id, name, categoryFromInt(category), quantity, weight,
                       volume, priorityFromInt(priority), destination);
    cargo = parsed;
    return is;
}

// ---------------------------- TransportManifest ----------------------------

TransportManifest::TransportManifest()
    : lines_(nullptr), count_(0), capacity_(0) {}

TransportManifest::TransportManifest(const TransportManifest& other)
    : lines_(nullptr), count_(0), capacity_(0) {
    if (other.count_ > 0) {
        ensureCapacity(other.count_);
        for (std::size_t i = 0; i < other.count_; ++i) {
            lines_[count_++] = new ReliefCargo(*other.lines_[i]);  // deep copy
        }
    }
}

TransportManifest& TransportManifest::operator=(const TransportManifest& other) {
    if (this != &other) {
        TransportManifest copy(other);  // copy-and-swap: strong exception safety
        std::swap(lines_, copy.lines_);
        std::swap(count_, copy.count_);
        std::swap(capacity_, copy.capacity_);
    }
    return *this;
}

TransportManifest::TransportManifest(TransportManifest&& other) noexcept
    : lines_(other.lines_), count_(other.count_), capacity_(other.capacity_) {
    other.lines_ = nullptr;
    other.count_ = 0;
    other.capacity_ = 0;
}

TransportManifest& TransportManifest::operator=(TransportManifest&& other) noexcept {
    if (this != &other) {
        releaseAll();
        lines_ = other.lines_;
        count_ = other.count_;
        capacity_ = other.capacity_;
        other.lines_ = nullptr;
        other.count_ = 0;
        other.capacity_ = 0;
    }
    return *this;
}

TransportManifest::~TransportManifest() {
    releaseAll();
}

void TransportManifest::releaseAll() noexcept {
    for (std::size_t i = 0; i < count_; ++i) {
        delete lines_[i];
        lines_[i] = nullptr;
    }
    delete[] lines_;
    lines_ = nullptr;
    count_ = 0;
    capacity_ = 0;
}

void TransportManifest::ensureCapacity(std::size_t required) {
    if (required <= capacity_) {
        return;
    }
    std::size_t newCapacity = capacity_ == 0 ? 2 : capacity_ * 2;
    while (newCapacity < required) {
        newCapacity *= 2;
    }
    ReliefCargo** expanded = new ReliefCargo*[newCapacity]();
    for (std::size_t i = 0; i < count_; ++i) {
        expanded[i] = lines_[i];
    }
    delete[] lines_;
    lines_ = expanded;
    capacity_ = newCapacity;
}

std::size_t TransportManifest::findIndex(const std::string& cargoId) const {
    for (std::size_t i = 0; i < count_; ++i) {
        if (lines_[i]->id() == cargoId) {
            return i;
        }
    }
    return count_;
}

TransportManifest& TransportManifest::operator+=(const ReliefCargo& line) {
    const std::size_t index = findIndex(line.id());
    if (index < count_) {
        *lines_[index] += line.quantity();  // restock existing line
        return *this;
    }
    ensureCapacity(count_ + 1);
    lines_[count_++] = new ReliefCargo(line);  // new line, deep-copied
    return *this;
}

TransportManifest& TransportManifest::operator-=(const ReliefCargo& line) {
    const std::size_t index = findIndex(line.id());
    if (index == count_) {
        throw std::invalid_argument("Cargo line not present in manifest: " + line.id());
    }
    if (line.quantity() > lines_[index]->quantity()) {
        throw std::invalid_argument("Insufficient manifest quantity for " + line.id());
    }
    *lines_[index] -= line.quantity();
    if (lines_[index]->quantity() == 0) {
        delete lines_[index];
        for (std::size_t i = index; i + 1 < count_; ++i) {
            lines_[i] = lines_[i + 1];
        }
        --count_;
        lines_[count_] = nullptr;
    }
    return *this;
}

bool TransportManifest::operator==(const TransportManifest& other) const {
    if (count_ != other.count_) {
        return false;
    }
    std::vector<ReliefCargo> left = snapshot();
    std::vector<ReliefCargo> right = other.snapshot();
    const auto byId = [](const ReliefCargo& a, const ReliefCargo& b) {
        return a.id() < b.id();
    };
    std::sort(left.begin(), left.end(), byId);
    std::sort(right.begin(), right.end(), byId);
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (left[i].id() != right[i].id() || left[i].name() != right[i].name() ||
            left[i].category() != right[i].category() ||
            left[i].quantity() != right[i].quantity() ||
            !Logistics::nearlyEqual(left[i].weightPerUnitKg(),
                                    right[i].weightPerUnitKg()) ||
            !Logistics::nearlyEqual(left[i].volumePerUnitM3(),
                                    right[i].volumePerUnitM3()) ||
            left[i].priority() != right[i].priority() ||
            left[i].destination() != right[i].destination()) {
            return false;
        }
    }
    return true;
}

void TransportManifest::clear() noexcept {
    releaseAll();
}

const ReliefCargo& TransportManifest::at(std::size_t index) const {
    if (index >= count_) {
        throw std::out_of_range("Manifest index out of range");
    }
    return *lines_[index];
}

ReliefCargo& TransportManifest::at(std::size_t index) {
    if (index >= count_) {
        throw std::out_of_range("Manifest index out of range");
    }
    return *lines_[index];
}

const ReliefCargo* TransportManifest::find(const std::string& cargoId) const {
    const std::size_t index = findIndex(cargoId);
    return index == count_ ? nullptr : lines_[index];
}

ReliefCargo* TransportManifest::find(const std::string& cargoId) {
    const std::size_t index = findIndex(cargoId);
    return index == count_ ? nullptr : lines_[index];
}

double TransportManifest::totalWeightKg() const {
    double total = 0.0;
    for (std::size_t i = 0; i < count_; ++i) {
        total += lines_[i]->totalWeightKg();
    }
    return total;
}

double TransportManifest::totalVolumeM3() const {
    double total = 0.0;
    for (std::size_t i = 0; i < count_; ++i) {
        total += lines_[i]->totalVolumeM3();
    }
    return total;
}

std::vector<ReliefCargo> TransportManifest::snapshot() const {
    std::vector<ReliefCargo> result;
    result.reserve(count_);
    for (std::size_t i = 0; i < count_; ++i) {
        result.push_back(*lines_[i]);
    }
    return result;
}

std::ostream& operator<<(std::ostream& os, const TransportManifest& manifest) {
    os << manifest.count_ << '\n';
    for (std::size_t i = 0; i < manifest.count_; ++i) {
        os << *manifest.lines_[i] << '\n';
    }
    return os;
}

std::istream& operator>>(std::istream& is, TransportManifest& manifest) {
    long long count = 0;
    if (!(is >> count) || count < 0 || count > 100000) {
        is.setstate(std::ios::failbit);
        return is;
    }
    TransportManifest parsed;
    for (long long i = 0; i < count; ++i) {
        ReliefCargo cargo;
        if (!(is >> cargo)) {
            is.setstate(std::ios::failbit);
            return is;
        }
        parsed += cargo;
    }
    manifest = parsed;  // move assignment
    return is;
}

}  // namespace Inventory
}  // namespace ReliefOps
