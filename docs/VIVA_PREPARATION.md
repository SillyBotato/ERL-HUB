# ERL-HUB — Viva Preparation (60 questions with answers)

## How the major classes fit together (quick revision)

- **ReliefCargo** — one cargo line (ID, name, category, quantity, unit weight/volume,
  priority, destination). Its `+=`/`-=` change quantity, `==` compares by ID, and
  `<<`/`>>` provide textual I/O.
- **TransportManifest** — dynamically allocated array of `ReliefCargo*`; rule of five;
  `+=` adds/restocks a line, `-=` deducts, `==` compares manifests, `<<`/`>>` serialise.
- **DistributionDepot** — ID/name/location plus `vector<ReliefCargo>`; duplicate
  detection, restock, deduct by ID, `cargoRef()` returns a live reference.
- **ReliefVehicle** — abstract base; fuel/tank/manifest/beacon, pure virtual
  `kind/fuelRatePerKm/evaluateRoute/clone`, virtual destructor.
- **TransportVehicle** — virtual base carrying `model_` and the two static
  statistics; derived vehicles are `AerialTransport`, `GroundConvoy`,
  `AmphibiousCraft`; the hybrid is `AmphibiousMedicalVehicle`.
- **Allocation.cpp** — the two overloaded `allocateSupply()` functions and
  `configureMissionParameters()` (call by reference).
- **PriorityDispatchQueue<T>** — template queue; `DispatchPriorityComparator`
  functor; `std::sort` for display order.
- **IncidentCommander** — friend of vehicle, request and depot; emergency overrides.
- **FileManager** — binary, field-by-field save/load with `seekp`/`seekg` and
  transactional file replacement.
- **ReliefHub** — application service owning everything; all 14 menu operations;
  UI lives only in `main.cpp`.

Supporting demonstrations (namespace `ReliefOps::Demos`, **not** part of the
vehicle hierarchy; each is compiled and covered by tests 15–17):

- **InheritanceAccessDemo** — `include/InheritanceAccessDemo.h`,
  `src/InheritanceAccessDemo.cpp`: one base inherited publicly, privately and
  protectively, plus a grandchild test of protected access.
- **DepotRegistry** — `include/DepotRegistry.h`, `src/DepotRegistry.cpp`:
  non-owning pointers to independently created `DistributionDepot` objects
  (aggregation, not ownership).
- **LanguageFeaturesDemo** — `include/LanguageFeaturesDemo.h`,
  `src/LanguageFeaturesDemo.cpp`: generic function template, array of objects,
  iterators, allocator, explicit/implicit conversions and a friend function.

For the definition, exact source location and an example of **every** syllabus
topic, see `docs/SYLLABUS_MAPPING.md`.

## Questions and answers

### Namespaces and call semantics

**1. What are the two project namespaces and why were they chosen?**
`ReliefOps::Inventory` holds cargo and depot types; `ReliefOps::Logistics` holds
vehicles, dispatch, persistence and the hub. They prevent name collisions and
mirror the two responsibility areas of the domain.

**2. Show the two `allocateSupply()` overloads and explain the difference.**
One takes `double volumeM3` and converts it into units using the cargo's
`volumePerUnitM3()`; the other takes `int quantity` plus a `bool medicalTriage`
(and optional `IncidentCommander*`) and forces `Priority::MedicalEmergency` when
triage is requested. Their parameter lists differ, so overload resolution is
unambiguous.

**3. Where is call by reference used to modify an inventory?**
`DistributionDepot::restock()` obtains `ReliefCargo&` from `cargoRef()` and applies
`target += units`. `deduct()` uses the pointer/reference returned by `findCargo()`
and applies `-=`. Both mutate the depot's actual stock, not a copy.

**4. Where is call by reference used for mission parameters?**
`configureMissionParameters(DispatchRequest&, destination, priority, weight,
volume, corridor)` writes into the caller's request object; `allocateSupply()`
passes its local request by reference.

