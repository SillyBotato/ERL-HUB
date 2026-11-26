#ifndef ERLHUB_EXCEPTIONS_H
#define ERLHUB_EXCEPTIONS_H

#include <exception>
#include <stdexcept>
#include <string>

// UNIT IV: three domain-specific exceptions derived from std::exception.
namespace ReliefOps {
namespace Logistics {

class CargoOverweightException : public std::runtime_error {
public:
    CargoOverweightException(double attemptedKg, double limitKg)
        : std::runtime_error("Cargo overweight: attempted " +
                             std::to_string(attemptedKg) + " kg, limit " +
                             std::to_string(limitKg) + " kg"),
          attemptedKg_(attemptedKg), limitKg_(limitKg) {}

    double attemptedKg() const noexcept { return attemptedKg_; }
    double limitKg() const noexcept { return limitKg_; }

private:
    double attemptedKg_;
    double limitKg_;
};

class FuelDepletionException : public std::runtime_error {
public:
    FuelDepletionException(double requiredL, double availableL)
        : std::runtime_error("Fuel depletion: required " +
                             std::to_string(requiredL) + " L, available " +
                             std::to_string(availableL) + " L"),
          requiredL_(requiredL), availableL_(availableL) {}

    double requiredL() const noexcept { return requiredL_; }
    double availableL() const noexcept { return availableL_; }

private:
    double requiredL_;
    double availableL_;
};

class DamagedCorridorException : public std::runtime_error {
public:
    explicit DamagedCorridorException(const std::string& destination)
        : std::runtime_error("Damaged corridor towards " + destination +
                             "; dispatch blocked"),
          destination_(destination) {}

    const std::string& destination() const noexcept { return destination_; }

private:
    std::string destination_;
};

}  // namespace Logistics
}  // namespace ReliefOps

#endif  // ERLHUB_EXCEPTIONS_H
