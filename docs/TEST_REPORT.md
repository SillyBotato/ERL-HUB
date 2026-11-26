# ERL-HUB — Test Report (executed)

Date: 2026-11-25  
Environment: Windows, MinGW-w64 g++ 15.2.0, `mingw32-make` (the GNU make
executable available in this toolchain; plain `make` is not installed).  
Compiler flags for all normal builds: `-std=c++17 -Wall -Wextra -pedantic -Iinclude`.
No compiler warnings were emitted by `main.cpp`, `tests/test_hub.cpp`, or any of
the 18 translation units under `src/`.

## 1. Commands actually executed (final verification)

| Command | Result |
|---|---|
| `mingw32-make clean` | PASS — all build artefacts removed |
| `mingw32-make all` (clean rebuild) | PASS — `bin/erlhub.exe` and `bin/tests.exe` built, no warnings |
| `bin\tests.exe` | PASS — **153 checks passed, 0 failed**, exit code 0 |
| `bin\tests.exe` × 5 repeated runs | PASS — 153/0 with exit code 0 on every run |
| strict warning syntax check (`-Wshadow -Wnon-virtual-dtor -Woverloaded-virtual -Wcast-align -Wnull-dereference -fsyntax-only`, `main.cpp` + `tests/test_hub.cpp` + all 18 `src/*.cpp`) | PASS, no output, exit code 0 |
| `mingw32-make debugstl` + `bin\tests_debugstl.exe` | PASS — `-D_GLIBCXX_DEBUG -D_GLIBCXX_ASSERTIONS`, 153/0, exit 0 |
| ASan/UBSan instrumented compile of **all 18** `src/*.cpp` files (`-fsanitize=address,undefined -fno-omit-frame-pointer -g -c`) | PASS — 18/18 compiled, exit 0 |
| `mingw32-make asan-tests` (link/run) | BLOCKED — toolchain has no `libasan`/`libubsan`; see §5 |
| `where.exe valgrind` | BLOCKED — Valgrind is not installed |
| `cmd /c "bin\erlhub.exe < tests\test_inputs\menu_smoke.txt > tests\test_outputs\menu_smoke.out 2>&1"` | PASS — exit 0, no `Operation failed` lines |
| restart load run (`tests\test_inputs\restart_load.txt`) | PASS — exit 0, saved state restored in a new process |
| text-record import run (`tests\test_inputs\import_record.txt`) | PASS — exit 0, `operator>>` read `MED9 Trauma-kits 2 40 7.5 0.3 4 SectorZ` |
| refuel-trim run (`tests\test_inputs\refuel_trim.txt`) | PASS — exit 0, 50 L requested / 20 L added / 200 L final |
| refuel-bounds run (`tests\test_inputs\refuel_bounds.txt`) | PASS — exit 0, exact fit 150 + 50 → 200 L; full tank 200 + 25 → 0 L added |

## 2. Automated test scenarios (tests/test_hub.cpp)

