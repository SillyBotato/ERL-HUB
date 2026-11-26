# ERL-HUB — CIC-211 Syllabus Coverage Map

Every topic below states **what it means**, **where it lives in the source**, and
**a concrete example/verification**.

Scope legend:

- **[APP]** — implemented in the working ERL-HUB application itself.
- **[DEMO]** — a small supporting example added solely to make a syllabus
  concept explicit; it is compiled and tested but is not part of the relief
  logistics domain model. The vehicle hierarchy is unchanged by these demos.

All items were compiled with `-std=c++17 -Wall -Wextra -pedantic` and exercised
by `tests/test_hub.cpp` (153 checks) or by the scripted UI runs listed in
`docs/TEST_REPORT.md`.

---

## UNIT I — Core Foundations and Call Semantics

| Topic / scope | Definition | Exact location | Example / verification |
|---|---|---|---|
| Namespaces **[APP]** | Named scopes that prevent name collisions. | `include/Types.h` and every header | `ReliefOps::Inventory::{ReliefCargo, DistributionDepot, Priority}`; `ReliefOps::Logistics::{ReliefVehicle, ReliefHub, allocateSupply}`. |
| Overloaded `allocateSupply()` – volume **[APP]** | Same function name, different parameter list; compiler picks by argument types. | `include/Allocation.h:30`, `src/Allocation.cpp:94` | `allocateSupply(depot, vehicle, "C1", 1.0 /*double m3*/, ...)` → 2 units × 10 kg = 20 kg. Test 18. |
| Overloaded `allocateSupply()` – emergency **[APP]** | Second overload with `int quantity` + `bool medicalTriage`. | `include/Allocation.h:42`, `src/Allocation.cpp:134` | `allocateSupply(..., 2, ..., true, ..., &commander)` forces `Priority::MedicalEmergency`. Test 18. |
| Call by reference (inventory) **[APP]** | Callee receives the original object, not a copy, so changes are visible to the caller. | `src/DistributionDepot.cpp` (`restock`, `deduct`), `src/ReliefCargo.cpp` (`operator+=`/`-=`) | `restock("C1", 25)` → `cargoRef("C1").quantity() == 125`. Test 1. |
| Call by reference (mission) **[APP]** | A reference parameter is mutated in place. | `src/Allocation.cpp:17` `configureMissionParameters(DispatchRequest&, ...)` | Sets destination, priority, weight, volume, corridor on the caller's request. Test 2. |
| Return by reference **[APP]** | Function returns an alias to a live object, avoiding a copy. | `include/DistributionDepot.h:34`, `include/ReliefVehicle.h:50`, `include/PriorityDispatchQueue.h:45`, `include/ReliefHub.h:102` | `cargoRef`, `manifestRef`, `beaconRef`, `peek`, `depotRef`, `vehicleRef`, `historyRef`. |
| Inline functions **[APP]** | Compiler may substitute the body at the call site; definition must be visible in every TU (ODR). | `include/Utilities.h:15–44`, `include/Types.h:24–60,94–121` | `payloadKg(3, 2.5) == 7.5`; `distanceKm("A","B")` deterministic. Test 14. |
| Friend class **[APP]** | A class granted access to another class's private members; one-way and not inherited. | `include/IncidentCommander.h`, `src/IncidentCommander.cpp`, friend declarations at `include/ReliefVehicle.h:86`, `include/Records.h:34`, `include/DistributionDepot.h:50` | `applyTriage()` writes private `DispatchRequest::priority`; `authorizeDamagedCorridor()` writes private `corridorOverride_`. Tests 6, 11. |
| Friend function **[APP]** | A non-member function with private access. | `friend std::ostream& operator<<(...)` / `operator>>(...)` in `include/ReliefCargo.h:31–32,78–79`, `include/DistributionDepot.h:42`, `include/GPSBeacon.h:26`, `include/EmergencyLifeSupportUnit.h:27`; friend `operator<<` in `include/Manipulators.h:63,79` | `std::cin >> cargo` (menu option 1) and `std::cout << manifest` (reports/tests). |
| Friend function **[DEMO]** | Minimal standalone illustration of a non-member reading a private field. | `include/LanguageFeaturesDemo.h:19–34`, `src/LanguageFeaturesDemo.cpp` | `sameVolume(Litres(50), Litres(50)) == true`. Test 17. |
| Default function arguments **[APP]** | A parameter gets a value when the caller omits it. | `include/Utilities.h:42` (`epsilon = 1e-9`), `include/ReliefVehicle.h:25` (`beacon = GPSBeacon()`), `include/Allocation.h:52` (`commander = nullptr`), `include/IncidentCommander.h:21`, `include/ReliefHub.h:60` | `nearlyEqual(a, b)` uses the default epsilon; `ReliefHub hub;` uses `data/erlhub.dat`. |