**5. Name five functions that return references.**
`DistributionDepot::cargoRef()`, `stockRef()`, `ReliefVehicle::manifestRef()`,
`beaconRef()`, `PriorityDispatchQueue::peek()`, `ReliefHub::depotRef()`,
`vehicleRef()`, `historyRef()`. They avoid copying and permit in-place updates.

**6. Why are the inline utilities in a header?**
`distanceKm`, `payloadKg`, `kilogramsToTonnes` are defined in `Utilities.h` with
`inline`, so every translation unit sees the definition (required by ODR) and the
compiler can inline the call. They are deterministic and side-effect free.

**7. What does the friend class do, and why is friendship needed?**
`IncidentCommander` must reach private fields: `ReliefVehicle::currentFuel_`,
`corridorOverride_`, `DispatchRequest::priority/triageApproved`, and
`DistributionDepot::stock_`. Friendship grants exactly this privileged emergency
access without making those fields public. It is one-way, not inherited, not transitive.

**8. How is a triage override performed?**
`applyTriage(DispatchRequest&)` sets `priority = MedicalEmergency` and
`triageApproved = true`. The priority functor then places that request ahead of
equal-priority non-triage requests.

### Encapsulation, dynamic memory, operators

**9. How is encapsulation enforced in `ReliefCargo`?**
All data members are private; access is through const-correct getters; constructors
validate inputs; mutators (`+=`, `-=`) reject invalid quantities.

**10. What are the static members and how are they kept correct?**
`TransportVehicle::operationalVehicles_` is incremented in constructors
(including copy constructor) and decremented in the destructor.
`totalDispatchedTonnes_` is incremented on dispatch and resettable on load. Static
getters expose both. Test 12 verifies the counter returns to baseline after
destruction.

**11. Describe `TransportManifest`'s dynamic memory layout.**
`ReliefCargo** lines_` points to an array of pointers; each pointer owns one
separately allocated `ReliefCargo`. `count_` and `capacity_` track use;
`ensureCapacity()` doubles capacity with `new[]`, moves pointer values, deletes the
old array.

**12. What is the rule of five and how is it implemented here?**
Because the class owns memory, it defines copy constructor (deep copy), copy
assignment (copy-and-swap for strong exception safety), move constructor, move
assignment (both steal and null the source), and destructor (delete each element
then the array).

**13. Why is copy-and-swap used for assignment?**
It builds a full deep copy first; only if that succeeds does it swap with `*this`.
The old contents are then destroyed when the temporary goes out of scope. A throw
during copying leaves the original object unchanged.

**14. What happens if a new cargo line is added but `new ReliefCargo` throws?**
`ensureCapacity()` may have grown the array, but `count_` is unchanged, so no
dangling pointer is recorded and the destructor still frees exactly the constructed
elements. The invariant "first `count_` pointers are valid" always holds.

**15. How does `operator+=` on a manifest avoid duplicate cargo?**
It searches by cargo ID (`ReliefCargo::operator==`). If found, it adds to the
existing line's quantity; otherwise it appends a new deep-copied line.

**16. How does `operator-=` behave when a line becomes empty?**
It deducts through `ReliefCargo::operator-=` and, if quantity reaches zero, deletes
that line and shifts the remaining pointers left, decrementing `count_`.

**17. How does manifest `operator==` handle line order?**
It takes snapshots, sorts them by ID with `std::sort`, and compares all fields
(including floating-point values with `nearlyEqual`). Order of insertion is irrelevant.

**18. What does `operator>>` for `ReliefCargo` read, and what is the pitfall?**
It reads ID, name, category int, quantity, weight, volume, priority int, then the
remainder of the line as destination. Using `>> std::ws` before the remainder would
skip the newline and consume the next manifest line — a real bug found and fixed
during testing.

**19. Where is `operator<<` used in the application?**
For cargo/manifest text serialisation in audits and tests, and for `DepotRecord`,
vehicle details, `DispatchRequest` and `DispatchRecord` in reports and debugging.

**20. How is a shallow copy prevented in vehicles?**
`ReliefVehicle`'s default copy copies its `TransportManifest` member, whose copy
constructor deep-copies all cargo lines. `clone()` allocates a new derived object
via its copy constructor, so polymorphic copies are also deep.

