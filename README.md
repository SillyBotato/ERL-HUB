# ERL-HUB — Emergency Relief Logistics Hub

A complete C++17 console application that simulates post-earthquake, multi-agency
emergency relief logistics. It manages relief cargo, distribution depots, a
heterogeneous transport fleet, allocation requests, priority dispatch, fuel and
refuelling, dispatch history, audits, and binary persistence.

The project is deliberately built as an academic demonstration of the full C++
syllabus: namespaces, call semantics, inline functions, friendship, operator
overloading, deep copy/dynamic memory, static members, inheritance (including
public/protected/private modes and a virtual-inheritance diamond), composition
and aggregation, runtime polymorphism, custom exceptions, templates, functors,
iterators/allocators, arrays of objects, STL algorithms, stream manipulators,
explicit/implicit conversions, and binary file handling with transactional
updates. See `docs/SYLLABUS_MAPPING.md` for the exact source location of every
topic.

Concepts that would otherwise be only implicit (inheritance access modes,
aggregation, raw arrays, allocators, explicit/implicit conversions, friend
functions) have small **supporting demonstrations** in `ReliefOps::Demos`. They
are compiled and tested but do not change the application's vehicle hierarchy or
domain behaviour.

## Features

- Four supply categories: ration kits, medical supplies, clean water, thermal blankets.
- Multiple distribution depots with duplicate-cargo detection and restocking via `+=`.
- Four vehicle types: `GroundConvoy`, `AerialTransport`, `AmphibiousCraft`, and the
  hybrid `AmphibiousMedicalVehicle` (diamond resolved with virtual bases).
- Two genuinely overloaded `allocateSupply()` functions: volume-based and
  emergency-medical-priority based.
- `PriorityDispatchQueue<T>` with a custom priority functor and `std::sort`.
- Dispatch preview with explicit confirmation/cancellation. Cancellation has no
  side effects; dispatch deducts stock exactly once and never auto-refuels.
- Rollback on failed dispatch or failed persisted refuel: no partial state changes.
- Refuelling trims the request to the tank's free space (no overfill exception) and
  reports the requested amount, the amount actually added and the final level.
  Exact-capacity requests are added in full; an already-full tank accepts the
  request but adds 0 L. Zero and negative amounts remain invalid.
- Custom `IncidentCommander` friend class for emergency refuel, damaged-corridor
  override, triage override and emergency restock.
- Field-by-field binary persistence with magic header, `seekp()`/`seekg()`, and
  temporary-file + verification + backup replacement.
- Tabular reporting through user-defined stream manipulators.

## Compilation

Requirements: a C++17 compiler (tested with g++ 15.2.0 / MinGW-w64). No external
libraries.

```bash
# With GNU make (mingw32-make on Windows):
mingw32-make all

# Equivalent manual build from the project root:
g++ -std=c++17 -Wall -Wextra -pedantic -Iinclude main.cpp src/*.cpp -o bin/erlhub

# Build + run the automated tests:
mingw32-make tests
mingw32-make run-tests
```

Quality builds:

```bash
mingw32-make asan          # AddressSanitizer build (needs sanitizer runtime installed)
mingw32-make asan-tests
g++ -std=c++17 -Wall -Wextra -pedantic -D_GLIBCXX_DEBUG -D_GLIBCXX_ASSERTIONS \
    -Iinclude tests/test_hub.cpp src/*.cpp -o bin/tests_debugstl.exe   # portable alternative
```

> Note: the MinGW toolchain in the development environment did not ship
> `libasan`/`libubsan`, so sanitizer *linking* could not be executed there.
> Instrumented compilation, a debug-STL run, strict warnings and a 500-object
> stress test were executed instead. See `docs/TEST_REPORT.md`.

## Execution

```bash
bin/erlhub.exe       # Windows
./bin/erlhub         # Linux/macOS
```

The application starts empty. Use option **12** at any time to restore a previous
save from `data/erlhub.dat` (created by option 11).

## Menu