---

## UNIT II — Encapsulation, Dynamic Memory and Operator Overloading

| Topic / scope | Definition | Exact location | Example / verification |
|---|---|---|---|
| Class + data abstraction **[APP]** | Private data with a public, const-correct interface. | `include/ReliefCargo.h`, `include/DistributionDepot.h`, `include/TransportVehicle.h` | Private `stock_`, `lines_`, `currentFuel_`; getters return `const&`. |
| Static data members **[APP]** | One variable shared by all objects of the class. | `include/TransportVehicle.h`, `src/TransportVehicle.cpp:11` | `totalDispatchedTonnes()`, `totalOperationalVehicles()`, setters used by tests/dispatch. Tests 3, 5, 12, 14. |
| Dynamic memory / manifest **[APP]** | Heap-allocated storage managed manually. | `include/ReliefCargo.h` (`ReliefCargo** lines_`), `src/ReliefCargo.cpp` (`ensureCapacity`, `~TransportManifest`) | `new[]` pointer array + one `new ReliefCargo` per line; test 7. |
| Parameterised constructor **[APP]** | Constructor taking arguments to initialise state. | `src/ReliefCargo.cpp`, `src/TransportVehicle.cpp`, `src/ReliefHub.cpp` (`makeVehicleFromSpec`) | `ReliefCargo("C1", "Rations", SupplyCategory::Rations, 100, 10, 0.5, Priority::Urgent)`. |
| Deep copy + rule of five **[APP]** | Copy owns new memory, not the source's pointers. | `src/ReliefCargo.cpp` (copy/move ctors, copy/move assignment, destructor) | `TransportManifest copy(original)`; mutating `copy` leaves `original` unchanged. Test 13. |
| Destructor **[APP]** | Releases owned resources at end of lifetime. | `src/ReliefCargo.cpp`, `src/ReliefVehicle.cpp:28`, `src/TransportVehicle.cpp` | `~TransportManifest()` deletes each line and the pointer array; virtual `~ReliefVehicle()`. |
| `operator+=` **[APP]** | Overloaded addition-assignment with domain meaning. | `src/ReliefCargo.cpp`, `src/DistributionDepot.cpp`, `src/ReliefHub.cpp:4` | `ReliefCargo += int` restocks; `TransportManifest += ReliefCargo` adds or restocks a line. Tests 1, 7. |
| `operator-=` **[APP]** | Overloaded subtraction-assignment with domain meaning. | `src/ReliefCargo.cpp`, `src/DistributionDepot.cpp` | Deduct on dispatch; zero-quantity lines removed. Tests 3, 7. |
| `operator==` **[APP]** | Equality by identity (cargo ID) or by full value (manifest). | `src/ReliefCargo.cpp`, `src/DistributionDepot.cpp` | Duplicate detection; order-independent manifest compare with `std::sort`. Test 7. |
| `operator<<` / `operator>>` **[APP]** | Stream insertion/extraction overloads for display, serialisation and input. | all `operator<<` overloads; `src/ReliefCargo.cpp` (`>>`), `main.cpp:addCargo` | `std::cin >> cargo` imports `MED9 Trauma-kits 2 40 7.5 0.3 4 SectorZ` in menu option 1; round-trip in test 7. |
| Arrays of objects **[DEMO]** | Fixed-size sequence of actual objects, not pointers. | `src/LanguageFeaturesDemo.cpp:19–25` | `ReliefCargo cargoArray[3] = {...}`; total quantity 10. Test 17. (The app uses `vector<ReliefCargo>` in `DistributionDepot` and a dynamic pointer array in `TransportManifest`.) |

---

## UNIT III — Inheritance, Containership and Polymorphism