### Inheritance, diamond, containership, polymorphism

**21. Why is `ReliefVehicle` abstract?**
It declares four pure virtual functions (`kind`, `fuelRatePerKm`, `evaluateRoute`,
`clone`). It cannot be instantiated and forces every concrete vehicle to define
its identity, consumption, routing and cloning behaviour.

**22. Why must the destructor of a base class used polymorphically be virtual?**
Deleting through `ReliefVehicle*` must call the most-derived destructor first.
Without a virtual destructor only the base part would be destroyed, leaking
derived resources. Test 12 checks all four derived destructor counters.

**23. Draw the diamond and explain the virtual inheritance.**
`AmphibiousMedicalVehicle` derives from `AmphibiousCraft` and `GroundConvoy`, both
of which derive `virtual public TransportVehicle`, which derives
`virtual public ReliefVehicle`. Virtual inheritance guarantees one shared
`TransportVehicle` and one shared `ReliefVehicle` sub-object, so no ambiguous base
member accesses or duplicated fuel tanks occur.

**24. Which constructor initialises the virtual bases?**
The most-derived constructor. `AmphibiousMedicalVehicle`'s initialiser list names
both `ReliefVehicle(...)` and `TransportVehicle(...)`; the virtual-base
initialisers written in the intermediate classes are ignored when they are built as
base sub-objects.

**25. What ambiguity arises in the hybrid and how is it resolved?**
Both branches declare their own `fuelRatePerKm_`, `kind()`, `evaluateRoute()` and
`printDetails()`. The hybrid overrides these virtual functions and computes a
single effective fuel rate from both branch rates via explicit scope
(`AmphibiousCraft::fuelRatePerKm()`, `GroundConvoy::fuelRatePerKm()`).

**26. How is the single-fuel-tank property verified?**
Test 13 reads `currentFuel()` through `ReliefVehicle*` and `TransportVehicle*`
pointing at the same hybrid and checks equality; refuelling through the base
changes the only tank.

**27. What is `clone()` and why return `unique_ptr<ReliefVehicle>`?**
It is a virtual copy constructor. Returning `unique_ptr` lets callers copy a vehicle
without knowing its dynamic type and guarantees exception-safe ownership; original
code using raw `new Derived(*this)` would risk leaks.

**28. What is the difference between inheritance and containership here?**
A helicopter *is a* `ReliefVehicle` (inheritance). A vehicle *has a* `GPSBeacon`,
and the medical vehicle *has an* `EmergencyLifeSupportUnit` (composition by value).
Composition gives independent lifetimes and no is-a relationship.

**29. Where is runtime polymorphism visible in the UI?**
Menu 6 calls `displayRouteEvaluation()`, which iterates
`std::vector<ReliefVehicle*>` and calls virtual `evaluateRoute()`/`kind()`;
the output differs per vehicle type. Fleet display calls virtual `printDetails()`.

**30. Why use `vector<ReliefVehicle*>` if `unique_ptr` owns the fleet?**
The brief explicitly requires a raw-pointer vector for the polymorphism demo.
`fleetView()` builds a fresh non-owning view each time; the owning collection is
`vector<unique_ptr<ReliefVehicle>>`. Raw pointers never own or outlive the fleet.
Test 12 also uses `vector<unique_ptr<ReliefVehicle>>` for polymorphic destruction.

**31. What does `EmergencyLifeSupportUnit::provideCare()` contribute?**
It returns a care-capability string that the hybrid appends to its route note,
showing that the contained object's behaviour is genuinely used.

### Exceptions, persistence, STL, templates

**32. Name the three custom exceptions and when each is thrown.**
`CargoOverweightException` — allocation payload exceeds remaining capacity;
`FuelDepletionException` — `consumeFuel()` or dispatch lacks fuel;
`DamagedCorridorException` — route evaluation on a damaged corridor without a
commander override. All derive from `std::runtime_error` → `std::exception`.

