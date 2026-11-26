# ERL-HUB — Master Requirement Checklist

Status legend: ✅ implemented and executed; ⚠️ implemented but runtime sanitizer
execution blocked by the environment (see `docs/TEST_REPORT.md`).

Scope legend for the final audit: **[APP]** = working application code;
**[DEMO]** = small supporting example in `ReliefOps::Demos`, compiled and tested
but not part of the domain model. Definitions, exact locations and examples for
every item are in `docs/SYLLABUS_MAPPING.md`.

## Stages

| Stage | Status | Evidence |
|---|---|---|
| 1 Architecture and planning | ✅ | `docs/ARCHITECTURE.md`, `docs/CLASS_DIAGRAM.md`, requirement-to-file map |
| 2 Project setup and class definitions | ✅ | full `include/` + `src/` tree created |
| 3 Unit I + Unit II | ✅ | tests 1–7, 13, 18 |
| 4 Unit III | ✅ | tests 12–17 (includes access modes, aggregation, arrays/iterators/allocators) |
| 5 Unit IV | ✅ | tests 4–6, 11, 17, 18; `src/FileManager.cpp` |
| 6 Application integration | ✅ | menu smoke, restart load, import-record, refuel-trim and refuel-bounds runs |
| 7 Verification and testing | ✅ / ⚠️ | 153/153 automated checks; ASan runtime unavailable, alternatives executed |
| 8 Documentation and viva | ✅ | README, six docs + checklist + 60-question viva guide |

## Mandatory features

- [x] Relief supplies: rations, medical, water, blankets — `Types.h::SupplyCategory`
- [x] Depots and inventories — `DistributionDepot`
- [x] Trucks, helicopters, amphibious boats — `GroundConvoy`, `AerialTransport`, `AmphibiousCraft`
- [x] Hybrid amphibious medical vehicle — `AmphibiousMedicalVehicle`
- [x] Allocation and dispatch — `Allocation.cpp`, `ReliefHub::dispatchNext`
- [x] Emergency priority management — `PriorityDispatchQueue<T>`
- [x] Fuel consumption and explicit refuelling — `ReliefVehicle`, menu 14
- [x] Dispatch history and persistence — `DispatchRecord`, `FileManager`
- [x] C++17, no external libraries — all files
- [x] Namespaces `ReliefOps::Logistics`, `ReliefOps::Inventory`
- [x] Overloaded `allocateSupply` (volume + emergency medical) **[APP]**
- [x] Call by reference, return by reference **[APP]**
- [x] Inline distance and payload utilities **[APP]**
- [x] Friend `IncidentCommander` with triage override **[APP]**
- [x] Friend functions (`operator<<`, `operator>>`) **[APP]**, plus minimal friend demo **[DEMO]**
- [x] `ReliefCargo`, `DistributionDepot`, `TransportVehicle` classes **[APP]**
- [x] Private data + const correctness **[APP]**
- [x] Static tonnage and operational-vehicle counters + static getters **[APP]**
- [x] Dynamic manifest, deep copy ctor, move ops, destructor **[APP]**
- [x] Operators `+=`, `-=`, `==`, `<<`, `>>` genuinely used **[APP]**
- [x] Abstract `ReliefVehicle` + three derived vehicles + hybrid **[APP]**
- [x] Virtual inheritance diamond resolved (one fuel tank verified) **[APP]**
- [x] **`public`, `protected` and `private` inheritance explicitly demonstrated** **[DEMO]** — `include/InheritanceAccessDemo.h`, `src/InheritanceAccessDemo.cpp`, test 15
- [x] **`GPSBeacon` and `EmergencyLifeSupportUnit` by composition** **[APP]**
- [x] **Aggregation explicitly distinguished from composition** **[DEMO]** — `include/DepotRegistry.h`, `src/DepotRegistry.cpp`, test 16
- [x] **Registry roles reconciled** — `ReliefHub` owns/indexes depots and the fleet; `DepotRegistry` is [DEMO] aggregation only; no redundant `FleetRegistry` (see `docs/ARCHITECTURE.md` §3c)
- [x] `std::vector<ReliefVehicle*>` dynamic dispatch, virtual destructor **[APP]**
- [x] Three custom exceptions derived from `std::exception` **[APP]**
- [x] Binary `fstream` persistence, `seekp()`, `seekg()` **[APP]**
- [x] Rollback for failed dispatch/refuel/save **[APP]**
- [x] `std::vector`, `std::unordered_map` **[APP]**
- [x] Generic class template `PriorityDispatchQueue<T>` (also instantiated with `int`) **[APP]**
- [x] Generic function templates (`writePod`/`readPod`, `sortedSnapshot<Compare>`) **[APP]**, plus `totalOf` **[DEMO]**
- [x] Iterators **[APP]** (`for_each`, `max_element`, `find_if`) and explicit iterator loop **[DEMO]**
- [x] Allocators **[DEMO]** — `std::allocator` + `allocator_traits` in `src/LanguageFeaturesDemo.cpp`
- [x] Arrays of objects **[DEMO]** — `ReliefCargo cargoArray[3]` in `src/LanguageFeaturesDemo.cpp`
- [x] Default function arguments **[APP]** — `Utilities.h`, `ReliefVehicle.h`, `Allocation.h`, `ReliefHub.h`
- [x] Explicit and implicit type conversions **[APP]** (`static_cast`, `explicit` constructors) and **[DEMO]** (`Litres`)
- [x] Function prototyping **[APP]** — declarations in headers, definitions in `src/`
- [x] Function overloading and overriding **[APP]** — `allocateSupply` overloads, virtual overrides
- [x] Polymorphism by parameter **[APP]** — overload/operator selection from argument types, test 18
- [x] Parametric polymorphism **[APP]** — generic queue and comparator, test 18
- [x] `std::sort`, `std::for_each`, priority functor **[APP]**
- [x] User-defined manipulators **[APP]**
- [x] All 14 menu options + exit **[APP]**
- [x] Cancelled dispatch has no inventory/fuel/statistics/history effect **[APP]**
- [x] Successful dispatch deducts exactly once, reduces fuel, updates status/history **[APP]**
- [x] No automatic refuel after dispatch **[APP]**
- [x] **Refuel trims to free capacity, reports requested/added/final fuel, persists, no overfill exception** **[APP]** — test 10 and test 19, `tests/test_inputs/refuel_trim.txt`, `tests/test_inputs/refuel_bounds.txt`
- [x] **Refuel boundaries: exact-fit, already-full (0 L), negative, zero** **[APP]** — test 19
- [x] **Registration/lookup, duplicate IDs, dispatch rollback/retry, save/load after registry decision** **[APP]** — tests 1, 5, 9, 10, 11, 19
- [x] Compile with `-std=c++17 -Wall -Wextra -pedantic` — warning-free
- [x] 19 automated test categories, 153 individual checks
- [x] Tests for inheritance access modes and aggregation (tests 15–16)
- [x] Memory safety stress test + debug STL run
- [⚠] AddressSanitizer / UBSan / Valgrind runtime — dependencies absent; instrumented compilation, strict warnings and debug STL executed and documented
- [x] Persistence verified across application restart
- [x] README, SYLLABUS_MAPPING, ARCHITECTURE, CLASS_DIAGRAM, TEST_REPORT, viva guide

## Verification note

No claim above rests on documentation alone: every checked item points to a
source file and, where applicable, a numbered test in `tests/test_hub.cpp` or a
captured UI run in `tests/test_outputs/`.