| Topic / scope | Definition | Exact location | Example / verification |
|---|---|---|---|
| Abstract base class **[APP]** | Class with ≥1 pure virtual function; cannot be instantiated. | `include/ReliefVehicle.h:21–36` | Pure virtual `kind`, `fuelRatePerKm`, `evaluateRoute`, `clone`. |
| Public single inheritance **[APP]** | `public` base members stay public, `protected` stay protected. | `include/AerialTransport.h`, `include/TransportVehicle.h` | `class AerialTransport : public TransportVehicle`; `class TransportVehicle : virtual public ReliefVehicle`. |
| Public inheritance **[DEMO]** | Outside code may upcast a derived pointer to the public base. | `include/InheritanceAccessDemo.h:38`, `src/InheritanceAccessDemo.cpp` | `std::is_convertible<PubliclyDerived*, AccessBase*>::value == true`; `pub.accessibleTotal() == 30`. Test 15. |
| Private inheritance **[DEMO]** | Public/protected base members become **private** in the derived class; no outside upcast. | `include/InheritanceAccessDemo.h:47`, `src/InheritanceAccessDemo.cpp` | `!std::is_convertible<PrivatelyDerived*, AccessBase*>::value`; only `accessibleTotal()` / `exposedTag()` expose values. Test 15. |
| Protected inheritance **[DEMO]** | Public/protected base members become **protected**; descendants retain access, outsiders do not. | `include/InheritanceAccessDemo.h:60`, `src/InheritanceAccessDemo.cpp` | `!std::is_convertible<ProtectedlyDerived*, AccessBase*>::value`; `PublicGrandchild::grandchildAccessibleTotal() == 30`. Test 15. |
| Access-rule summary **[DEMO]** | Base `private` is never directly reachable from derived code in any mode; base `public`/`protected` are reachable in derived code in all modes, but their visibility outside depends on the mode (public → public, protected → protected, private → private). | comments in `include/InheritanceAccessDemo.h:12–76` | `privateValueThroughPublicApi()` is the public gate to a private base field. Test 15. |
| Virtual inheritance / diamond **[APP]** | Shared base sub-object when a class inherits the same base through two paths. | `include/GroundConvoy.h`, `include/AmphibiousCraft.h`, `include/AmphibiousMedicalVehicle.h`, `src/AmphibiousMedicalVehicle.cpp` | Both branches `virtual public TransportVehicle`; one fuel tank verified via `ReliefVehicle*` and `TransportVehicle*` in test 13. |
| Containership (composition) **[APP]** | `has-a` by value; the whole owns the part's lifetime. | `include/ReliefVehicle.h:80–81`, `include/AmphibiousMedicalVehicle.h`, `include/DistributionDepot.h:48` | `GPSBeacon`, `EmergencyLifeSupportUnit`, `vector<ReliefCargo>` stored by value. |
| Aggregation **[DEMO]** | `has-a` by reference; parts exist independently and the whole does not own/delete them. | `include/DepotRegistry.h`, `src/DepotRegistry.cpp` | `DepotRegistry` stores `vector<DistributionDepot*>`; depots are created before it, survive `clear()` and the registry's destructor. Test 16. |
| Composition vs aggregation **[DEMO]** | Composition = ownership and matching lifetimes; aggregation = non-owning reference and independent lifetimes. | `aggregationExplanation()` in `src/DepotRegistry.cpp`; decision in `docs/ARCHITECTURE.md` §3c | `ReliefHub` owns vehicles via `unique_ptr` and depots by value (composition); `DepotRegistry` only points at depots (aggregation). No `FleetRegistry` is needed. Tests 16, 19. |
| `std::vector<ReliefVehicle*>` **[APP]** | Container of non-owning base pointers used for dynamic dispatch. | `include/ReliefHub.h:109`, `src/ReliefHub.cpp:534` | `fleetView()` + `displayRouteEvaluation()`; test 12. |
| Virtual functions / overriding **[APP]** | Derived class redefines a base virtual function; call chosen at runtime. | all four vehicle `.cpp` files (`kind`, `fuelRatePerKm`, `evaluateRoute`, `clone`, `printDetails`) | `vehicle->evaluateRoute(...)` through base pointers prints type-specific routes. Tests 12, UI option 6. |
| Virtual destructor **[APP]** | Ensures derived destructor runs when deleting via base pointer. | `include/ReliefVehicle.h:28` | Per-derived destroyed counters all increment; base count = 4. Test 12. |