**33. How is transactional rollback implemented for dispatch?**
Before mutating, the hub snapshots cargo quantity, vehicle fuel/status/destination,
history and audit sizes, and total tonnage. A catch-all block restores each of
these and rethrows. Validation happens before the snapshot, so failures before
mutation need no rollback.

**34. How do you guarantee a cancelled dispatch changes nothing?**
`dispatchNext(false)` only removes the request from the queue (and writes an audit
record). It never touches inventory, fuel, statistics or dispatch history.

**35. Describe the binary file format.**
8-byte magic `"ERLHUB01"`, 4-byte version, 8-byte section count (patched with
`seekp`), then six sections (depots, vehicles, history, pending, audits, meta).
Each section is `{uint8 id, uint64 payloadSize, payload}`. Strings are
`uint32 length + bytes`; numbers are written field by field with fixed widths.

**36. Where exactly are `seekp()` and `seekg()` used?**
`seekp(countPosition, beg)` patches the section count after all sections are
written; `seekp(0, end)` finalises the file. `seekg(0, beg)` re-reads the magic,
`seekg(COUNT_OFFSET, beg)` reads the patched count, and `seekg(HEADER_SIZE, beg)`
starts section parsing.

**37. How is a save made transactional on disk?**
Write `*.tmp`, reopen and verify it loads with the expected record counts, copy the
old file to `*.bak`, remove the target, rename the temporary into place. On any
failure the temporary is removed and the backup restored.

**38. Why not serialise objects with `reinterpret_cast` into a file?**
Pointers are process-specific, strings/vectors store internal pointers and
allocator-dependent layouts, and padding/endianness differ. The code writes only
explicit primitive fields with length prefixes.

**39. How are polymorphic vehicles restored from a binary file?**
The file stores a `VehicleRecord` with a `VehicleKind` tag and all primitive
fields. `ReliefHub::makeVehicle()` switches on the tag to construct the correct
derived class, then restores status, destination and manifest.

**40. What is `PriorityDispatchQueue<T>` and why is it a template?**
It is a reusable container storing any `T` in a vector, with `push`, `peek`, `pop`
and sorted snapshots. It is instantiated with `DispatchRequest` here, but the
implementation contains no request-specific code, which is the point of a template.

**41. How does the priority functor decide order?**
`DispatchPriorityComparator` compares priority enum values descending, then
commander-approved triage first, then the FIFO sequence number. `std::sort` uses it
in `priorityOrdered()`; `std::max_element` uses it for `peek`/`pop`.

**42. Which STL algorithms are used and where?**
`std::sort` in manifest comparison and queue snapshots; `std::for_each` in fleet,
queue, history, audit and route displays; `std::max_element` for queue top;
`std::count_if` for availability statistics; `std::swap`, `std::copy` indirectly.

**43. Why `unordered_map` in `ReliefHub`?**
`depotIndex_` and `vehicleIndex_` map IDs to vector indices in expected O(1),
avoiding linear scans for every allocation, dispatch and refuel request.

**44. What are the user-defined manipulators and how do they differ from standard ones?**
`manifestTableHeader/Footer`, `dispatchReportHeader/Footer` and `sectionRule` are
stateless functions `ostream&(ostream&)`, used as `os << manifestTableHeader`.
`reportTitle` and `reportField` are parameterised manipulator objects with
`operator<<(ostream&, const Type&)`. Both styles are user-defined, unlike `std::setw`.

**45. Which tests give confidence about memory safety, and what was the environment limitation?**
The 500-hybrid/500-clone stress test (test 14), deep-copy independence test (13),
virtual-destructor counter test (12), repeated runs, and a full run under
`-D_GLIBCXX_DEBUG -D_GLIBCXX_ASSERTIONS` (153/153). Runtime ASan/UBSan/Valgrind
could not run because this MinGW installation lacks `libasan`/`libubsan` and
Valgrind is not installed; ASan-instrumented compilation still succeeded. The
limitation is recorded in `docs/TEST_REPORT.md`.

### Final syllabus-coverage questions (supporting demos and audit topics)

