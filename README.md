# ERL-HUB — Emergency Relief Logistics Hub

**A C++17 console application for post-earthquake, multi-agency emergency relief logistics.**

ERL-HUB models the coordination of relief supplies, distribution depots and transport vehicles during an emergency. It provides a menu-driven workflow for recording cargo, maintaining inventory, registering vehicles, allocating supplies, prioritising dispatch requests, tracking fuel, viewing operational records and saving application state between runs.

> **Project type:** Academic C++ Object-Oriented Programming project  
> **Language standard:** C++17  
> **Build:** Makefile / MinGW-w64 g++  
> **External libraries:** None required

---

## 1. Problem Statement

After an earthquake, relief teams may need to move food, medical supplies, clean water and other essential goods from multiple depots to affected locations. Different vehicles have different capacities and travel capabilities, while road conditions, fuel availability and emergency priorities can change which delivery should be handled first.

If inventory and dispatch operations are managed inconsistently, the system may record duplicate cargo, allocate unavailable stock, exceed vehicle capacity or partially update records when a dispatch fails. ERL-HUB models these challenges in one application and applies validation and rollback rules to keep its simulated logistics state consistent.

## 2. Objectives

- Represent cargo, depots, vehicles, allocations and dispatch records using object-oriented classes.
- Maintain depot inventory and support cargo registration and restocking.
- Model ground, aerial, amphibious and hybrid amphibious-medical vehicles.
- Validate payload, inventory, fuel and route conditions before dispatch.
- Prioritise emergency requests and allow the operator to confirm or cancel a dispatch.
- Track fuel consumption and support explicit vehicle refuelling.
- Preserve application state through binary save/load operations.
- Demonstrate the major C++ OOP and programming concepts covered by the course syllabus.

## 3. Main Features

- Four supply categories: ration kits, medical supplies, clean water and thermal blankets.
- Multiple distribution depots with duplicate-cargo detection and restocking via `+=`.
- Four vehicle types: `GroundConvoy`, `AerialTransport`, `AmphibiousCraft` and the hybrid `AmphibiousMedicalVehicle`.
- A virtual-inheritance diamond for the amphibious medical vehicle.
- Overloaded `allocateSupply()` functions for standard and emergency-medical allocation.
- `PriorityDispatchQueue<T>` with a custom priority functor.
- Dispatch preview with explicit confirmation or cancellation.
- Validation of stock, payload, fuel and route conditions.
- Rollback when dispatch or persisted refuelling fails, preventing partial state changes.
- Explicit refuelling with automatic trimming to the tank's remaining capacity.
- Dispatch history, audit records and logistics statistics.
- Field-by-field binary persistence with temporary-file verification and backup replacement.
- Tabular output through user-defined stream manipulators.

### Refuelling rule

The amount actually added is limited to the free tank capacity:

```cpp
actualAdded = min(requestedAmount, tankCapacity - currentFuel);
```

For example, if a 100 L tank currently contains 80 L and the operator requests 50 L, the application adds 20 L and the final level becomes 100 L. An already-full tank adds 0 L. Zero and negative requests are invalid. The receipt reports the requested amount, actual amount added and final fuel level.

## 4. Application Workflow

1. Register one or more distribution depots.
2. Add relief cargo to a depot or restock an existing cargo line.
3. Register suitable transport vehicles.
4. Allocate cargo to a vehicle and destination. This creates a pending dispatch request.
5. Review the priority queue and preview the highest-priority request.
6. Confirm the dispatch or cancel it. Cancellation does not deduct inventory or fuel.
7. Review dispatch history and statistics.
8. Save the application state and load it in a later run when required.

The application starts with an empty in-memory state. Use menu option **11** to save and option **12** to load the saved state from `data/erlhub.dat`.

## 5. Menu Reference

| # | Option | Description |
|---|---|---|
| 1 | Add New Relief Cargo | Add a cargo line to a depot; supports text-record import through `operator>>`. |
| 2 | View Available Inventory | Display depot inventory in tabular form. |
| 3 | Register Distribution Depot | Create a depot with an ID, name and location. |
| 4 | Restock Existing Cargo | Restock an existing cargo line using `ReliefCargo::operator+=`. |
| 5 | Register Transport Vehicle | Register one of the four vehicle types. |
| 6 | View Registered Vehicles | Display fleet information and polymorphic route evaluation. |
| 7 | Allocate Supplies to Vehicle | Select depot, vehicle, cargo and destination; supports emergency mode. |
| 8 | Dispatch Emergency Vehicle | Preview the top-priority request and confirm or cancel dispatch. |
| 9 | View Emergency Priority Queue | Display pending requests in priority order. |
| 10 | View Dispatch History | Display completed dispatches and operation audit records. |
| 11 | Save Application Data | Save state using the transactional binary persistence layer. |
| 12 | Load Previously Saved Data | Restore depots, fleet, fuel, requests, history, audits and statistics. |
| 13 | Display Logistics Statistics | Display operational vehicles, tonnage, cargo, pending requests and availability. |
| 14 | Refuel Vehicle | Top up a vehicle; excess is trimmed to available tank space. |
| 0 | Exit | Exit the application. |

