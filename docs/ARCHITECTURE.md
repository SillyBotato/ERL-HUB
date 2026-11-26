# ERL-HUB — Architecture and Design (Stage 1)  

## 1. Problem framing

ERL-HUB is a console application that coordinates post-earthquake relief logistics. The
design separates four concerns:

| Concern | Owner | Files |
|---|---|---|
| Domain value objects and inventory | `ReliefOps::Inventory` | `Types.h`, `Utilities.h`, `ReliefCargo.*`, `DistributionDepot.*` |
| Fleet, routing and polymorphic dispatch | `ReliefOps::Logistics` | `ReliefVehicle.*`, `TransportVehicle.*`, `GroundConvoy.*`, `AerialTransport.*`, `AmphibiousCraft.*`, `AmphibiousMedicalVehicle.*`, `GPSBeacon.h`, `EmergencyLifeSupportUnit.h` |
| Allocation, emergency triage, records | `ReliefOps::Logistics` | `Allocation.*`, `IncidentCommander.*`, `PriorityDispatchQueue.h`, `Records.h`, `Manipulators.h`, `Exceptions.h` |
| Persistence and application service | `ReliefOps::Logistics` | `FileManager.*`, `ReliefHub.*` |
| User interface only | global `main` | `main.cpp` |

`main.cpp` contains no business rules. It reads input, calls `ReliefHub`, and prints the
returned strings/report streams.

## 2. Complete class hierarchy

```
ReliefVehicle (abstract, virtual destructor, virtual clone/kind/rate/evaluateRoute)
        ^  virtual inheritance
        |
TransportVehicle (static totals, model, virtual inheritance from ReliefVehicle)
        ^  virtual inheritance
        |\
        | \-- AerialTransport
        |  \-- GroundConvoy -----------\
        |                               > AmphibiousMedicalVehicle (diamond resolved)
        \----- AmphibiousCraft --------/
```

* `TransportVehicle : virtual public ReliefVehicle`
* `AerialTransport : public TransportVehicle`
* `GroundConvoy : virtual public TransportVehicle`
* `AmphibiousCraft : virtual public TransportVehicle`
* `AmphibiousMedicalVehicle : public AmphibiousCraft, public GroundConvoy`

Because both diamond branches inherit **virtually** from `TransportVehicle`, and
`TransportVehicle` inherits **virtually** from `ReliefVehicle`, there is exactly one
`TransportVehicle` sub-object and exactly one `ReliefVehicle` sub-object in
`AmphibiousMedicalVehicle`. Its most-derived constructor explicitly initialises both
virtual bases; the intermediate constructors' virtual-base initialisers are ignored
by C++ in that case (they still run their own bodies).

## 3. Containment (composition), not inheritance

* Every `ReliefVehicle` owns one `GPSBeacon` (by value).
* `AmphibiousMedicalVehicle` owns one `EmergencyLifeSupportUnit` (by value).
* Every `ReliefVehicle` owns a `TransportManifest` (by value). `TransportManifest`
  owns a dynamically allocated array of `ReliefCargo*` (explicit deep copy, rule of
  five). The manifest is a *value member*, but its internal buffer is dynamic — this
  is the syllabus-mandated dynamic-memory demonstration.
* `DistributionDepot` owns `std::vector<ReliefCargo>`.
* `ReliefHub` owns depots by value and the fleet through
  `std::vector<std::unique_ptr<ReliefVehicle>>`. A separate
  `std::vector<ReliefVehicle*>` (`fleetView()`) is returned as a **non-owning**
  polymorphic view; it is used for dynamic-dispatch demonstrations. No raw pointer
  in this design owns memory.

### 3b. Supporting demonstrations (outside the domain model)

To make syllabus mechanics explicit without changing the vehicle hierarchy, three
small demonstration units live in namespace `ReliefOps::Demos`:

* `InheritanceAccessDemo` (`include/InheritanceAccessDemo.h`,
  `src/InheritanceAccessDemo.cpp`) — the same base-type members observed under
  public, private and protected inheritance, with compile-time convertibility
  assertions and a grandchild proving protected access survives further derivation.