| # | Option | What it does |
|---|---|---|
| 1 | Add New Relief Cargo | Adds a cargo line (ID, name, quantity, unit weight/volume, priority) to a depot; can also import it from a text record via `operator>>` |
| 2 | View Available Inventory | Tabular depot inventory through the manifest manipulators |
| 3 | Register Distribution Depot | Creates a depot with ID, name and location |
| 4 | Restock Existing Cargo | Uses `ReliefCargo::operator+=` through `DistributionDepot::restock` |
| 5 | Register Transport Vehicle | Registers any of the four vehicle types with type-specific fields |
| 6 | View Registered Vehicles | Fleet table plus polymorphic route evaluation via `vector<ReliefVehicle*>` |
| 7 | Allocate Supplies to Vehicle | Depot + vehicle + cargo + destination; volume-based or emergency mode |
| 8 | Dispatch Emergency Vehicle | Previews the top priority request, asks for confirmation, then dispatches |
| 9 | View Emergency Priority Queue | Requests sorted by the custom priority functor using `std::sort` |
| 10 | View Dispatch History | Completed dispatches plus manifest/operation audit records |
| 11 | Save Application Data | Transactional binary save |
| 12 | Load Previously Saved Data | Restores depots, fleet, fuel, requests, history, audits and statistics |
| 13 | Display Logistics Statistics | Operational vehicles, tonnage, cargo, pending requests, availability |
| 14 | Refuel Vehicle | Explicit top-up; excess is trimmed to free tank space (0 L added if already full), receipt shows requested/added/final fuel, and the new level is persisted |
| 0 | Exit | Shuts down cleanly |

## Project structure

```
ERL-HUB/
├── main.cpp                  UI only
├── Makefile                  build, test and quality targets
├── README.md                 this file
├── include/                  all headers (Types, Utilities, Exceptions, Manipulators,
│                             ReliefCargo, DistributionDepot, ReliefVehicle,
│                             TransportVehicle, derived vehicles, components,
│                             PriorityDispatchQueue, IncidentCommander, Allocation,
│                             Records, FileManager, ReliefHub, plus the Demos headers)
├── src/                      implementations (including DepotRegistry.cpp,
│                             InheritanceAccessDemo.cpp, LanguageFeaturesDemo.cpp)
├── data/.gitkeep             default binary data directory
├── tests/
│   ├── test_hub.cpp          19-scenario automated harness (153 checks)
│   ├── test_inputs/          scripted menu input files (smoke, restart, import,
│   │                         refuel trim, refuel bounds)
│   ├── test_outputs/         captured reports and binary persistence fixtures
│   └── README.md
└── docs/                     ARCHITECTURE, CLASS_DIAGRAM, SYLLABUS_MAPPING,
                              REQUIREMENT_CHECKLIST, TEST_REPORT, VIVA_PREPARATION
```

`TransportVehicle.h/.cpp`, `Records.cpp` and the extra docs were added because the
static-member layer and plain record types need their own translation units; this
is permitted by the brief.

## Design highlights

- `ReliefHub` is the only owner of domain objects; raw `ReliefVehicle*` values are
  non-owning observers returned by `fleetView()`.
- No separate `FleetRegistry` is needed: `ReliefHub` already combines
  `unique_ptr` fleet ownership with `vehicleIndex_` lookup. Depots are owned by
  value with `depotIndex_`. `DepotRegistry` is a supporting aggregation example
  only, never a second application owner.
- `TransportManifest` explicitly allocates `ReliefCargo**` and implements the rule
  of five; its copy constructor/assignment perform deep copies.
- Dispatch validates everything before mutating, captures a rollback snapshot, and
  restores depot stock, vehicle fuel/status/destination, statistics, history and
  audits if any step throws.
- Binary files contain no raw pointers or object layouts: every field is written
  explicitly with fixed-width integers and length-prefixed strings.

## Supporting syllabus demonstrations

These live under namespace `ReliefOps::Demos`; none of them modifies the existing
vehicle hierarchy.

| Demo | Files | What it shows |
|---|---|---|
| Inheritance access modes | `include/InheritanceAccessDemo.h`, `src/InheritanceAccessDemo.cpp` | One base with public/protected/private members inherited publicly, privately and protectively; compile-time `static_assert`s verify the upcasting rules and a grandchild verifies protected access. |
| Aggregation | `include/DepotRegistry.h`, `src/DepotRegistry.cpp` | A registry of **non-owning** `DistributionDepot*` pointers. Depots are created and destroyed independently; the registry never deletes them. Contrast with `DistributionDepot`'s owned `vector<ReliefCargo>` (composition). |
| Language features | `include/LanguageFeaturesDemo.h`, `src/LanguageFeaturesDemo.cpp` | Generic function template, raw array of objects, explicit iterator traversal, `std::allocator` with `allocator_traits`, explicit/implicit conversions, and a friend function. |

Rule for avoiding dangling pointers in the aggregation demo: register only
depots that are already alive, call `unregister()`/`clear()` before those depots
are destroyed, and never delete through a registry pointer. The test declares
the depots outside the registry's scope and verifies they survive it.

See `docs/ARCHITECTURE.md` and `docs/CLASS_DIAGRAM.md` for the full design, and
`docs/SYLLABUS_MAPPING.md` for a per-topic definition/location/example index.