## 6. System Architecture

The application is organised around domain objects and a central application controller:

- **Inventory domain:** `ReliefCargo` and `DistributionDepot` represent cargo and depot stock.
- **Vehicle domain:** `ReliefVehicle` defines the common abstract vehicle interface. Specialised vehicle classes implement their transport behaviour.
- **Components:** `GPSBeacon`, `TransportManifest` and `EmergencyLifeSupportUnit` are composed into relevant domain objects.
- **Coordination:** `ReliefHub` coordinates registration, allocation, dispatch, reporting and persistence.
- **Dispatch:** `PriorityDispatchQueue<T>` stores pending requests and orders them using a custom priority functor.
- **Persistence:** `FileManager` and record types serialize and restore application state.

### Ownership and registries

`ReliefHub` is the sole application owner of the live domain objects. It owns depots by value and owns vehicles through `std::unique_ptr<ReliefVehicle>`. Identifier indexes (`depotIndex_` and `vehicleIndex_`) support lookup. Raw `ReliefVehicle*` values returned by `fleetView()` are non-owning observers.

There is no separate `FleetRegistry`: fleet ownership and lookup are already responsibilities of `ReliefHub`, so another owning registry would duplicate state. `DepotRegistry` is retained as a supporting aggregation demonstration only. It stores non-owning `DistributionDepot*` pointers and is not a second owner of the application's depots.

### Vehicle inheritance

`ReliefVehicle` is the abstract base class. `TransportVehicle` provides shared transport behaviour. `GroundConvoy` and `AmphibiousCraft` use virtual inheritance in the shared vehicle hierarchy. `AmphibiousMedicalVehicle` inherits from both and therefore demonstrates diamond inheritance with a single shared virtual base subobject. `AerialTransport` is another specialised vehicle type. Virtual functions provide runtime polymorphism, and the base class has a virtual destructor.

### Composition, aggregation and memory management

- Vehicles contain component objects such as a GPS beacon and transport manifest.
- The hybrid medical vehicle contains an emergency life-support unit.
- A depot owns its cargo collection.
- The `DepotRegistry` demonstration observes depots without owning or deleting them.
- `TransportManifest` manages a dynamic array of cargo pointers and implements the rule of five, including deep-copy operations.
- The live fleet uses smart-pointer ownership; raw pointers are used only as non-owning observers in the relevant interfaces/demonstration.

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) and [`docs/CLASS_DIAGRAM.md`](docs/CLASS_DIAGRAM.md) for the detailed design.

## 7. C++ Concepts Demonstrated

The project is designed to demonstrate the course's C++ OOP topics. The detailed source-by-source mapping is in [`docs/SYLLABUS_MAPPING.md`](docs/SYLLABUS_MAPPING.md).

| Concept | Where it appears |
|---|---|
| Classes, objects, encapsulation and abstraction | Domain classes with private state and public interfaces; abstract `ReliefVehicle`. |
| Constructors, destructors and copy control | Vehicle, cargo and manifest classes; `TransportManifest` rule of five. |
| Dynamic memory and deep copying | Dynamic cargo storage managed by `TransportManifest`. |
| Function overloading | Overloaded `allocateSupply()` operations. |
| Operator overloading | Cargo restocking, deduction, comparison and stream operators. |
| Friend functions/classes | Stream operations and controlled emergency access through `IncidentCommander`. |
| Inheritance and access modes | Vehicle hierarchy plus a supporting inheritance-access demonstration. |
| Virtual functions and runtime polymorphism | Abstract vehicle interface and derived overrides. |
| Virtual inheritance | Shared virtual base in the amphibious hybrid diamond. |
| Composition and aggregation | Vehicle components, depot cargo ownership and non-owning `DepotRegistry` demo. |
| Templates, STL and algorithms | `PriorityDispatchQueue<T>`, vectors, maps, iterators, algorithms and allocator demonstration. |
| Exceptions | Domain-specific failures and dispatch rollback. |
| File handling | Binary save/load and transactional file replacement. |
| Namespaces and stream manipulators | `ReliefOps` namespaces and custom tabular output manipulators. |