| # | Scenario | Expected | Actual | Status |
|---|---|---|---|---|
| 1 | Normal inventory management | depot/cargo registered; duplicate ID rejected; restock via `+=` gives 125 units; negative restock rejected | exactly as expected | PASS |
| 2 | Successful supply allocation | request queued; inventory and fuel unchanged until dispatch; payload 200 kg recorded | exactly as expected | PASS |
| 3 | Successful dispatch | stock 100→70; fuel reduced; status `OnMission`; 1 history record; tonnage > 0; queue empty | exactly as expected | PASS |
| 4 | Overweight cargo | `CargoOverweightException`; no request; stock unchanged | exactly as expected | PASS |
| 5 | Insufficient fuel | `FuelDepletionException`; stock, tonnage, history unchanged; request retained | rollback verified | PASS |
| 6 | Damaged corridor | `DamagedCorridorException`; commander override permits route; inspection shows override; revocation restores exception | exactly as expected | PASS |
| 7 | Duplicate cargo / operators | `ReliefCargo ==` and depot duplicate throw work; `+=` restocks existing line; `==` order-independent; `<<`/`>>` round-trip; `-=` deducts | `>>` initially ate the next line; fixed by not skipping whitespace before the remainder-of-line destination | PASS |
| 8 | Cancelled dispatch | inventory, fuel, stats, history unchanged; request removed | exactly as expected | PASS |
| 9 | Invalid user inputs | unknown depot/vehicle, empty destination, zero quantity, duplicate vehicle rejected; empty-queue preview safe | exactly as expected | PASS |
| 10 | **Refuelling with automatic trimming** | negative/zero rejected; 1000 L request trimmed to free capacity with **no exception**; receipt shows requested/added/final; a request that fits is added in full; both levels persist across reload | exactly as expected, 13 checks | PASS |
| 11 | Binary save/load across restart | magic header; depot stock 88; hybrid vehicle kind; exact fuel; 1 history record; 1 pending request; tonnage and vehicle count restored | exactly as expected | PASS |
| 12 | Virtual destructors | all four derived destructor counters incremented (hybrid also destroys both branch bases); base destructor count 4; static count balanced | exactly as expected | PASS |
| 13 | Deep copy | independent copy ctor and assignment; self-assignment safe; single hybrid fuel tank; clone independent manifest | exactly as expected | PASS |
| 14 | Memory safety stress / utilities | 500 hybrids + 500 deep clones alive and then freed with balanced static counter; inline utilities deterministic; manipulators emit text | exactly as expected | PASS |
| 15 | **Inheritance access modes** | public upcast allowed; private/protected upcast blocked; derived reachability correct; grandchild of protected base can still access; report contains all modes | `static_assert` + runtime checks all satisfied | PASS |
| 16 | **Aggregation (`DepotRegistry`)** | register/existing access, duplicate rejected, mutation reaches original, sorted IDs, unregister/clear do not destroy depots, depots outlive registry, composition vs aggregation explanation | no dangling pointers; 12 checks satisfied | PASS |
| 17 | **Supporting language features** | array of objects (total 10), iterator traversal (3), allocator (2 objects), explicit/implicit conversion, friend function, generic function template with another type | report strings and direct checks all satisfied | PASS |
| 18 | **Polymorphism by parameter + parametric polymorphism** | `double` selects volume overload (2 units/20 kg), `int`+`bool` selects emergency overload (triage priority), overloads distinct, generic queue works with `int` | exactly as expected | PASS |
| 19 | **Registration, lookup and refuel boundary cases** | depot/vehicle registration receipts and lookup; duplicate/unknown IDs rejected; direct 100 L / 80 L / 50 L → 20 L added; exact fit → 50 L added; already full → 0 L added; negative/zero rejected; hub receipts for all boundaries; reload in fresh hub; failed-dispatch rollback preserves status/destination and leaves request retryable after refuel | exactly as expected, 30 checks | PASS |

Final harness output:

```
========== SUMMARY ==========
  PASSED: 153
  FAILED: 0
```

## 3. Scripted UI verification

- `tests/test_outputs/menu_smoke.out` — depot/cargo/vehicle registration,
  allocation queue, dispatch preview/confirmation, 200 kg dispatched, statistics,
  refuel receipt, save, load, reports, clean exit. No `Operation failed` line.
- `tests/test_outputs/restart_load.out` (fresh process):

```
=> Saved data restored successfully.
| C1 | Ration kits | Ration kits | 80 | 2 | 800.0 |
Total dispatched tonnage    : 0.200000 tonnes
Total cargo dispatched      : 200.000000 kg
DSP-00006 ... V1 ... Village North ... 20 ... 22.1
AUD-00007 [DISPATCH] D1 / V1 : DSP-00006 -> Village North
```

- `tests/test_outputs/import_record.out` — menu option 1 reads a cargo record via
  `operator>>` and reports "(record read with operator>>)".
