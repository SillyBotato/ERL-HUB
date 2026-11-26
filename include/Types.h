#ifndef ERLHUB_TYPES_H
#define ERLHUB_TYPES_H

#include <string>

// UNIT I: two required project namespaces.
namespace ReliefOps {
namespace Inventory {

enum class SupplyCategory {
    Rations = 1,
    Medical = 2,
    Water = 3,
    Blankets = 4
};

enum class Priority {
    Routine = 1,
    Urgent = 2,
    Critical = 3,
    MedicalEmergency = 4
};

inline std::string categoryToString(SupplyCategory c) {
    switch (c) {
        case SupplyCategory::Rations:  return "Ration kits";
        case SupplyCategory::Medical:  return "Medical supplies";
        case SupplyCategory::Water:    return "Clean water";
        case SupplyCategory::Blankets: return "Thermal blankets";
    }
    return "Unknown";
}

inline std::string priorityToString(Priority p) {
    switch (p) {
        case Priority::Routine:          return "Routine";
        case Priority::Urgent:           return "Urgent";
        case Priority::Critical:         return "Critical";
        case Priority::MedicalEmergency: return "MEDICAL EMERGENCY";
    }
    return "Unknown";
}

inline Priority priorityFromInt(int value) {
    switch (value) {
        case 4: return Priority::MedicalEmergency;
        case 3: return Priority::Critical;
        case 2: return Priority::Urgent;
        default: return Priority::Routine;
    }
}

inline SupplyCategory categoryFromInt(int value) {
    switch (value) {
        case 2: return SupplyCategory::Medical;
        case 3: return SupplyCategory::Water;
        case 4: return SupplyCategory::Blankets;
        default: return SupplyCategory::Rations;
    }
}

}  // namespace Inventory

namespace Logistics {

enum class VehicleKind {
    Ground = 1,
    Aerial = 2,
    Amphibious = 3,
    AmphibiousMedical = 4
};

enum class VehicleStatus {
    Available = 1,
    OnMission = 2,
    Refuelling = 3,
    Retired = 4
};

enum class CorridorStatus {
    Clear = 1,
    Congested = 2,
    Damaged = 3
};

struct RouteAssessment {
    bool feasible = false;
    double distanceKm = 0.0;
    double fuelLitres = 0.0;
    CorridorStatus corridor = CorridorStatus::Clear;
    std::string note;
};

inline std::string vehicleKindToString(VehicleKind k) {
    switch (k) {
        case VehicleKind::Ground:            return "GroundConvoy";
        case VehicleKind::Aerial:            return "AerialTransport";
        case VehicleKind::Amphibious:        return "AmphibiousCraft";
        case VehicleKind::AmphibiousMedical: return "AmphibiousMedicalVehicle";
    }
    return "Unknown";
}

inline std::string vehicleStatusToString(VehicleStatus s) {
    switch (s) {
        case VehicleStatus::Available:  return "Available";
        case VehicleStatus::OnMission:  return "OnMission";
        case VehicleStatus::Refuelling: return "Refuelling";
        case VehicleStatus::Retired:    return "Retired";
    }
    return "Unknown";
}

inline std::string corridorToString(CorridorStatus c) {
    switch (c) {
        case CorridorStatus::Clear:     return "Clear";
        case CorridorStatus::Congested: return "Congested";
        case CorridorStatus::Damaged:   return "Damaged";
    }
    return "Unknown";
}

}  // namespace Logistics
}  // namespace ReliefOps

#endif  // ERLHUB_TYPES_H
