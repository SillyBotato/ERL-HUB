#include "Records.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <ostream>
#include <sstream>

namespace ReliefOps {
namespace Logistics {

std::uint64_t nowEpochSeconds() {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
}

std::string epochToString(std::uint64_t epochSeconds) {
    const std::time_t time = static_cast<std::time_t>(epochSeconds);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string makeUniqueId(const std::string& prefix, std::uint64_t sequence) {
    std::ostringstream oss;
    oss << prefix << '-' << std::setw(5) << std::setfill('0') << sequence;
    return oss.str();
}

std::ostream& operator<<(std::ostream& os, const DispatchRequest& request) {
    os << request.id << " | " << request.cargoName << " x" << request.quantity
       << " -> " << request.destination << " | "
       << Inventory::priorityToString(request.priority);
    return os;
}

std::ostream& operator<<(std::ostream& os, const DispatchRecord& record) {
    os << record.id << " | " << record.vehicleId << " -> " << record.destination
       << " | " << record.cargoName << " x" << record.quantity;
    return os;
}

}  // namespace Logistics
}  // namespace ReliefOps
