#ifndef ERLHUB_RELIEFCARGO_H
#define ERLHUB_RELIEFCARGO_H

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

#include "Types.h"

namespace ReliefOps {
namespace Inventory {

// A single cargo line: private data, public const-correct interface (UNIT II).
class ReliefCargo {
public:
    ReliefCargo();
    ReliefCargo(std::string id, std::string name, SupplyCategory category,
                int quantity, double weightPerUnitKg, double volumePerUnitM3,
                Priority priority, std::string destination = std::string());

    // Restock / deduct a cargo line. Both are genuinely used by the application.
    ReliefCargo& operator+=(int units);
    ReliefCargo& operator-=(int units);

    // Duplicate detection is based on the unique cargo ID.
    bool operator==(const ReliefCargo& other) const;
    bool operator!=(const ReliefCargo& other) const { return !(*this == other); }

    // Textual display and read-back (used in manifest audit tests).
    friend std::ostream& operator<<(std::ostream& os, const ReliefCargo& cargo);
    friend std::istream& operator>>(std::istream& is, ReliefCargo& cargo);

    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }
    SupplyCategory category() const { return category_; }
    int quantity() const { return quantity_; }
    double weightPerUnitKg() const { return weightPerUnitKg_; }
    double volumePerUnitM3() const { return volumePerUnitM3_; }
    Priority priority() const { return priority_; }
    const std::string& destination() const { return destination_; }

    void setDestination(const std::string& destination) { destination_ = destination; }
    void setPriority(Priority priority) { priority_ = priority; }

    double totalWeightKg() const;
    double totalVolumeM3() const;

private:
    std::string id_;
    std::string name_;
    SupplyCategory category_;
    int quantity_;
    double weightPerUnitKg_;
    double volumePerUnitM3_;
    Priority priority_;
    std::string destination_;
};

// UNIT II: dynamically allocated transport manifest with rule-of-five deep copy.
class TransportManifest {
public:
    TransportManifest();
    TransportManifest(const TransportManifest& other);
    TransportManifest& operator=(const TransportManifest& other);
    TransportManifest(TransportManifest&& other) noexcept;
    TransportManifest& operator=(TransportManifest&& other) noexcept;
    ~TransportManifest();

    // Add a new cargo line, or restock an existing line with the same ID.
    TransportManifest& operator+=(const ReliefCargo& line);
    // Deduct cargo on departure; lines reaching zero are removed.
    TransportManifest& operator-=(const ReliefCargo& line);
    // Compare two manifests (order-independent).
    bool operator==(const TransportManifest& other) const;
    bool operator!=(const TransportManifest& other) const { return !(*this == other); }

    friend std::ostream& operator<<(std::ostream& os, const TransportManifest& manifest);
    friend std::istream& operator>>(std::istream& is, TransportManifest& manifest);

    void clear() noexcept;
    bool empty() const noexcept { return count_ == 0; }
    std::size_t size() const noexcept { return count_; }
    std::size_t capacity() const noexcept { return capacity_; }

    const ReliefCargo& at(std::size_t index) const;
    ReliefCargo& at(std::size_t index);
    const ReliefCargo* find(const std::string& cargoId) const;
    ReliefCargo* find(const std::string& cargoId);

    double totalWeightKg() const;
    double totalVolumeM3() const;
    std::vector<ReliefCargo> snapshot() const;

private:
    void ensureCapacity(std::size_t required);
    std::size_t findIndex(const std::string& cargoId) const;
    void releaseAll() noexcept;

    ReliefCargo** lines_;
    std::size_t count_;
    std::size_t capacity_;
};

}  // namespace Inventory
}  // namespace ReliefOps

#endif  // ERLHUB_RELIEFCARGO_H
