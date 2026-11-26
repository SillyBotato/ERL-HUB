# ERL-HUB — Class Diagram and Relationships

## 1. Inheritance tree (UML-style text)

```
                          +---------------------------+
                          |     ReliefVehicle  (ABC)  |
                          |---------------------------|
                          | # id_, fuelCapacity_,     |
                          |   currentFuel_,           |
                          |   maxPayloadKg_, status_, |
                          |   destination_            |
                          | - corridorOverride_       |
                          | # beacon_ : GPSBeacon     |
                          | # manifest_: Transport... |
                          | + evaluateRoute() = 0     |
                          | + kind() = 0              |
                          | + fuelRatePerKm() = 0     |
                          | + clone() = 0             |
                          | + virtual ~ReliefVehicle()|
                          +-------------+-------------+
                                        ^  (virtual)
                                        |
                          +-------------+-------------+
                          |     TransportVehicle      |
                          |---------------------------|
                          | # model_                  |
                          | $ operationalVehicles_    |
                          | $ totalDispatchedTonnes_  |
                          +-------------+-------------+
                            ^ (virtual)   ^ (virtual)
              +-------------+             +-------------+
              |                           |
      +-------+--------+          +--------+--------+
      | AerialTransport|          | GroundConvoy   |
      |----------------|          |----------------|
      | crew_, rotor   |          | axles_         |
      +----------------+          +--------+-------+
                                           ^ (public)
      +-------------------+                |
      | AmphibiousCraft   |                |
      |-------------------|                |
      | hullTonnes_,      |                |
      | waterSpeedKnots_  |                |
      +---------+---------+                |
                ^ (public)                 |
                +------------+-------------+
                             |
              +--------------+---------------+
              | AmphibiousMedicalVehicle     |
              |------------------------------|
              | - lifeSupport_ : ELSU  (has-a)|
              | - triageBeds_                |
              | evaluateRoute() override     |
              +------------------------------+
```

`$` = static member. `#` = protected. `-` = private. `^ (virtual)` marks virtual
inheritance.

## 2. Containership (has-a) relationships

| Owner | Member | Multiplicity | Notes |
|---|---|---|---|
| `ReliefVehicle` | `GPSBeacon beacon_` | 1 | Constructed and destroyed with the vehicle |
| `ReliefVehicle` | `TransportManifest manifest_` | 1 | Deep-copied on vehicle copy |
| `AmphibiousMedicalVehicle` | `EmergencyLifeSupportUnit lifeSupport_` | 1 | Not shared with other vehicles |
| `TransportManifest` | `ReliefCargo** lines_` | 0..n | Dynamic array, explicit deep copy |
| `DistributionDepot` | `std::vector<ReliefCargo> stock_` | 0..n | STL ownership |
| `ReliefHub` | `std::vector<std::unique_ptr<ReliefVehicle>> fleet_` | 0..n | Sole owner of vehicles |
| `ReliefHub` | `PriorityDispatchQueue<DispatchRequest>` | 1 | Template instantiation |
| `ReliefHub` | `std::unordered_map<std::string, size_t>` | 2 | Fast ID lookup |
| `DepotRegistry` **[DEMO]** | `std::vector<DistributionDepot*>` | 0..n | **Aggregation**: non-owning pointers; depots live independently and are never deleted by the registry |

`ReliefHub` owns and indexes the real depots and fleet; `DepotRegistry` is only the
aggregation example. A separate `FleetRegistry` is intentionally absent because it
would duplicate `fleet_`/`vehicleIndex_` ownership or lookup. See
`docs/ARCHITECTURE.md` §3c.

## 3. Diamond resolution details

`AmphibiousMedicalVehicle` constructor initialisation order (most-derived is
responsible for virtual bases):

```cpp
AmphibiousMedicalVehicle::AmphibiousMedicalVehicle(...)
  : ReliefVehicle(id, fuelCapacity, currentFuel, maxPayload, status),   // virtual base
    TransportVehicle(id, fuelCapacity, currentFuel, maxPayload, status, model), // virtual base
    AmphibiousCraft(id, ..., hullTonnes, waterSpeed),
    GroundConvoy(id, ..., axles),
    lifeSupport_(...), triageBeds_(...) {}
```

The two virtual-base initialiser lists in `AmphibiousCraft` and `GroundConvoy` are
ignored when they are constructed as base sub-objects of the hybrid. There is one
`ReliefVehicle::currentFuel_`, so `refuel()` and `consumeFuel()` act on a single
fuel tank, and there is one `TransportVehicle` static counter update per object.

## 4. Friend relationships

```
IncidentCommander  ---friend--->  ReliefVehicle        (reads/writes private fuel,
                                                       corridorOverride_)
IncidentCommander  ---friend--->  DispatchRequest      (triage override)
IncidentCommander  ---friend--->  DistributionDepot    (emergency restock of private stock_)
```

Friendship is one-way, not inherited and not transitive.

## 5. Operator placement

| Operator | Class | Meaning |
|---|---|---|
| `+=` | `ReliefCargo` | Increase quantity of a cargo line |
| `-=` | `ReliefCargo` | Decrease quantity (throws if insufficient) |
| `==` | `ReliefCargo` | Same cargo ID → duplicate detection |
| `<<`, `>>` | `ReliefCargo` | Textual display / read-back |
| `+=` | `TransportManifest` | Add a new line or restock matching ID |
| `-=` | `TransportManifest` | Deduct dispatched cargo, erase empty lines |
| `==` | `TransportManifest` | Compare manifests line-by-line after sorting by ID |
| `<<`, `>>` | `TransportManifest` | Serialise/deserialise whole manifest for audit tests |
| `operator<<` | `DepotRegistry` **[DEMO]** | Non-owning registry report |

## 6. Supporting demonstration classes (namespace `ReliefOps::Demos`)

These classes are deliberately separate from the relief domain; the existing
vehicle hierarchy above is unchanged.

```
AccessBase                         <- public <- PubliclyDerived
    ^                              <- private <- PrivatelyDerived
    |                              <- protected <- ProtectedlyDerived <- public <- PublicGrandchild
    (public/protected/private members; static_assert checks convertibility)

DepotRegistry  o----> DistributionDepot   (aggregation: o = non-owning reference;
                                           DistributionDepot objects are created
                                           and destroyed independently)

Litres                              (explicit ctor from double, implicit conversion
                                    to double, friend function sameVolume)
```

`LanguageFeaturesDemo.cpp` additionally demonstrates `ReliefCargo cargoArray[3]`,
explicit iterator traversal, `std::allocator<ReliefCargo>` and a generic
`totalOf<Container, Getter>` function template. Tests 15–17 exercise all of them.
