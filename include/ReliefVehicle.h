#ifndef ERLHUB_RELIEFVEHICLE_H
#define ERLHUB_RELIEFVEHICLE_H

#include <cstddef>
#include <iosfwd>
#include <memory>
#include <string>

#include "Exceptions.h"
#include "GPSBeacon.h"
#include "ReliefCargo.h"
#include "Types.h"
#include "Utilities.h"

namespace ReliefOps {
namespace Logistics {

class IncidentCommander;

// UNIT III: abstract base class with virtual destructor and pure virtual interface.
class ReliefVehicle {
public:
    ReliefVehicle(std::string id, double fuelCapacity, double currentFuel,
                  double maxPayloadKg, VehicleStatus status,
                  GPSBeacon beacon = GPSBeacon());
    ReliefVehicle(const ReliefVehicle&) = default;
    ReliefVehicle& operator=(const ReliefVehicle&) = default;
    virtual ~ReliefVehicle();

    // Pure virtual interface — forces dynamic dispatch through base pointers.
    virtual VehicleKind kind() const = 0;
    virtual double fuelRatePerKm() const = 0;
    virtual RouteAssessment evaluateRoute(const std::string& destination,
                                          CorridorStatus corridor) const = 0;
    virtual std::unique_ptr<ReliefVehicle> clone() const = 0;
    virtual void printDetails(std::ostream& os) const;

    // Fuel management. Refuelling is explicit; dispatch never refuels.
    void consumeFuel(double litres);

    // Adds up to the remaining tank capacity and returns the amount actually
    // added. A request larger than the free space is trimmed, never rejected.
    double refuel(double litres);
    void setStatus(VehicleStatus status) { status_ = status; }
    void setDestination(const std::string& destination) { destination_ = destination; }
    void setBeacon(const GPSBeacon& beacon) { beacon_ = beacon; }

    // Cargo manifest API (return by reference to the live object).
    void assignManifest(const Inventory::TransportManifest& manifest);
    void clearManifest() { manifest_.clear(); }

    // UNIT I: functions returning references rather than copies.
    Inventory::TransportManifest& manifestRef() { return manifest_; }
    const Inventory::TransportManifest& manifestRef() const { return manifest_; }
    GPSBeacon& beaconRef() { return beacon_; }
    const GPSBeacon& beaconRef() const { return beacon_; }

    const std::string& id() const { return id_; }
    double fuelCapacity() const { return fuelCapacity_; }
    double currentFuel() const { return currentFuel_; }
    double maxPayloadKg() const { return maxPayloadKg_; }
    VehicleStatus status() const { return status_; }
    const std::string& destination() const { return destination_; }
    const GPSBeacon& beacon() const { return beacon_; }
    const Inventory::TransportManifest& manifest() const { return manifest_; }

    // Debug/verification helper: proves base destructor is virtual and invoked.
    static std::size_t destructorCalls();
    static void resetDestructorTrace();

protected:
    // Common route computation reused by every derived evaluateRoute().
    RouteAssessment assessRoute(const std::string& destination,
                                CorridorStatus corridor) const;

    std::string id_;
    double fuelCapacity_;
    double currentFuel_;
    double maxPayloadKg_;
    VehicleStatus status_;
    std::string destination_;
    bool corridorOverride_;
    GPSBeacon beacon_;
    Inventory::TransportManifest manifest_;

private:
    static std::size_t destructorCalls_;

    friend class IncidentCommander;
};

}  // namespace Logistics
}  // namespace ReliefOps

#endif  // ERLHUB_RELIEFVEHICLE_H
