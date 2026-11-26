#ifndef ERLHUB_AERIALTRANSPORT_H
#define ERLHUB_AERIALTRANSPORT_H

#include <cstddef>
#include <iosfwd>
#include <memory>
#include <string>

#include "TransportVehicle.h"

namespace ReliefOps {
namespace Logistics {

class AerialTransport : public TransportVehicle {
public:
    AerialTransport(std::string id, double fuelCapacity, double currentFuel,
                    double maxPayloadKg, VehicleStatus status, std::string model,
                    int crewCount = 3, bool rotorCraft = true,
                    double fuelRatePerKm = 0.85);
    ~AerialTransport() override;

    VehicleKind kind() const override { return VehicleKind::Aerial; }
    double fuelRatePerKm() const override { return fuelRatePerKm_; }
    RouteAssessment evaluateRoute(const std::string& destination,
                                  CorridorStatus corridor) const override;
    std::unique_ptr<ReliefVehicle> clone() const override;
    void printDetails(std::ostream& os) const override;

    int crewCount() const { return crewCount_; }
    bool rotorCraft() const { return rotorCraft_; }
    static std::size_t destroyedCount();

protected:
    int crewCount_;
    bool rotorCraft_;
    double fuelRatePerKm_;

private:
    static std::size_t destroyedCount_;
};

}  // namespace Logistics
}  // namespace ReliefOps

#endif  // ERLHUB_AERIALTRANSPORT_H
