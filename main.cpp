#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "include/Manipulators.h"
#include "include/Records.h"
#include "include/ReliefHub.h"
#include "include/Types.h"
#include "include/Utilities.h"

using ReliefOps::Inventory::Priority;
using ReliefOps::Inventory::SupplyCategory;
using ReliefOps::Logistics::ReliefHub;
using ReliefOps::Logistics::VehicleKind;
using ReliefOps::Logistics::VehicleSpec;
using ReliefOps::Logistics::reportTitle;

namespace {

void clearLine() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

std::string readLine(const std::string& prompt) {
    std::cout << "  " << prompt << ": ";
    std::string value;
    std::getline(std::cin >> std::ws, value);
    return value;
}

std::string readNonEmpty(const std::string& prompt) {
    std::string value;
    do {
        value = readLine(prompt);
        if (value.empty()) {
            std::cout << "    ! Value must not be empty.\n";
        }
    } while (value.empty() && std::cin);
    return value;
}

int readInt(const std::string& prompt, int minimum, int maximum) {
    int value = 0;
    while (true) {
        std::cout << "  " << prompt << " [" << minimum << '-' << maximum << "]: ";
        if (std::cin >> value && value >= minimum && value <= maximum) {
            clearLine();
            return value;
        }
        std::cout << "    ! Please enter a number in range.\n";
        if (std::cin.eof()) {
            return minimum;
        }
        clearLine();
    }
}

double readDouble(const std::string& prompt, double minimum) {
    double value = 0.0;
    while (true) {
        std::cout << "  " << prompt << " (>= " << minimum << "): ";
        if (std::cin >> value && value >= minimum) {
            clearLine();
            return value;
        }
        std::cout << "    ! Please enter a numeric value in range.\n";
        if (std::cin.eof()) {
            return minimum;
        }
        clearLine();
    }
}

bool readYesNo(const std::string& prompt) {
    while (true) {
        const std::string answer = readLine(prompt + " (y/n)");
        if (answer == "y" || answer == "Y" || answer == "yes" ||
            answer == "YES") {
            return true;
        }
        if (answer == "n" || answer == "N" || answer == "no" ||
            answer == "NO") {
            return false;
        }
        std::cout << "    ! Please answer y or n.\n";
        if (std::cin.eof()) {
            return false;
        }
    }
}

int readFromList(const std::string& prompt,
                 const std::vector<std::string>& options) {
    std::cout << "  " << prompt << ":\n";
    for (std::size_t i = 0; i < options.size(); ++i) {
        std::cout << "    " << (i + 1) << ") " << options[i] << '\n';
    }
    return readInt("Select", 1, static_cast<int>(options.size()));
}

void printMenu() {
    std::cout << "\n========== ERL-HUB MAIN MENU ==========\n"
              << "  1. Add New Relief Cargo\n"
              << "  2. View Available Inventory\n"
              << "  3. Register Distribution Depot\n"
              << "  4. Restock Existing Cargo\n"
              << "  5. Register Transport Vehicle\n"
              << "  6. View Registered Vehicles\n"
              << "  7. Allocate Supplies to Vehicle\n"
              << "  8. Dispatch Emergency Vehicle\n"
              << "  9. View Emergency Priority Queue\n"
              << "  10. View Dispatch History\n"
              << "  11. Save Application Data\n"
              << "  12. Load Previously Saved Data\n"
              << "  13. Display Logistics Statistics\n"
              << "  14. Refuel Vehicle\n"
              << "  0. Exit\n"
              << "=======================================\n"
              << "  Enter choice: ";
}

SupplyCategory chooseCategory() {
    return static_cast<SupplyCategory>(readFromList(
        "Cargo category", {"Ration kits", "Medical supplies", "Clean water",
                           "Thermal blankets"}));
}

Priority choosePriority() {
    return ReliefOps::Inventory::priorityFromInt(readFromList(
        "Emergency priority",
        {"Routine", "Urgent", "Critical", "Medical emergency"}));
}

void addCargo(ReliefHub& hub) {
    const std::string depotId = readNonEmpty("Depot ID");
    if (readYesNo("Import cargo line from a text record")) {
        std::cout << "    Format: ID name category(1-4) quantity weight volume "
                     "priority(1-4) destination\n    Record> ";
        ReliefOps::Inventory::ReliefCargo cargo;
        if (!(std::cin >> cargo)) {
            clearLine();
            throw std::invalid_argument("Invalid cargo text record");
        }
        clearLine();
        std::cout << "  => "
                  << hub.addCargo(depotId, cargo.id(), cargo.name(),
                                  cargo.category(), cargo.quantity(),
                                  cargo.weightPerUnitKg(),
                                  cargo.volumePerUnitM3(), cargo.priority())
                  << " (record read with operator>>)\n";
        return;
    }
    const std::string cargoId = readNonEmpty("Cargo ID");
    const std::string name = readNonEmpty("Cargo name");
    const SupplyCategory category = chooseCategory();
    const int quantity = readInt("Quantity", 1, 1000000);
    const double weight = readDouble("Weight per unit (kg)", 0.0);
    const double volume = readDouble("Volume per unit (m3)", 0.001);
    const Priority priority = choosePriority();
    std::cout << "  => "
              << hub.addCargo(depotId, cargoId, name, category, quantity, weight,
                              volume, priority)
              << '\n';
}

void registerDepot(ReliefHub& hub) {
    const std::string id = readNonEmpty("Depot ID");
    const std::string name = readNonEmpty("Depot name");
    const std::string location = readNonEmpty("Location / district");
    std::cout << "  => " << hub.addDepot(id, name, location) << '\n';
}

void registerVehicle(ReliefHub& hub) {
    VehicleSpec spec;
    spec.kind = static_cast<VehicleKind>(readFromList(
        "Vehicle type",
        {"GroundConvoy", "AerialTransport", "AmphibiousCraft",
         "AmphibiousMedicalVehicle"}));
    spec.id = readNonEmpty("Vehicle ID");
    spec.model = readNonEmpty("Model / call sign");
    spec.fuelCapacity = readDouble("Fuel capacity (L)", 1.0);
    spec.currentFuel = readDouble("Current fuel (L)", 0.0);
    spec.maxPayloadKg = readDouble("Maximum payload (kg)", 1.0);
    spec.operational = readYesNo("Mark vehicle operational");
    spec.beaconId = "GPS-" + spec.id;
    spec.latitude = 28.61;
    spec.longitude = 77.21;
    spec.beaconActive = true;

    switch (spec.kind) {
        case VehicleKind::Ground:
            spec.axles = readInt("Axles", 2, 12);
            break;
        case VehicleKind::Aerial:
            spec.crewCount = readInt("Crew count", 1, 12);
            spec.rotorCraft = readYesNo("Rotorcraft (otherwise fixed-wing)");
            break;
        case VehicleKind::Amphibious:
            spec.hullCapacityTonnes = readDouble("Hull capacity (tonnes)", 0.1);
            spec.waterSpeedKnots = readDouble("Water speed (knots)", 0.1);
            break;
        case VehicleKind::AmphibiousMedical:
            spec.hullCapacityTonnes = readDouble("Hull capacity (tonnes)", 0.1);
            spec.waterSpeedKnots = readDouble("Water speed (knots)", 0.1);
            spec.axles = readInt("Axles", 2, 12);
            spec.triageBeds = readInt("Triage beds", 1, 40);
            spec.lifeSupportId = "ELS-" + spec.id;
            spec.oxygenUnits = readInt("Oxygen units", 0, 1000);
            spec.surgicalReady = readYesNo("Surgical team on board");
            break;
    }
    std::cout << "  => " << hub.registerVehicle(spec) << '\n';
}

void allocateSupplies(ReliefHub& hub) {
    const std::string depotId = readNonEmpty("Depot ID");
    const std::string vehicleId = readNonEmpty("Vehicle ID");
    const std::string cargoId = readNonEmpty("Cargo ID");
    const std::string destination = readNonEmpty("Destination");
    const int mode = readFromList(
        "Allocation mode",
        {"Volume-based allocation", "Quantity allocation / medical priority"});

    ReliefOps::Logistics::AllocationResult result;
    if (mode == 1) {
        const double volume = readDouble("Volume to allocate (m3)", 0.001);
        const Priority priority = choosePriority();
        result = hub.allocateVolume(depotId, vehicleId, cargoId, volume,
                                    destination, priority);
    } else {
        const int quantity = readInt("Quantity", 1, 1000000);
        const Priority priority = choosePriority();
        const bool triage = readYesNo("Apply emergency medical triage override");
        result = hub.allocateQuantity(depotId, vehicleId, cargoId, quantity,
                                      destination, priority, triage);
    }
    if (result.accepted) {
        std::cout << "  => " << result.message << '\n'
                  << "     Request " << result.request.id
                  << " queued. Dispatch to deduct stock is pending.\n";
    } else {
        std::cout << "  ! " << result.message << '\n';
    }
}

void dispatchEmergency(ReliefHub& hub) {
    const ReliefOps::Logistics::DispatchPreview preview =
        hub.previewNextDispatch();
    if (!preview.available) {
        std::cout << "  " << preview.message << '\n';
        return;
    }
    std::cout << preview.details;
    if (readYesNo("Confirm this dispatch")) {
        std::cout << "  => " << hub.dispatchNext(true) << '\n';
    } else {
        std::cout << "  => " << hub.dispatchNext(false) << '\n';
    }
}

void refuelVehicle(ReliefHub& hub) {
    const std::string vehicleId = readNonEmpty("Vehicle ID");
    const double litres = readDouble("Fuel quantity to add (L)", 0.01);
    std::cout << "    Requested: " << litres << " L\n";
    // Excess fuel is trimmed to the free tank space; the receipt states the
    // requested amount, the actual amount added and the final fuel level.
    std::cout << "  => " << hub.refuel(vehicleId, litres) << '\n';
}

}  // namespace

