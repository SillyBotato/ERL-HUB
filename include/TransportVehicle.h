#ifndef ERLHUB_TRANSPORTVEHICLE_H
#define ERLHUB_TRANSPORTVEHICLE_H

#include <cstddef>
#include <string>

#include "ReliefVehicle.h"

// Added file (permitted by the brief): the concrete transport layer that carries
// the UNIT II static members and is the common virtual base of the diamond.
namespace ReliefOps {
namespace Logistics {

class TransportVehicle : virtual public ReliefVehicle {
public:
    TransportVehicle(std::string id, double fuelCapacity, double currentFuel,
                     double maxPayloadKg, VehicleStatus status, std::string model);
    TransportVehicle(const TransportVehicle& other);
    TransportVehicle& operator=(const TransportVehicle& other);
    ~TransportVehicle() override;

    const std::string& model() const { return model_; }

    // UNIT II: static data members and static retrieval functions.
    static std::size_t totalOperationalVehicles();
    static double totalDispatchedTonnes();
    static void addDispatchedTonnes(double tonnes);
    static void setTotalDispatchedTonnes(double tonnes);

protected:
    std::string model_;

private:
    static std::size_t operationalVehicles_;
    static double totalDispatchedTonnes_;
};

}  // namespace Logistics
}  // namespace ReliefOps

#endif  // ERLHUB_TRANSPORTVEHICLE_H
