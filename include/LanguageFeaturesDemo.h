#ifndef ERLHUB_LANGUAGEFEATURESDEMO_H
#define ERLHUB_LANGUAGEFEATURESDEMO_H

#include <cmath>
#include <string>

#include "ReliefCargo.h"

// Supporting examples for syllabus topics that the main application either
// uses only implicitly (allocators, raw arrays of objects) or demonstrates in
// domain-specific form (function templates, conversions, friend functions).
// These small types are NOT part of the relief logistics domain model.
namespace ReliefOps {
namespace Demos {

// EXPLICIT conversion: double -> Litres needs a cast/constructor call.
// IMPLICIT conversion: Litres -> double happens automatically via operator double.
// FRIEND FUNCTION: sameVolume() is a non-member that reads the private field.
class Litres {
public:
    explicit Litres(double amount) : amount_(amount) {}
    operator double() const { return amount_; }

    friend bool sameVolume(const Litres& a, const Litres& b) {
        return std::fabs(a.amount_ - b.amount_) < 1e-9;
    }

private:
    double amount_;
};

// Namespace-scope declaration makes the friend function visible to ordinary
// qualified lookup as well as argument-dependent lookup (ADL).
bool sameVolume(const Litres& a, const Litres& b);

// GENERIC FUNCTION TEMPLATE: works for any container whose elements expose a
// getter callable that returns a numeric value (duck typing).
template <typename Container, typename Getter>
double totalOf(const Container& items, Getter getter) {
    double total = 0.0;
    for (const auto& item : items) {  // range-for uses begin()/end() iterators
        total += static_cast<double>(getter(item));
    }
    return total;
}

std::string languageFeaturesReport();

}  // namespace Demos
}  // namespace ReliefOps

#endif  // ERLHUB_LANGUAGEFEATURESDEMO_H
