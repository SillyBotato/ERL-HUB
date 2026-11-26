#ifndef ERLHUB_GROUNDCONVOY_H
#define ERLHUB_GROUNDCONVOY_H

#include <cstddef>
#include <iosfwd>
#include <memory>
#include <string>

#include "TransportVehicle.h"

namespace ReliefOps {
namespace Logistics {

class GroundConvoy : virtual public TransportVehicle {
public:
    GroundConvoy(std::string id, double fuelCapacity, double currentFuel,
                 double maxPayloadKg, VehicleStatus status, std::string model,
                 int axles = 4, double fuelRatePerKm = 0.18);
    ~GroundConvoy() override;

    VehicleKind kind() const override { return VehicleKind::Ground; }
    double fuelRatePerKm() const override { return fuelRatePerKm_; }
    RouteAssessment evaluateRoute(const std::string& destination,
                                  CorridorStatus corridor) const override;
    std::unique_ptr<ReliefVehicle> clone() const override;
    void printDetails(std::ostream& os) const override;

    int axles() const { return axles_; }
    void setAxles(int axles) { axles_ = axles; }
    static std::size_t destroyedCount();

protected:
    int axles_;
    double fuelRatePerKm_;

private:
    static std::size_t destroyedCount_;
};

}  // namespace Logistics
}  // namespace ReliefOps

#endif  // ERLHUB_GROUNDCONVOY_H