**46. Explain public, protected and private inheritance with the exact rules.**
*[DEMO] `include/InheritanceAccessDemo.h`, `src/InheritanceAccessDemo.cpp`.*
A base's `public` members become public/protected/private in the derived class
under public/protected/private inheritance respectively; its `protected` members
become protected/protected/private; its `private` members are never directly
reachable from derived code in any mode. Example: `PubliclyDerived` can be
upcast to `AccessBase*`, whereas `PrivatelyDerived` and `ProtectedlyDerived`
cannot; `PublicGrandchild` still reaches the protected-inherited members.
Verified by `static_assert`s and test 15.

**47. Why are `static_assert`s used in the inheritance demo, and why is this demo separate?**
Because convertibility rules are compile-time properties: `static_assert` proves
`PubliclyDerived*` → `AccessBase*` is allowed while the other two are blocked.
It lives in `ReliefOps::Demos` so the existing vehicle hierarchy (which needs
public + virtual inheritance only) is untouched.

**48. What is aggregation and how does it differ from composition?**
*[DEMO] `include/DepotRegistry.h`, `src/DepotRegistry.cpp`; [APP]
`include/DistributionDepot.h:48`, `include/ReliefVehicle.h:80–81`.*
Composition is a strong `has-a`: the whole owns its parts and controls their
lifetime (`DistributionDepot` owns `vector<ReliefCargo>`; `ReliefVehicle` owns a
`GPSBeacon` by value). Aggregation is a weak `has-a`: the whole only refers to
independently owned parts. `DepotRegistry` stores `DistributionDepot*` and never
deletes them; `aggregationExplanation()` states the distinction and test 16
checks the depots outlive a `clear()` and the registry itself. In the running
application `ReliefHub` owns depots by value and the fleet via `unique_ptr`, so
`DepotRegistry` is only a syllabus example and there is no second owner.

**49. How does the registry avoid dangling pointers?**
The depots are declared before the registry and remain alive after it; the
registry only stores observing pointers, offers `unregister()`/`clear()` so
references can be dropped before objects are destroyed, and never deletes
through those pointers. Test 16 verifies `first.id()` and `second.id()` after
the registry's scope ends.

**50. Where are generic function templates used?**
*[APP]* `src/FileManager.cpp:20–29` defines `writePod<T>`/`readPod<T>` used for
`uint32`, `uint64`, `double`, etc.; `include/PriorityDispatchQueue.h:69` defines
`sortedSnapshot<Compare>` used by menu option 9. *[DEMO]*
`include/LanguageFeaturesDemo.h` defines `totalOf<Container, Getter>`; test 17
instantiates it with `ReliefCargo[3]` and with `vector<int>`.

**51. What is a generic class template and how is parametric polymorphism shown?**
*[APP]* `include/PriorityDispatchQueue.h:30` defines `PriorityDispatchQueue<T>`
independently of `DispatchRequest`. `ReliefHub` instantiates it with
`DispatchRequest`; test 18 instantiates it with `int` and supplies a different
comparator to `sortedSnapshot`. Writing once, instantiating for many types, *is*
parametric polymorphism.

**52. Where are iterators demonstrated?**
*[APP]* `std::for_each(fleet_.begin(), fleet_.end(), ...)` in `src/ReliefHub.cpp`,
`std::max_element(items_.begin(), items_.end(), Compare{})` in
`PriorityDispatchQueue`, `std::find_if` in `src/DepotRegistry.cpp`. *[DEMO]*
an explicit `for (auto it = std::begin(cargoArray); it != std::end(cargoArray);
++it)` loop in `src/LanguageFeaturesDemo.cpp`, checked by test 17.

**53. Are allocators used by the main application?**
Not explicitly: `TransportManifest` uses `new[]`/`new` directly. To make the
syllabus topic explicit, *[DEMO]* `src/LanguageFeaturesDemo.cpp` uses
`std::allocator<ReliefCargo>` with `allocator_traits::construct/destroy` and
`deallocate`, including exception cleanup. Test 17 checks the report line
"allocator constructed 2".