Some syllabus topics that are not central to the relief domain are shown in isolated supporting demonstrations under `ReliefOps::Demos`. These are compiled and tested but do not alter the application's vehicle hierarchy or domain behaviour.

## 8. Build and Run

### Requirements

- C++17-compatible compiler (the recorded build used MinGW-w64 g++ 15.2.0).
- GNU Make; on the recorded Windows environment, use `mingw32-make`.
- No external libraries are required.

### Build

From the project root:

```bash
mingw32-make clean
mingw32-make all
```

On systems where GNU Make is available as `make`, replace `mingw32-make` with `make`.

### Run the application

```bash
# Windows
bin\erlhub.exe

# Linux/macOS (if built with the platform's compiler)
./bin/erlhub
```

### Build and run automated tests

```bash
mingw32-make tests
mingw32-make run-tests
```

### Optional Debug-STL build

```bash
g++ -std=c++17 -Wall -Wextra -pedantic \
    -D_GLIBCXX_DEBUG -D_GLIBCXX_ASSERTIONS \
    -Iinclude tests/test_hub.cpp src/*.cpp \
    -o bin/tests_debugstl.exe

bin/tests_debugstl.exe
```

The Makefile also provides sanitizer targets. They require compatible AddressSanitizer/UndefinedBehaviorSanitizer runtimes to be installed and linkable in the toolchain.

## 9. Testing and Verification

The project's test report records the following results:

| Verification | Recorded result |
|---|---|
| Clean C++17 build with `-Wall -Wextra -pedantic` | Passed; no warnings reported. |
| Automated test harness | **153 passed, 0 failed** across 19 scenarios. |
| Five repeated test runs | Passed; 153/0 on each run. |
| Debug-STL build and run | Passed; 153 passed, 0 failed. |
| Additional strict warning checks | No diagnostics reported. |
| Scripted interactive runs | Passed: menu smoke, restart/load, record import, refuel trim and refuel boundaries. |
| Fresh-process persistence | Saved state restored successfully in a separate process. |
| Runtime ASan/UBSan | Not run: the recorded MinGW environment lacked `libasan`/`libubsan`. |
| Valgrind | Not run: Valgrind was not installed in the recorded environment. |

The test report also records successful instrumented compilation of the source files. This is not a sanitizer runtime result. For complete details, see [`docs/TEST_REPORT.md`](docs/TEST_REPORT.md) and [`tests/README.md`](tests/README.md).

## 10. Project Structure

```text
ERL-HUB/
├── main.cpp                  Console application entry point
├── Makefile                  Build, test and quality targets
├── README.md                 Project overview and usage guide
├── include/                  Class declarations and interfaces
├── src/                      Class implementations
├── data/                     Default persistence directory
├── tests/
│   ├── test_hub.cpp           Automated test harness
│   ├── test_inputs/           Scripted menu input files
│   ├── test_outputs/          Expected/captured outputs and test fixtures
│   └── README.md              Test instructions
└── docs/
    ├── ARCHITECTURE.md        System design and ownership
    ├── CLASS_DIAGRAM.md       Class relationships
    ├── REQUIREMENT_CHECKLIST.md Requirement coverage
    ├── SYLLABUS_MAPPING.md    C++ topic-to-source mapping
    ├── TEST_REPORT.md         Verification results
    └── VIVA_PREPARATION.md    Viva questions and explanations
```

## 11. Limitations and Future Work

- Binary persistence is host-endian and platform-family specific; a portable interchange format would be needed for cross-platform data exchange.
- Route distances are deterministic simulation values, not real GIS calculations or live route data.
- The project is a console-based academic simulation; it does not include a GUI, networking or integration with live emergency agencies.
- Runtime ASan/UBSan and Valgrind checks could not be completed in the recorded development environment because the required runtimes/tools were unavailable.
- Possible extensions include real map integration, configurable agency roles, richer operational reports and a portable persistence format.

## 12. Documentation

- [Architecture](docs/ARCHITECTURE.md)
- [Class Diagram](docs/CLASS_DIAGRAM.md)
- [Requirement Checklist](docs/REQUIREMENT_CHECKLIST.md)
- [C++ Syllabus Mapping](docs/SYLLABUS_MAPPING.md)
- [Test Report](docs/TEST_REPORT.md)
- [Viva Preparation](docs/VIVA_PREPARATION.md)

---

## Project Details

**Project title:** Emergency Relief Logistics Hub (ERL-HUB)  
**Course:** C++ Object-Oriented Programming  
**Team member(s):** Gargi Tyagi (02196402725), Bharti Choudhary (03296402725)
**Institution:** MAIT, Delhi