---

## UNIT IV — Streams, Persistence, Exception Safety and STL

| Topic / scope | Definition | Exact location | Example / verification |
|---|---|---|---|
| Custom exceptions **[APP]** | User-defined exception types derived from `std::exception`. | `include/Exceptions.h`, `src/Allocation.cpp`, `src/ReliefVehicle.cpp`, `src/ReliefHub.cpp` | `CargoOverweightException` (test 4), `FuelDepletionException` (test 5), `DamagedCorridorException` (test 6). Refuelling overfill is **not** an exception: it is trimmed. |
| Binary `fstream` persistence **[APP]** | Field-by-field binary I/O; no object layout or pointer is written. | `include/FileManager.h`, `src/FileManager.cpp` | `writePod/readPod`, `writeString/readString`, six typed sections. Test 11, restart UI run. |
| `seekp()` / `seekg()` **[APP]** | Random-access stream positioning. | `src/FileManager.cpp` | `seekp(COUNT_OFFSET)` patches section count; `seekg(0)`, `seekg(COUNT_OFFSET)`, `seekg(HEADER_SIZE)` on load. |
| Transactional update / rollback **[APP]** | All-or-nothing mutation: snapshot, mutate, restore on failure. | `src/FileManager.cpp` (`.tmp` + verify + `.bak` + rename), `src/ReliefHub.cpp` (`dispatchNext`, `refuel`) | Test 5 (failed dispatch rollback), test 8 (cancel), test 10 (persisted refuel), test 19 (rollback preserves status/destination and the request stays retryable). |
| `std::vector` **[APP]** | Dynamic contiguous sequence container. | throughout | depot/fleet/history/audit/queue storage. |
| `std::unordered_map` **[APP]** | Hash-based key→value lookup (expected O(1)). | `include/ReliefHub.h:135–136`, `src/ReliefHub.cpp` | `depotIndex_`, `vehicleIndex_`. |
| Generic class template **[APP]** | A class parameterised by type, instantiated on demand. | `include/PriorityDispatchQueue.h:30–82` | `PriorityDispatchQueue<DispatchRequest> queue_` in `ReliefHub`; a second instantiation with `int` in test 18 stores and sorts numbers. |
| Generic function template **[APP]** | A function parameterised by type(s); the compiler generates each used version. | `src/FileManager.cpp:20–29` (`writePod<T>`, `readPod<T>`); `include/PriorityDispatchQueue.h:69` (`sortedSnapshot<Compare>`) | `writePod(os, uint32)` and `writePod(os, double)` in the same save path; `sortedSnapshot(functor)` in menu option 9. |
| Generic function template **[DEMO]** | Duck-typed template over a container + getter. | `include/LanguageFeaturesDemo.h:38–45` (`totalOf<Container, Getter>`), used in `src/LanguageFeaturesDemo.cpp` | `totalOf(cargoArray, getter)` → 10 and `totalOf(vector<int>, getter)` → 12. Test 17. |
| Iterators **[APP]** | Objects that traverse containers; used implicitly by range-for and STL algorithms. | `src/ReliefHub.cpp:437,455,485,522,534`, `include/PriorityDispatchQueue.h:49,61,72`, `src/DepotRegistry.cpp` (`find_if`) | `std::for_each(fleet_.begin(), fleet_.end(), ...)`; `std::max_element(items_.begin(), items_.end(), Compare{})`. |
| Iterators **[DEMO]** | Explicit `begin()`/`end()` loop over a raw array. | `src/LanguageFeaturesDemo.cpp:31–35` | Counts 3 elements; verified in test 17. |
| Allocators **[DEMO]** | Standard abstraction for raw memory allocation + object construction/destruction. | `src/LanguageFeaturesDemo.cpp:37–56` | `std::allocator<ReliefCargo>` + `allocator_traits::construct/destroy/deallocate`; report shows "allocator constructed 2". Test 17. (The app itself uses `new[]`/`new` in `TransportManifest`.) |
| `std::sort` **[APP]** | STL sort with a comparator. | `include/PriorityDispatchQueue.h:72`, `src/ReliefCargo.cpp` (`operator==`) | Priority-ordered queue display; order-independent manifest equality. |
| `std::for_each` **[APP]** | STL algorithm applying a functor/lambda to every element. | `src/ReliefHub.cpp:437,455,485,522,534` | Fleet, queue, history, audits, route evaluation displays. |
| Priority functor **[APP]** | Function object with `operator()` used as a comparator. | `include/PriorityDispatchQueue.h:16–28` | `DispatchPriorityComparator`: priority → triage → FIFO sequence. |
| Function overloading **[APP]** | Multiple functions with the same name but different signatures. | `include/Allocation.h:30,42`, `src/Allocation.cpp:94,134` | Volume overload vs emergency overload; test 18. |
| Polymorphism by parameter **[APP]** | Compile-time selection of the correct overload/operator from argument types. | same as above, plus `operator+=` overloads in `src/ReliefCargo.cpp` | `double` argument selects volume allocation; `int`+`bool` selects emergency allocation; observed different priority and volume. Test 18. |
| Parametric polymorphism **[APP]** | Writing code once over a type parameter; both the queue class and its comparator parameter are generic. | `include/PriorityDispatchQueue.h:30,69`; instantiated in `include/ReliefHub.h:137` and test 18 | The same queue code works for `DispatchRequest` and for `int` with a different comparator. |
| Explicit / implicit type conversion **[APP]** | Explicit requires a cast or `explicit` ctor; implicit happens automatically. | `src/Allocation.cpp:106` (`static_cast<int>`), `src/PriorityDispatchQueue.h:18–19` (`static_cast<int>` on enum), `src/FileManager.cpp` (`static_cast<uint32_t>`) | Volume `1.0 m3 / 0.5` explicitly converted to `int` units. |
| Explicit / implicit conversion **[DEMO]** | Minimal contrast in one class. | `include/LanguageFeaturesDemo.h:19–30` | `Litres(20.0)` explicit; `double d = litres;` implicit. Test 17. |
| Function prototyping **[APP]** | Declaration (prototype) before use; definition in a separate TU. | every header, e.g. `include/ReliefHub.h:63–120`, `include/Allocation.h:30–60`, `include/FileManager.h` | `main.cpp` includes `ReliefHub.h` and calls `hub.dispatchNext()` whose definition is in `src/ReliefHub.cpp`. |
| User-defined manipulators **[APP]** | Objects/functions inserted into a stream to change formatting. | `include/Manipulators.h` | `reportTitle("...")`, `manifestTableHeader/Footer`, `dispatchReportHeader/Footer`; tests 14, menu reports. |
| Refuelling with trimming **[APP]** | Requested fuel above free capacity is clipped to the free space; no overfill exception. An exact fit is added in full; a full tank adds 0 L; zero/negative remain invalid. | `include/ReliefVehicle.h:43`, `src/ReliefVehicle.cpp:42`, `src/ReliefHub.cpp:352`, `main.cpp:265` | Request 50 L with 20 L free → add 20 L, receipt shows requested 50, added 20, final 200. Exact-fit/full-tank/negative/zero boundaries in test 19 and `tests/test_inputs/refuel_bounds.txt`. |

---

## Application menu coverage

All 14 options are implemented in `main.cpp` and serviced by `ReliefHub` in
`src/ReliefHub.cpp`; option 0 exits cleanly. No placeholder or unimplemented
option exists.

- `tests/test_inputs/menu_smoke.txt` — registration, allocation, dispatch,
  statistics, refuel, save/load, reports.
- `tests/test_inputs/restart_load.txt` — restoration in a fresh process.
- `tests/test_inputs/import_record.txt` — `operator>>` used by the real UI.
- `tests/test_inputs/refuel_trim.txt` — 50 L requested, 20 L added, 200 L final.
- `tests/test_inputs/refuel_bounds.txt` — exact-fit request (150 + 50 → 200 L)
  and already-full request (200 + 25 → 200 L, 0 L added), then load/review.

The supporting demos are compiled into the application and test binary via the
`src/*.cpp` Makefile wildcard, but they do not alter the existing vehicle
hierarchy or domain behaviour.