int main() {
    ReliefHub hub;
    std::cout << reportTitle("ERL-HUB :: EMERGENCY RELIEF LOGISTICS HUB")
              << "  Post-earthquake multi-agency dispatch simulator (C++17)\n"
              << "  Data file: data/erlhub.dat (use option 12 to restore a save)\n";

    while (true) {
        printMenu();
        int choice = 0;
        if (!(std::cin >> choice)) {
            std::cout << "\n  End of input. Exiting ERL-HUB.\n";
            break;
        }
        clearLine();
        if (choice == 0) {
            std::cout << "\n  ERL-HUB shutdown complete. Stay safe.\n";
            break;
        }
        try {
            switch (choice) {
                case 1:
                    addCargo(hub);
                    break;
                case 2:
                    hub.displayInventory(std::cout);
                    break;
                case 3:
                    registerDepot(hub);
                    break;
                case 4: {
                    const std::string depotId = readNonEmpty("Depot ID");
                    const std::string cargoId = readNonEmpty("Cargo ID");
                    const int units = readInt("Additional units", 1, 1000000);
                    std::cout << "  => " << hub.restock(depotId, cargoId, units)
                              << '\n';
                    break;
                }
                case 5:
                    registerVehicle(hub);
                    break;
                case 6:
                    hub.displayFleet(std::cout);
                    hub.displayRouteEvaluation(std::cout);
                    break;
                case 7:
                    allocateSupplies(hub);
                    break;
                case 8:
                    dispatchEmergency(hub);
                    break;
                case 9:
                    hub.displayQueue(std::cout);
                    break;
                case 10:
                    hub.displayHistory(std::cout);
                    hub.displayAudits(std::cout);
                    break;
                case 11:
                    std::cout << "  => "
                              << (hub.save() ? "Application data saved."
                                             : "Save failed (see diagnostics).")
                              << '\n';
                    break;
                case 12:
                    std::cout << "  => "
                              << (hub.load()
                                      ? "Saved data restored successfully."
                                      : "No valid saved data found.")
                              << '\n';
                    break;
                case 13:
                    hub.displayStatistics(std::cout);
                    break;
                case 14:
                    refuelVehicle(hub);
                    break;
                default:
                    std::cout << "  ! Invalid menu choice. Please try again.\n";
                    break;
            }
        } catch (const std::exception& error) {
            std::cout << "  ! Operation failed: " << error.what() << '\n';
        }
    }
    return 0;
}
