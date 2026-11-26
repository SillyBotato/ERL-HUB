#ifndef ERLHUB_GPSBEACON_H
#define ERLHUB_GPSBEACON_H

#include <iosfwd>
#include <string>

// UNIT III: containership — a GPSBeacon is composed inside ReliefVehicle.
namespace ReliefOps {
namespace Logistics {

class GPSBeacon {
public:
    GPSBeacon();
    GPSBeacon(std::string beaconId, double latitude, double longitude, bool active = true);

    const std::string& beaconId() const { return beaconId_; }
    double latitude() const { return latitude_; }
    double longitude() const { return longitude_; }
    bool active() const { return active_; }

    void activate() { active_ = true; }
    void deactivate() { active_ = false; }
    void setPosition(double latitude, double longitude);

    std::string coordinates() const;
    friend std::ostream& operator<<(std::ostream& os, const GPSBeacon& beacon);

private:
    std::string beaconId_;
    double latitude_;
    double longitude_;
    bool active_;
};

}  // namespace Logistics
}  // namespace ReliefOps

#endif  // ERLHUB_GPSBEACON_H