**54. Are there arrays of objects in the project?**
*[APP]* `DistributionDepot::stock_` is a `vector<ReliefCargo>` (dynamic array of
objects) and `TransportManifest::lines_` is a heap array of pointers. A
fixed-size raw array of objects is *[DEMO]* `ReliefCargo cargoArray[3]` in
`src/LanguageFeaturesDemo.cpp`, verified by test 17 (quantity total 10).

**55. Give examples of default function arguments in the application.**
*[APP]* `nearlyEqual(a, b, epsilon = 1e-9)` in `include/Utilities.h:42`;
`ReliefVehicle(..., GPSBeacon beacon = GPSBeacon())` in
`include/ReliefVehicle.h:25`; `IncidentCommander* commander = nullptr` in
`include/Allocation.h:52`; `ReliefHub(std::string dataFile = "data/erlhub.dat")`
in `include/ReliefHub.h:60`. Omitting them uses those defaults.

**56. Give examples of explicit and implicit conversions.**
*[APP]* `static_cast<int>(volume / volumePerUnit)` in `src/Allocation.cpp:106`
and `static_cast<int>(priority)` in the priority functor are **explicit**.
Constructors marked `explicit` (`FileManager`, `ReliefHub`, `reportTitle`)
require direct initialisation. *[DEMO]* `class Litres` in
`include/LanguageFeaturesDemo.h` has `explicit Litres(double)` but an implicit
`operator double()`, so `double d = Litres(20.0)` compiles; test 17 checks it.

**57. What is function prototyping and where is it demonstrated?**
A declaration (prototype) tells the compiler a function's signature before its
definition; separating them into headers and `.cpp` files also hides
implementation. *[APP]* `main.cpp` includes `include/ReliefHub.h` and calls
`hub.dispatchNext(bool)`, whose definition is in `src/ReliefHub.cpp`. Every
header in `include/` is a set of prototypes.

**58. What is a friend function and how is it different from a friend class?**
A friend **function** is a non-member function granted private access; a friend
**class** grants that access to all members of another class. *[APP]*
`friend std::ostream& operator<<` / `friend std::istream& operator>>` in
`include/ReliefCargo.h`, and `friend class IncidentCommander` in
`include/ReliefVehicle.h`. *[DEMO]* `sameVolume(const Litres&, const Litres&)`
reads a private field; test 17 checks both true and false cases.

**59. Distinguish function overloading, overriding and polymorphism by parameter.**
Overloading = same name, different signature, chosen at compile time
(`allocateSupply(double volumeM3, ...)` vs `allocateSupply(int, bool, ...)` in
`include/Allocation.h`; test 18 verifies different results). Overriding = a
derived class redefines a virtual base function, chosen at runtime
(`evaluateRoute()` in each vehicle; test 12). Polymorphism by parameter is the
compile-time selection of the right overload/operator from the argument types —
e.g. a `double` argument selects volume allocation, an `int`+`bool` selects
emergency allocation.

**60. What happens when a refuel request exceeds the remaining tank capacity?**
*[APP]* It is **trimmed**, not rejected. `ReliefVehicle::refuel(double)`
(`src/ReliefVehicle.cpp:42`) computes `freeSpace = fuelCapacity_ - currentFuel_`,
adds `clampValue(litres, 0.0, freeSpace)` and returns the amount actually added.
`ReliefHub::refuel` (`src/ReliefHub.cpp:352`) builds a receipt containing the
requested amount, actual amount and final fuel, audits both values, and saves.
Example from `tests/test_inputs/refuel_trim.txt`: request 50 L with 20 L free →
"requested 50.000000 L, actually added 20.000000 L, final fuel 200.000000 L".
Test 10 also checks a request that fits is added in full and that both levels
survive a reload. Test 19 adds the exact brief example (100 L tank, 80 L current,
50 L requested → 20 L added), an exact-fit request (50 L into 50 L free), and an
already-full tank (25 L requested → 0 L added, fuel stays at 100 L). Zero and
negative amounts are still rejected. `tests/test_inputs/refuel_bounds.txt`
reproduces the exact-fit/full cases through menu 14. If persistence fails, the
*actual* amount is rolled back.
