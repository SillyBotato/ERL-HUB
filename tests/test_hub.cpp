// ERL-HUB automated regression harness. Compiled by `make tests` / `make asan-tests`.
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "../include/AerialTransport.h"
#include "../include/AmphibiousCraft.h"
#include "../include/AmphibiousMedicalVehicle.h"
#include "../include/Allocation.h"
#include "../include/DepotRegistry.h"
#include "../include/DistributionDepot.h"
#include "../include/Exceptions.h"
#include "../include/FileManager.h"
#include "../include/GroundConvoy.h"
#include "../include/IncidentCommander.h"
#include "../include/InheritanceAccessDemo.h"
#include "../include/LanguageFeaturesDemo.h"
#include "../include/Manipulators.h"
#include "../include/PriorityDispatchQueue.h"
#include "../include/Records.h"
#include "../include/ReliefHub.h"
#include "../include/TransportVehicle.h"
#include "../include/Utilities.h"

using namespace ReliefOps;
using namespace ReliefOps::Logistics;
using ReliefOps::Inventory::DistributionDepot;
using ReliefOps::Inventory::Priority;
using ReliefOps::Inventory::ReliefCargo;
using ReliefOps::Inventory::SupplyCategory;
using ReliefOps::Inventory::TransportManifest;

namespace {

int passed = 0;
int failed = 0;

void check(const std::string& name, bool condition) {
    if (condition) {
        ++passed;
        std::cout << "  [PASS] " << name << '\n';
    } else {
        ++failed;
        std::cout << "  [FAIL] " << name << '\n';
    }
}

template <typename Exception, typename Callable>
bool throws(Callable callable) {
    try {
        callable();
    } catch (const Exception&) {
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

void resetGlobals() {
    TransportVehicle::setTotalDispatchedTonnes(0.0);
}

VehicleSpec specFor(VehicleKind kind, const std::string& id, double fuel,
                    double payload = 5000.0) {
    VehicleSpec spec;
    spec.kind = kind;
    spec.id = id;
    spec.model = "Test-" + id;
    spec.fuelCapacity = 200.0;
    spec.currentFuel = fuel;
    spec.maxPayloadKg = payload;
    spec.operational = true;
    spec.axles = 4;
    spec.crewCount = 3;
    spec.rotorCraft = true;
    spec.hullCapacityTonnes = 8.0;
    spec.waterSpeedKnots = 18.0;
    spec.triageBeds = 4;
    spec.lifeSupportId = "ELS-" + id;
    spec.oxygenUnits = 20;
    spec.surgicalReady = true;
    spec.beaconId = "GPS-" + id;
    return spec;
}

void seedHub(ReliefHub& hub, double fuel = 200.0,
             double payload = 5000.0, VehicleKind kind = VehicleKind::Ground) {
    hub.addDepot("D1", "Central Depot", "Sector A");
    hub.addCargo("D1", "C1", "Ration kits", SupplyCategory::Rations, 100,
                 10.0, 0.5, Priority::Urgent);
    hub.registerVehicle(specFor(kind, "V1", fuel, payload));
}

// ---------------------------------------------------------------------------

void test01_inventory() {
    std::cout << "\n[01] Normal inventory management\n";
    resetGlobals();
    ReliefHub hub;
    seedHub(hub);
    check("depot and cargo registered", hub.depotCount() == 1 &&
                                           hub.depotRef("D1").cargoCount() == 1);
    check("addCargo duplicate rejected",
          throws<std::invalid_argument>(
              [&] { hub.addCargo("D1", "C1", "Duplicate", SupplyCategory::Water,
                                 1, 1.0, 1.0, Priority::Routine); }));
    hub.restock("D1", "C1", 25);
    check("restock uses overloaded +=",
          hub.depotRef("D1").cargoRef("C1").quantity() == 125);
    check("negative restock rejected",
          throws<std::invalid_argument>([&] { hub.restock("D1", "C1", -3); }));
}

void test02_allocation() {
    std::cout << "\n[02] Successful supply allocation\n";
    resetGlobals();
    ReliefHub hub;
    seedHub(hub);
    const double fuelBefore = hub.vehicleRef("V1").currentFuel();
    const auto result = hub.allocateQuantity("D1", "V1", "C1", 20, "Village North",
                                             Priority::Critical, false);
    check("allocation accepted", result.accepted);
    check("request queued", hub.pendingCount() == 1);
    check("inventory untouched until dispatch",
          hub.depotRef("D1").cargoRef("C1").quantity() == 100);
    check("fuel untouched until dispatch",
          hub.vehicleRef("V1").currentFuel() == fuelBefore);
    check("payload recorded", std::fabs(result.request.weightKg - 200.0) < 1e-9);
}

void test03_dispatch() {
    std::cout << "\n[03] Successful dispatch\n";
    resetGlobals();
    ReliefHub hub;
    seedHub(hub);
    hub.allocateQuantity("D1", "V1", "C1", 30, "Village East", Priority::Urgent,
                         false);
    const double fuelBefore = hub.vehicleRef("V1").currentFuel();
    const DispatchPreview preview = hub.previewNextDispatch();
    check("preview available with full details",
          preview.available && preview.details.find("Estimated fuel") !=
                                   std::string::npos);
    const std::string receipt = hub.dispatchNext(true);
    check("dispatch completed", receipt.find("completed") != std::string::npos);
    check("stock deducted exactly once",
          hub.depotRef("D1").cargoRef("C1").quantity() == 70);
    check("fuel reduced", hub.vehicleRef("V1").currentFuel() < fuelBefore);
    check("status updated",
          hub.vehicleRef("V1").status() == VehicleStatus::OnMission);
    check("history recorded once", hub.historyCount() == 1);
    check("tonnage updated", TransportVehicle::totalDispatchedTonnes() > 0.0);
    check("queue drained", hub.pendingCount() == 0);
}

void test04_overweight() {
    std::cout << "\n[04] Overweight cargo\n";
    resetGlobals();
    ReliefHub hub;
    hub.addDepot("D1", "Depot", "Sector A");
    hub.addCargo("D1", "HEAVY", "Generators", SupplyCategory::Medical, 10,
                 2000.0, 1.0, Priority::Critical);
    hub.registerVehicle(specFor(VehicleKind::Ground, "V1", 200.0, 1000.0));
    check("overweight throws CargoOverweightException",
          throws<CargoOverweightException>([&] {
              hub.allocateQuantity("D1", "V1", "HEAVY", 5, "Camp",
                                   Priority::Urgent, false);
          }));
    check("failed allocation leaves no request", hub.pendingCount() == 0);
    check("failed allocation leaves stock intact",
          hub.depotRef("D1").cargoRef("HEAVY").quantity() == 10);
}

void test05_insufficient_fuel() {
    std::cout << "\n[05] Insufficient fuel\n";
    resetGlobals();
    ReliefHub hub;
    seedHub(hub, 0.5);
    hub.allocateQuantity("D1", "V1", "C1", 5, "Remote Camp", Priority::Urgent,
                         false);
    const int stockBefore = hub.depotRef("D1").cargoRef("C1").quantity();
    const double tonnesBefore = TransportVehicle::totalDispatchedTonnes();
    check("dispatch throws FuelDepletionException",
          throws<FuelDepletionException>([&] { hub.dispatchNext(true); }));
    check("stock unchanged after rollback",
          hub.depotRef("D1").cargoRef("C1").quantity() == stockBefore);
    check("tonnage unchanged after rollback",
          TransportVehicle::totalDispatchedTonnes() == tonnesBefore);
    check("history unchanged after rollback", hub.historyCount() == 0);
    check("request retained for later refuelling", hub.pendingCount() == 1);
}

void test06_damaged_corridor() {
    std::cout << "\n[06] Damaged corridor and commander override\n";
    ReliefHub hub;
    seedHub(hub);
    ReliefVehicle& vehicle = hub.vehicleRef("V1");
    check("damaged corridor throws",
          throws<DamagedCorridorException>(
              [&] { vehicle.evaluateRoute("Bridge City", CorridorStatus::Damaged); }));
    IncidentCommander commander("Colonel Rao");
    commander.authorizeDamagedCorridor(vehicle, true);
    const RouteAssessment assessment =
        vehicle.evaluateRoute("Bridge City", CorridorStatus::Damaged);
    check("commander override permits route", assessment.feasible);
    check("inspection exposes restricted state",
          commander.inspectRestricted(vehicle).find("corridorOverride=ON") !=
              std::string::npos);
    commander.authorizeDamagedCorridor(vehicle, false);
    check("override can be revoked",
          throws<DamagedCorridorException>(
              [&] { vehicle.evaluateRoute("Bridge City", CorridorStatus::Damaged); }));
}

void test07_duplicate_cargo() {
    std::cout << "\n[07] Duplicate cargo and manifest operators\n";
    DistributionDepot depot("D2", "North", "Sector B");
    const ReliefCargo cargo("X1", "Water", SupplyCategory::Water, 10, 5.0, 0.25,
                            Priority::Urgent);
    depot.addCargo(cargo);
    check("ReliefCargo == detects duplicate ID", cargo == depot.cargoRef("X1"));
    check("depot duplicate throws", throws<std::invalid_argument>(
                                        [&] { depot.addCargo(cargo); }));

    TransportManifest manifest;
    manifest += cargo;  // new line
    manifest += ReliefCargo("X1", "Water", SupplyCategory::Water, 5, 5.0, 0.25,
                            Priority::Urgent);
    check("manifest += restocks instead of duplicating",
          manifest.size() == 1 && manifest.at(0).quantity() == 15);
    manifest += ReliefCargo("Y2", "Blankets", SupplyCategory::Blankets, 4, 2.0,
                            0.1, Priority::Routine);
    TransportManifest reordered;
    reordered += ReliefCargo("Y2", "Blankets", SupplyCategory::Blankets, 4,
                             2.0, 0.1, Priority::Routine);
    reordered += cargo;  // X1 quantity 10
    reordered += ReliefCargo("X1", "Water", SupplyCategory::Water, 5, 5.0,
                             0.25, Priority::Urgent);
    check("manifest == works across order", manifest == reordered);

    std::ostringstream out;
    out << manifest;
    TransportManifest restored;
    std::istringstream in(out.str());
    in >> restored;
    check("manifest << / >> round trip", restored == manifest);
    check("manifest -= deduct uses overloaded operator",
          (manifest -= ReliefCargo("X1", "Water", SupplyCategory::Water, 15,
                                   5.0, 0.25, Priority::Urgent)).size() == 1);
}

void test08_cancelled_dispatch() {
    std::cout << "\n[08] Cancelled dispatch\n";
    resetGlobals();
    ReliefHub hub;
    seedHub(hub);
    hub.allocateQuantity("D1", "V1", "C1", 10, "Village West", Priority::Critical,
                         false);
    const double fuelBefore = hub.vehicleRef("V1").currentFuel();
    const double tonnesBefore = TransportVehicle::totalDispatchedTonnes();
    const std::string message = hub.dispatchNext(false);
    check("cancel message returned", message.find("cancelled") != std::string::npos);
    check("inventory unchanged", hub.depotRef("D1").cargoRef("C1").quantity() == 100);
    check("fuel unchanged", hub.vehicleRef("V1").currentFuel() == fuelBefore);
    check("statistics unchanged",
          TransportVehicle::totalDispatchedTonnes() == tonnesBefore);
    check("history unchanged", hub.historyCount() == 0);
    check("request removed", hub.pendingCount() == 0);
}

void test09_invalid_inputs() {
    std::cout << "\n[09] Invalid user inputs\n";
    resetGlobals();
    ReliefHub hub;
    seedHub(hub);
    check("unknown depot rejected", throws<std::out_of_range>(
                                        [&] { hub.restock("NOPE", "C1", 1); }));
    check("unknown vehicle rejected", throws<std::out_of_range>([&] {
              hub.allocateQuantity("D1", "NOPE", "C1", 1, "Town",
                                   Priority::Routine, false);
          }));
    check("empty destination rejected", throws<std::invalid_argument>([&] {
              hub.allocateQuantity("D1", "V1", "C1", 1, "", Priority::Routine,
                                   false);
          }));
    check("zero allocation rejected", throws<std::invalid_argument>([&] {
              hub.allocateQuantity("D1", "V1", "C1", 0, "Town",
                                   Priority::Routine, false);
          }));
    check("duplicate vehicle rejected", throws<std::invalid_argument>(
                                            [&] { hub.registerVehicle(specFor(
                                                      VehicleKind::Ground, "V1", 50)); }));
    check("empty pending queue preview is safe",
          !hub.previewNextDispatch().available);
}

void test10_refuelling() {
    std::cout << "\n[10] Refuelling with automatic trimming (no auto-refuel at dispatch)\n";
    resetGlobals();
    const std::string path = "tests/test_outputs/refuel_test.dat";
    ReliefHub hub(path);
    seedHub(hub, 60.0);
    hub.allocateQuantity("D1", "V1", "C1", 5, "Town", Priority::Routine, false);
    hub.dispatchNext(true);
    const double depleted = hub.vehicleRef("V1").currentFuel();
    const double capacity = hub.vehicleRef("V1").fuelCapacity();
    check("fuel stays depleted after dispatch", depleted < 60.0);
    check("negative refuel rejected", throws<std::invalid_argument>(
                                          [&] { hub.refuel("V1", -5.0); }));
    check("zero refuel rejected", throws<std::invalid_argument>(
                                      [&] { hub.refuel("V1", 0.0); }));

    // 1000 L requested with only (capacity - depleted) L of free space.
    const std::string receipt = hub.refuel("V1", 1000.0);
    const double expectedAdded = capacity - depleted;
    check("overfill request trimmed, not rejected",
          std::fabs(hub.vehicleRef("V1").currentFuel() - capacity) < 1e-9);
    check("receipt displays requested amount",
          receipt.find("requested 1000.000000 L") != std::string::npos);
    check("receipt displays actual amount added",
          receipt.find("actually added " + std::to_string(expectedAdded)) !=
              std::string::npos);
    check("receipt displays final fuel level",
          receipt.find("final fuel " + std::to_string(capacity)) !=
              std::string::npos);

    // A request that fits is added in full (the trimming must not over-correct).
    hub.registerVehicle(specFor(VehicleKind::Ground, "V2", 60.0));
    const std::string partial = hub.refuel("V2", 50.0);
    check("partial request added in full",
          std::fabs(hub.vehicleRef("V2").currentFuel() - 110.0) < 1e-9);
    check("partial receipt reports exact addition",
          partial.find("actually added 50.000000 L") != std::string::npos);

    std::ifstream file(path, std::ios::binary);
    check("refuelled fuel levels persisted to disk", file.good());
    ReliefHub restarted(path);
    check("persisted file loads", restarted.load());
    check("trimmed final fuel restored exactly",
          std::fabs(restarted.vehicleRef("V1").currentFuel() - capacity) < 1e-9);
    check("partial refuel restored exactly",
          std::fabs(restarted.vehicleRef("V2").currentFuel() - 110.0) < 1e-9);
}

void test11_persistence() {
    std::cout << "\n[11] Binary save/load across restart\n";
    resetGlobals();
    const std::string path = "tests/test_outputs/save_test.dat";
    double fuelAfterDispatch = 0.0;
    {
        ReliefHub hub(path);
        seedHub(hub, 120.0, 5000.0, VehicleKind::AmphibiousMedical);
        hub.allocateQuantity("D1", "V1", "C1", 12, "Hospital",
                             Priority::MedicalEmergency, true);
        hub.dispatchNext(true);
        fuelAfterDispatch = hub.vehicleRef("V1").currentFuel();
        hub.allocateQuantity("D1", "V1", "C1", 4, "Clinic", Priority::Urgent,
                             false);
        check("save succeeded", hub.save());
    }  // simulate application restart: all objects destroyed
    std::ifstream raw(path, std::ios::binary);
    check("binary file created", raw.good());
    char magic[8] = {};
    raw.read(magic, 8);
    check("magic header verified", std::string(magic, 8) == "ERLHUB01");

    ReliefHub reloaded(path);
    check("load succeeded", reloaded.load());
    check("depot inventory restored",
          reloaded.depotRef("D1").cargoRef("C1").quantity() == 88);
    check("vehicle type restored",
          reloaded.vehicleRef("V1").kind() == VehicleKind::AmphibiousMedical);
    check("fuel restored exactly",
          std::fabs(reloaded.vehicleRef("V1").currentFuel() -
                    fuelAfterDispatch) < 1e-9);
    check("history restored", reloaded.historyCount() == 1);
    check("pending request restored", reloaded.pendingCount() == 1);
    check("tonnage restored", TransportVehicle::totalDispatchedTonnes() > 0.0);
    check("operational vehicle count restored",
          TransportVehicle::totalOperationalVehicles() == 1);
}

void test12_virtual_destructor() {
    std::cout << "\n[12] Virtual destructor invocation\n";
    const std::size_t baseline = TransportVehicle::totalOperationalVehicles();
    ReliefVehicle::resetDestructorTrace();
    const std::size_t groundBefore = GroundConvoy::destroyedCount();
    const std::size_t aerialBefore = AerialTransport::destroyedCount();
    const std::size_t amphibiousBefore = AmphibiousCraft::destroyedCount();
    const std::size_t medicalBefore = AmphibiousMedicalVehicle::destroyedCount();
    {
        std::vector<std::unique_ptr<ReliefVehicle>> vehicles;
        vehicles.push_back(std::unique_ptr<ReliefVehicle>(
            new GroundConvoy("G", 50, 50, 1000, VehicleStatus::Available, "G")));
        vehicles.push_back(std::unique_ptr<ReliefVehicle>(
            new AerialTransport("A", 50, 50, 1000, VehicleStatus::Available, "A")));
        vehicles.push_back(std::unique_ptr<ReliefVehicle>(
            new AmphibiousCraft("B", 50, 50, 1000, VehicleStatus::Available, "B")));
        vehicles.push_back(std::unique_ptr<ReliefVehicle>(
            new AmphibiousMedicalVehicle("M", 50, 50, 1000,
                                         VehicleStatus::Available, "M", 8, 18, 4,
                                         4, EmergencyLifeSupportUnit("ELS-M", 10, true))));
        check("four vehicles constructed polymorphically", vehicles.size() == 4);
        check("dynamic dispatch via base pointers",
              vehicles[0]->kind() == VehicleKind::Ground &&
                  vehicles[3]->kind() == VehicleKind::AmphibiousMedical);
    }
    // The hybrid also destroys its GroundConvoy and AmphibiousCraft branches,
    // hence +2 for those two base destructors and +1 for the aerial vehicle.
    check("GroundConvoy destructor ran (direct + hybrid branch)",
          GroundConvoy::destroyedCount() == groundBefore + 2);
    check("AerialTransport destructor ran",
          AerialTransport::destroyedCount() == aerialBefore + 1);
    check("AmphibiousCraft destructor ran (direct + hybrid branch)",
          AmphibiousCraft::destroyedCount() == amphibiousBefore + 2);
    check("hybrid destructor ran",
          AmphibiousMedicalVehicle::destroyedCount() == medicalBefore + 1);
    check("base destructor count is four", ReliefVehicle::destructorCalls() == 4);
    check("static vehicle counter balanced",
          TransportVehicle::totalOperationalVehicles() == baseline);
}

void test13_deep_copy() {
    std::cout << "\n[13] Deep copy correctness\n";
    ReliefCargo first("A", "Rations", SupplyCategory::Rations, 5, 2.0, 0.2,
                      Priority::Urgent);
    TransportManifest original;
    original += first;
    original += ReliefCargo("B", "Water", SupplyCategory::Water, 3, 1.0, 0.1,
                            Priority::Routine);

    TransportManifest copy(original);  // deep copy constructor
    copy.at(0).setPriority(Priority::MedicalEmergency);
    copy += ReliefCargo("C", "Blankets", SupplyCategory::Blankets, 1, 1.0, 0.1,
                        Priority::Routine);
    check("copy is independent of original",
          original.at(0).priority() == Priority::Urgent && original.size() == 2 &&
              copy.size() == 3);

    TransportManifest assigned;
    assigned = original;  // deep copy assignment
    check("copy assignment equalises value", assigned == original);
    assigned.at(1).setPriority(Priority::Critical);
    check("copy assignment is deep", original.at(1).priority() == Priority::Routine);
    assigned = assigned;  // self-assignment must be harmless
    check("self assignment safe", assigned.size() == 2);

    {
        AmphibiousMedicalVehicle hybrid("HY", 100, 100, 1000,
                                        VehicleStatus::Available, "Med", 8, 18,
                                        4, 6, EmergencyLifeSupportUnit("ELS-H", 5, true));
        ReliefVehicle* viaBase = &hybrid;
        TransportVehicle* viaTransport = &hybrid;
        check("hybrid exposes a single fuel tank",
              viaBase->currentFuel() == viaTransport->currentFuel());
        std::unique_ptr<ReliefVehicle> clone = hybrid.clone();
        check("clone preserves hybrid kind",
              clone->kind() == VehicleKind::AmphibiousMedical);
        check("clone has independent manifest pointer",
              &clone->manifestRef() != &hybrid.manifestRef());
    }
}

void test14_memory_safety() {
    std::cout << "\n[14] Memory safety stress (run under ASan)\n";
    const std::size_t baseline = TransportVehicle::totalOperationalVehicles();
    {
        std::vector<std::unique_ptr<ReliefVehicle>> fleet;
        for (int i = 0; i < 500; ++i) {
            fleet.push_back(std::unique_ptr<ReliefVehicle>(
                new AmphibiousMedicalVehicle(
                    "M" + std::to_string(i), 100, 50, 1000,
                    VehicleStatus::Available, "Med", 8, 18, 4, 2,
                    EmergencyLifeSupportUnit("ELS", 5, true))));
            fleet.back()->assignManifest(TransportManifest() += ReliefCargo(
                                             "C", "Water", SupplyCategory::Water,
                                             3, 1.0, 0.1, Priority::Urgent));
        }
        check("500 hybrids alive", TransportVehicle::totalOperationalVehicles() ==
                                       baseline + 500);
        std::vector<std::unique_ptr<ReliefVehicle>> clones;
        for (const auto& vehicle : fleet) {
            clones.push_back(vehicle->clone());
        }
        check("500 deep polymorphic clones created", clones.size() == 500);
    }
    check("all objects freed and static count balanced",
          TransportVehicle::totalOperationalVehicles() == baseline);

    // Inline utilities and manipulators are genuinely exercised here.
    check("inline payload calculation",
          std::fabs(payloadKg(3, 2.5) - 7.5) < 1e-9);
    check("inline distance is deterministic",
          distanceKm("A", "B") == distanceKm("A", "B"));
    std::ostringstream report;
    report << reportTitle("UNIT TEST") << manifestTableHeader
           << manifestTableFooter << dispatchReportHeader
           << dispatchReportFooter;
    check("custom manipulators emit formatted text",
          report.str().find("UNIT TEST") != std::string::npos &&
              report.str().find("Cargo ID") != std::string::npos);
}

void test15_inheritance_access() {
    std::cout << "\n[15] Inheritance access modes (supporting demo)\n";
    using ReliefOps::Demos::AccessBase;
    using ReliefOps::Demos::PrivatelyDerived;
    using ReliefOps::Demos::ProtectedlyDerived;
    using ReliefOps::Demos::PublicGrandchild;
    using ReliefOps::Demos::PubliclyDerived;
    using ReliefOps::Demos::inheritanceAccessReport;

    PubliclyDerived pub(1, 10, 20, 30);
    PrivatelyDerived priv(2, 10, 20, 30);
    ProtectedlyDerived prot(3, 10, 20, 30);
    PublicGrandchild grand(4, 10, 20, 30);

    check("public inheritance allows outside upcast",
          std::is_convertible<PubliclyDerived*, AccessBase*>::value);
    check("private inheritance blocks outside upcast",
          !std::is_convertible<PrivatelyDerived*, AccessBase*>::value);
    check("protected inheritance blocks outside upcast",
          !std::is_convertible<ProtectedlyDerived*, AccessBase*>::value);
    check("public mode exposes public and protected through derived API",
          pub.accessibleTotal() == 30 && pub.privateValueThroughPublicApi() == 30);
    check("private mode keeps base members usable only inside derived class",
          priv.accessibleTotal() == 30 && priv.exposedTag() == 2);
    check("protected mode keeps access for descendants",
          prot.accessibleTotal() == 30 && prot.exposedPrivateValue() == 30 &&
              grand.grandchildAccessibleTotal() == 30);
    const std::string report = inheritanceAccessReport();
    check("inheritance report names all three modes",
          report.find("public") != std::string::npos &&
              report.find("private") != std::string::npos &&
              report.find("protected") != std::string::npos &&
              report.find("grandchild") != std::string::npos);
}

void test16_aggregation() {
    std::cout << "\n[16] Aggregation: non-owning DepotRegistry (supporting demo)\n";
    using ReliefOps::Demos::DepotRegistry;
    using ReliefOps::Demos::aggregationExplanation;

    // Depots are created independently and outlive the registry below.
    DistributionDepot first("A1", "North", "Sector 1");
    DistributionDepot second("A2", "South", "Sector 2");
    second.addCargo(ReliefCargo("B1", "Water", SupplyCategory::Water, 5, 1.0,
                                0.2, Priority::Urgent));
    {
        DepotRegistry registry;
        check("registering existing depots succeeds",
              registry.registerDepot(first) && registry.registerDepot(second));
        check("duplicate registration rejected", !registry.registerDepot(first));
        check("registry holds references, not copies", registry.size() == 2);
        check("access registered depot through registry",
              registry.find("A1") != nullptr &&
                  registry.find("A1")->location() == "Sector 1");
        check("const registry access works",
              static_cast<const DepotRegistry&>(registry).find("A2") != nullptr);
        registry.find("A2")->restock("B1", 3);
        check("mutation through non-owning pointer reaches original",
              second.cargoRef("B1").quantity() == 8);
        check("registered IDs are sorted",
              registry.registeredIds() == std::vector<std::string>({"A1", "A2"}));
        check("report states non-ownership",
              registry.report().find("non-owning") != std::string::npos);
        check("unregister leaves depot alive",
              registry.unregister("A1") && first.id() == "A1" &&
                  registry.size() == 1);
        registry.clear();
        check("clear removes references without deleting depots",
              registry.empty() && second.id() == "A2" &&
                  second.cargoCount() == 1);
    }
    check("depots exist independently of the registry",
          first.id() == "A1" && second.id() == "A2");
    check("explanation distinguishes aggregation from composition",
          aggregationExplanation().find("Composition:") != std::string::npos &&
              aggregationExplanation().find("Aggregation:") != std::string::npos);
}

void test17_language_features() {
    std::cout << "\n[17] Supporting language-feature examples\n";
    using ReliefOps::Demos::languageFeaturesReport;
    using ReliefOps::Demos::totalOf;
    using ReliefOps::Demos::Litres;

    const std::string report = languageFeaturesReport();
    check("array of objects exercised",
          report.find("3 cargo lines, total quantity 10") != std::string::npos);
    check("iterators exercised",
          report.find("iterators traversed: 3") != std::string::npos);
    check("allocator exercised",
          report.find("allocator constructed 2") != std::string::npos);
    check("explicit and implicit conversions exercised",
          report.find("Litres(20) -> 20") != std::string::npos);
    check("friend function exercised",
          report.find("sameVolume(50, 50): true") != std::string::npos);
    check("friend function reads private data correctly",
          ReliefOps::Demos::sameVolume(Litres(12.5), Litres(12.5)) &&
              !ReliefOps::Demos::sameVolume(Litres(12.5), Litres(13.0)));

    // Generic function template instantiated with a different element type.
    const std::vector<int> numbers{2, 4, 6};
    check("generic function template works with another type",
          std::fabs(totalOf(numbers, [](int n) { return n; }) - 12.0) < 1e-9);
}

void test18_parameter_polymorphism() {
    std::cout << "\n[18] Polymorphism by parameter + parametric polymorphism\n";
    resetGlobals();
    ReliefHub volumeHub;
    seedHub(volumeHub);
    const AllocationResult volumeResult = volumeHub.allocateVolume(
        "D1", "V1", "C1", 1.0, "Depot A", Priority::Routine);
    check("double parameter selects the volume overload",
          volumeResult.accepted && volumeResult.request.quantity == 2 &&
              std::fabs(volumeResult.request.weightKg - 20.0) < 1e-9);

    ReliefHub countHub;
    seedHub(countHub);
    const AllocationResult countResult = countHub.allocateQuantity(
        "D1", "V1", "C1", 2, "Depot B", Priority::Critical, true);
    check("int + bool parameters select the emergency overload",
          countResult.accepted && countResult.request.quantity == 2 &&
              std::fabs(countResult.request.weightKg - 20.0) < 1e-9);
    check("emergency overload applies triage priority",
          countResult.request.priority == Priority::MedicalEmergency &&
              countResult.request.triageApproved);
    check("overloaded calls remain distinct",
          volumeResult.request.priority == Priority::Routine &&
              volumeResult.request.volumeM3 == 1.0);

    // Parametric polymorphism: the generic class template stores any T and the
    // member template accepts any comparator, independently of DispatchRequest.
    PriorityDispatchQueue<int> numbers;
    numbers.push(3);
    numbers.push(1);
    numbers.push(2);
    const std::vector<int> sorted =
        numbers.sortedSnapshot([](int a, int b) { return a < b; });
    check("generic class template instantiated with int",
          numbers.size() == 3 && sorted == std::vector<int>({1, 2, 3}));
}

void test19_registration_and_fuel_boundaries() {
    std::cout << "\n[19] Registration, lookup and refuel boundary cases\n";
    resetGlobals();
    const std::string path = "tests/test_outputs/refuel_bounds_test.dat";
    ReliefHub hub(path);

    // Application registries: ReliefHub owns by value/unique_ptr and looks up
    // through its indexes; there is no second owner or duplicate store.
    check("depot registration receipt names ID",
          hub.addDepot("BX", "Boundary Depot", "Sector X")
                  .find("Depot BX registered") != std::string::npos);
    check("depot lookup by ID returns the registered depot",
          hub.depotRef("BX").location() == "Sector X");
    check("duplicate depot ID rejected", throws<std::invalid_argument>([&] {
              hub.addDepot("BX", "Duplicate", "Sector Y");
          }));
    check("unknown depot lookup throws",
          throws<std::out_of_range>([&] { hub.depotRef("NO-DEPOT"); }));

    const VehicleSpec exactSpec = specFor(VehicleKind::Ground, "EXACT", 150.0);
    check("vehicle registration receipt names ID",
          hub.registerVehicle(exactSpec).find("EXACT") != std::string::npos);
    check("vehicle lookup by ID returns the registered vehicle",
          hub.vehicleRef("EXACT").id() == "EXACT");
    check("duplicate vehicle ID rejected", throws<std::invalid_argument>([&] {
              hub.registerVehicle(exactSpec);
          }));
    check("unknown vehicle lookup throws",
          throws<std::out_of_range>([&] { hub.vehicleRef("NO-VEHICLE"); }));

    // Direct vehicle-level semantics using the exact figures from the brief:
    // 100 L tank, 80 L current, 50 L requested -> 20 L added, final 100 L.
    GroundConvoy overfill("OVER", 100.0, 80.0, 1000.0,
                          VehicleStatus::Available, "Overfill");
    check("over-capacity request adds only the 20 L that fits",
          nearlyEqual(overfill.refuel(50.0), 20.0) &&
              nearlyEqual(overfill.currentFuel(), 100.0));
    GroundConvoy exact("EXACT-DIRECT", 100.0, 50.0, 1000.0,
                       VehicleStatus::Available, "Exact");
    check("exact-capacity request is added in full",
          nearlyEqual(exact.refuel(50.0), 50.0) &&
              nearlyEqual(exact.currentFuel(), 100.0));
    GroundConvoy full("FULL", 100.0, 100.0, 1000.0,
                      VehicleStatus::Available, "Full");
    check("already-full tank accepts request and adds zero",
          nearlyEqual(full.refuel(25.0), 0.0) &&
              nearlyEqual(full.currentFuel(), 100.0));
    GroundConvoy invalid("INVALID", 100.0, 40.0, 1000.0,
                         VehicleStatus::Available, "Invalid");
    check("negative direct refuel rejected",
          throws<std::invalid_argument>([&] { invalid.refuel(-1.0); }));
    check("zero direct refuel rejected",
          throws<std::invalid_argument>([&] { invalid.refuel(0.0); }));
    check("rejected refuels leave fuel unchanged",
          nearlyEqual(invalid.currentFuel(), 40.0));

    // Hub-level receipts for the remaining boundaries, including persistence.
    const std::string exactReceipt = hub.refuel("EXACT", 50.0);
    check("exact-capacity receipt reports requested amount",
          exactReceipt.find("requested 50.000000 L") != std::string::npos);
    check("exact-capacity receipt reports 50 L actually added",
          exactReceipt.find("actually added 50.000000 L") != std::string::npos);
    check("exact-capacity receipt reports final fuel level",
          exactReceipt.find("final fuel 200.000000 L") != std::string::npos);

    hub.registerVehicle(specFor(VehicleKind::Ground, "FULL", 200.0));
    const std::string fullReceipt = hub.refuel("FULL", 25.0);
    check("full-tank receipt reports zero actually added",
          fullReceipt.find("actually added 0.000000 L") != std::string::npos);
    check("full-tank receipt reports unchanged final level",
          fullReceipt.find("final fuel 200.000000 L") != std::string::npos);

    hub.registerVehicle(specFor(VehicleKind::Ground, "TRIM", 180.0));
    const std::string trimReceipt = hub.refuel("TRIM", 50.0);
    check("over-capacity hub request trims to free space",
          trimReceipt.find("actually added 20.000000 L") != std::string::npos &&
              nearlyEqual(hub.vehicleRef("TRIM").currentFuel(), 200.0));

    ReliefHub reloaded(path);
    check("boundary refuels reload in a fresh hub", reloaded.load());
    check("exact-capacity level restored",
          nearlyEqual(reloaded.vehicleRef("EXACT").currentFuel(), 200.0));
    check("full-tank level restored",
          nearlyEqual(reloaded.vehicleRef("FULL").currentFuel(), 200.0));
    check("trimmed level restored",
          nearlyEqual(reloaded.vehicleRef("TRIM").currentFuel(), 200.0));

    // A rolled-back dispatch must remain retryable after a real refuel.
    ReliefHub retry("tests/test_outputs/refuel_rollback_test.dat");
    seedHub(retry, 0.5);
    retry.allocateQuantity("D1", "V1", "C1", 5, "Village East", Priority::Urgent,
                           false);
    const VehicleStatus statusBefore = retry.vehicleRef("V1").status();
    check("failed dispatch rolls back before refuel",
          throws<FuelDepletionException>([&] { retry.dispatchNext(true); }));
    check("rollback preserves status and destination",
          retry.vehicleRef("V1").status() == statusBefore &&
              retry.vehicleRef("V1").destination().empty());
    check("failed dispatch keeps request pending", retry.pendingCount() == 1);
    const std::string retryReceipt = retry.refuel("V1", 1000.0);
    check("refuel after rollback trims to capacity",
          retryReceipt.find("actually added 199.500000 L") != std::string::npos &&
              nearlyEqual(retry.vehicleRef("V1").currentFuel(), 200.0));
    check("retried dispatch succeeds after refuel",
          retry.dispatchNext(true).find("completed") != std::string::npos);
    check("retried dispatch consumes fuel without auto-refuel",
          retry.vehicleRef("V1").currentFuel() < 200.0);
}

}  // namespace

int main() {
    std::cout << "ERL-HUB automated test suite (C++17)\n";
    test01_inventory();
    test02_allocation();
    test03_dispatch();
    test04_overweight();
    test05_insufficient_fuel();
    test06_damaged_corridor();
    test07_duplicate_cargo();
    test08_cancelled_dispatch();
    test09_invalid_inputs();
    test10_refuelling();
    test11_persistence();
    test12_virtual_destructor();
    test13_deep_copy();
    test14_memory_safety();
    test15_inheritance_access();
    test16_aggregation();
    test17_language_features();
    test18_parameter_polymorphism();
    test19_registration_and_fuel_boundaries();
    std::cout << "\n========== SUMMARY ==========\n"
              << "  PASSED: " << passed << "\n"
              << "  FAILED: " << failed << '\n';
    return failed == 0 ? 0 : 1;
}