- `tests/test_outputs/refuel_trim.out` — trimming behaviour in the real UI:

```
Requested: 50 L
=> Vehicle V1 refuel: requested 50.000000 L, actually added 20.000000 L, final fuel 200.000000 L. Saved.
```

- `tests/test_outputs/refuel_bounds.out` — exact-fit and already-full cases in
  the real UI, followed by load and fleet review:

```
=> Vehicle EXACT refuel: requested 50.000000 L, actually added 50.000000 L, final fuel 200.000000 L. Saved.
=> Vehicle FULL refuel: requested 25.000000 L, actually added 0.000000 L, final fuel 200.000000 L. Saved.
=> Saved data restored successfully.
```

This confirms inventory, statistics, history, audits, fuel level and the new
refuelling policy survive an application restart.

## 4. Transactional and ownership behaviour verified

- Insufficient fuel during dispatch (test 5): depot quantity, vehicle fuel,
  status/destination, tonnage and history were restored; the request stayed queued.
- Test 19 additionally checks that rollback leaves status and destination intact
  and that the still-pending request can be dispatched after a trimmed refuel.
- Cancellation (test 8): no state change apart from removing the request and
  writing a cancellation audit record.
- Save: writes `data/*.tmp`, re-reads and verifies it, copies the previous file to
  `*.bak`, then renames; verification/rename failure removes the temporary file
  and, if needed, restores the backup.
- Refuel: if persistence fails, the **actually added** amount is consumed back
  and the audit record is removed before the exception is rethrown. The rollback
  path now restores fuel only when a positive amount was consumed, so it cannot
  call the deliberately invalid `refuel(0)`.
- Ownership: `ReliefHub` is the only application owner (`unique_ptr` fleet, depots
  by value); `fleetView()` and `DepotRegistry` are non-owning observers. No raw
  pointer deletes an object. Debug-STL and the 500-object clone stress test
  returned all static counters to baseline.

## 5. Sanitizer limitation (honest disclosure)

`mingw32-make asan-tests` produced:

```
ld.exe: cannot find -lasan: No such file or directory
ld.exe: cannot find -lubsan: No such file or directory
collect2.exe: error: ld returned 1 exit status
```

`where.exe valgrind` reported that no Valgrind executable is installed.
Therefore runtime ASan/UBSan/Valgrind execution could **not** be performed in
this environment. This is a toolchain limitation, not a skipped or failed test.

Compensation executed in the final verification:

1. ASan/UBSan **instrumented compilation** succeeded for all 18 translation
   units with `-fsanitize=address,undefined -fno-omit-frame-pointer -g -c`.
2. Full suite built and run with `-D_GLIBCXX_DEBUG -D_GLIBCXX_ASSERTIONS`:
   153/0, exit 0. Debug STL detects invalid iterator/vector/string usage.
3. Strict warning syntax check with five additional warning families produced
   no diagnostics.
4. 500 hybrid vehicles plus 500 deep clones were constructed and destroyed,
   with static counts returning exactly to baseline.
5. Five repeated normal runs produced identical 153/0 results.

## 6. Remaining limitations

- Binary files use host-endian, fixed-width integers and IEEE-754 doubles. They
  are intended to be read back on the same platform family; a portable
  network-byte-order encoder would be needed for cross-platform exchange.
- Route distances are deterministic hashes of place names, not real GIS data.
- `CorridorStatus` for a pending request is always `Clear` in the menu flow;
  damaged corridors are demonstrated through `ReliefVehicle::evaluateRoute()` and
  the commander override (test 6). This keeps menu option 7 within the specified
  input fields.
- `DepotRegistry` is a supporting aggregation example and is intentionally not
  used by `ReliefHub`; no `FleetRegistry` exists because `ReliefHub` already owns
  and indexes the fleet. See `docs/ARCHITECTURE.md` §3c.
- No GUI or networking, by design.
