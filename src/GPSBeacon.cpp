#include "GPSBeacon.h"

#include <iomanip>
#include <ostream>
#include <sstream>

namespace ReliefOps {
namespace Logistics {

GPSBeacon::GPSBeacon()
    : beaconId_("GPS-000"), latitude_(0.0), longitude_(0.0), active_(true) {}

GPSBeacon::GPSBeacon(std::string beaconId, double latitude, double longitude,
                     bool active)
    : beaconId_(std::move(beaconId)), latitude_(latitude),
      longitude_(longitude), active_(active) {}

void GPSBeacon::setPosition(double latitude, double longitude) {
    latitude_ = latitude;
    longitude_ = longitude;
}

std::string GPSBeacon::coordinates() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3) << latitude_ << ", " << longitude_;
    return oss.str();
}

std::ostream& operator<<(std::ostream& os, const GPSBeacon& beacon) {
    os << beacon.beaconId_ << " @" << beacon.coordinates()
       << (beacon.active_ ? " [ONLINE]" : " [OFFLINE]");
    return os;
}

}  // namespace Logistics
}  // namespace ReliefOps