* `DepotRegistry` (`include/DepotRegistry.h`, `src/DepotRegistry.cpp`) —
  **aggregation**: a vector of non-owning `DistributionDepot*`. Depots are created,
  used and destroyed independently; the registry only observes them and offers
  `unregister()`/`clear()` so references can be dropped before object destruction.
  This contrasts with the composition rules above.
* `LanguageFeaturesDemo` (`include/LanguageFeaturesDemo.h`,
  `src/LanguageFeaturesDemo.cpp`) — a generic function template, an array of
  `ReliefCargo` objects, explicit iterator traversal, `std::allocator` usage with
  exception-safe cleanup, explicit/implicit conversions and a friend function.

They are linked into both the application and the test binary through the
`src/*.cpp` build wildcard, but no domain class inherits from them and no
application behaviour depends on them.

### 3c. Registry decision (final audit)

No separate `FleetRegistry` is introduced. `ReliefHub` is the single owner of
`fleet_` and already maintains `vehicleIndex_` for direct lookup; a separate
fleet registry would either duplicate that ownership or add a redundant
pass-through layer. Depots follow the same pattern: `depots_` owns them by value
and `depotIndex_` provides lookup. `DepotRegistry` is retained only as the
syllabus aggregation demonstration and is deliberately **not** used by
`ReliefHub`. Therefore the application has no duplicated data and no second
owner; the demo has no ownership at all.

## 4. Ownership and lifetime rules

1. `ReliefHub` is the single owner of all fleet objects.
2. `clone()` returns `std::unique_ptr<ReliefVehicle>` so polymorphic copies cannot leak.
3. `TransportManifest` allocates with `new[]` for the pointer array and `new` per
   `ReliefCargo`; its copy constructor/assignment perform element-wise deep copies,
   and its destructor deletes every element and the array.
4. `FileManager` never serialises object addresses or compiler-dependent layouts. It
   writes an explicit field-by-field binary format using fixed-width integers and
   length-prefixed strings.
5. All allocation/dispatch steps validate **before** mutating. Dispatch additionally
   captures a rollback snapshot; any exception restores depot quantity, vehicle
   fuel/status/destination, statistics, history size and audit size.

## 5. Data flow

### Allocate (menu 7)
`main → ReliefHub::allocateQuantity/allocateVolume → Allocation::allocateSupply
(overload 1 or 2) → validate depot cargo + payload → build DispatchRequest by
reference → PriorityDispatchQueue::push → return receipt`. Inventory, fuel and
statistics are untouched at this stage.

### Dispatch (menu 8)
`main → ReliefHub::previewNextDispatch → queue.peek() + fleet vehicle lookup +
ReliefVehicle::evaluateRoute (virtual) → preview printed → user confirms/cancels`.
On confirm: `ReliefHub::dispatchNext(true) → rollback snapshot → depot.deduct →
vehicle.consumeFuel → status/destination update → static tonnage update → history +
audit append → queue.pop`. On failure: rollback and error message. On cancel: only
the request is popped.

### Persist (menu 11/12/14)
`ReliefHub::snapshot() → HubSnapshot (plain records) → FileManager::save()` writes a
temporary file, seeks back to patch the record count, verifies the temporary file by
reading it, then atomically replaces the previous file with a backup/restore path.
`FileManager::load()` restores the snapshot and `ReliefHub::restore()` reconstructs
depots and polymorphic vehicles via a record-driven factory.

## 6. Binary file layout (`data/erlhub.dat`)

```
offset 0  : char[8]  magic "ERLHUB01"
offset 8  : uint32   version (1)
offset 12 : uint64   section count (placeholder patched with seekp)
offset 20 : repeated sections { uint8 id, uint64 payloadSize, payload bytes }
            id 1 depots, 2 vehicles, 3 history, 4 pending, 5 audits, 6 meta
```

Payload strings are `uint32 length + raw bytes`; doubles are stored as 8-byte IEEE-754
values on the same platform. Unknown section IDs are skipped with `seekg()`.

