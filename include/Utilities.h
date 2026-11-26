#ifndef ERLHUB_UTILITIES_H
#define ERLHUB_UTILITIES_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <string>

// UNIT I: inline utility functions (no function-call overhead, header defined).
namespace ReliefOps {
namespace Logistics {

// Deterministic "map distance" so route estimates are reproducible in tests.
inline double distanceKm(const std::string& from, const std::string& to) {
    if (from == to) {
        return 0.0;
    }
    std::hash<std::string> hasher;
    const std::size_t h = hasher(from + "|" + to);
    const double base = 25.0 + static_cast<double>(h % 180U);
    const double fraction = static_cast<double>((h >> 9U) % 100U) / 100.0;
    return base + fraction;
}

// Inline payload calculation used by allocation and manifest reporting.
inline double payloadKg(int quantity, double weightPerUnitKg) {
    if (quantity <= 0 || weightPerUnitKg < 0.0) {
        return 0.0;
    }
    return static_cast<double>(quantity) * weightPerUnitKg;
}

inline double kilogramsToTonnes(double kilograms) {
    return kilograms / 1000.0;
}

inline double clampValue(double value, double low, double high) {
    return std::max(low, std::min(value, high));
}

inline bool nearlyEqual(double a, double b, double epsilon = 1e-9) {
    return std::fabs(a - b) <= epsilon;
}

}  // namespace Logistics
}  // namespace ReliefOps

#endif  // ERLHUB_UTILITIES_H