## 7. Requirement-to-file map

| Requirement | Implementation |
|---|---|
| Namespaces | `include/Types.h`, all headers |
| Overloaded `allocateSupply` | `include/Allocation.h`, `src/Allocation.cpp` |
| Call by reference / return by reference | `src/Allocation.cpp`, `include/DistributionDepot.h`, `include/ReliefHub.h` |
| Inline utilities | `include/Utilities.h` |
| Friend `IncidentCommander` | `include/IncidentCommander.h`, `src/IncidentCommander.cpp` |
| Static totals | `include/TransportVehicle.h`, `src/TransportVehicle.cpp` |
| Dynamic manifest + deep copy | `include/ReliefCargo.h`, `src/ReliefCargo.cpp` |
| Operators `+= -= == << >>` | `src/ReliefCargo.cpp` |
| Virtual inheritance / diamond | `include/AmphibiousMedicalVehicle.h`, `src/AmphibiousMedicalVehicle.cpp` |
| Polymorphic `vector<ReliefVehicle*>` | `include/ReliefHub.h`, `src/ReliefHub.cpp` |
| Exceptions | `include/Exceptions.h` |
| Binary I/O, seekp/seekg, rollback | `src/FileManager.cpp`, `src/ReliefHub.cpp` |
| Templates, sort, functors | `include/PriorityDispatchQueue.h`, `src/ReliefHub.cpp`, `src/FileManager.cpp` |
| Manipulators | `include/Manipulators.h` |
| Public/protected/private inheritance | `include/InheritanceAccessDemo.h`, `src/InheritanceAccessDemo.cpp` **[DEMO]** |
| Aggregation vs composition | `include/DepotRegistry.h`, `src/DepotRegistry.cpp` **[DEMO]**; composition in `include/ReliefVehicle.h`, `include/DistributionDepot.h` **[APP]** |
| Arrays, iterators, allocators, conversions, friend function | `include/LanguageFeaturesDemo.h`, `src/LanguageFeaturesDemo.cpp` **[DEMO]** |
| Refuel trimming + receipt | `src/ReliefVehicle.cpp`, `src/ReliefHub.cpp`, `main.cpp` **[APP]** |
| 14-option UI | `main.cpp`, `src/ReliefHub.cpp` |

Full definitions, locations and examples for every syllabus topic are in
`docs/SYLLABUS_MAPPING.md`.

## 8. Known design decisions

* "Operational vehicles" is defined as all registered vehicles except `Retired`; the
  status enum includes `Retired` for completeness and the counter tracks construction
  and destruction of live vehicles.
* Refuelling accepts any positive request and adds `min(request, freeSpace)`. An
  over-capacity request is trimmed; an exact-fit request is added in full; a full
  tank adds `0 L`. Zero and negative requests remain invalid.
* Registry responsibilities are resolved in §3c: `ReliefHub` owns and indexes the
  real fleet/depots, `DepotRegistry` is an aggregation demo only, and there is no
  redundant `FleetRegistry`.
* Fuel rate is a virtual property of each vehicle type (L/km).
* A `Damaged` corridor throws `DamagedCorridorException` in route evaluation unless
  the `IncidentCommander` has set the private `corridorOverride_` flag via friendship.
* Allocation creates a *pending request* only; deduction happens exactly once at
  confirmed dispatch. This is what makes cancellation side-effect free.

## 9. Memory-risk checklist

| Risk | Mitigation |
|---|---|
| Shallow manifest copy → double delete | Rule-of-five deep copy with element-wise `new ReliefCargo(*other)` |
| Leak on partially built manifest | `ensureCapacity` throws only before mutation; destructor frees all constructed lines |
| Dangling fleet pointers | `unique_ptr` ownership; `fleetView()` rebuilt on demand and never stored |
| Diamond ambiguity | Virtual base initialisation in most-derived constructor; disambiguated overrides |
| Non-virtual base destruction | `ReliefVehicle::~ReliefVehicle()` is virtual; derived destructors verified in tests |
| File corruption | Temporary file + verification + backup/restore replacement |
