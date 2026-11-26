# ERL-HUB tests

## Automated harness

`test_hub.cpp` is a standalone executable (no test framework, no external
libraries) containing 19 scenario groups and 153 individual assertions. It prints
`[PASS]` / `[FAIL]` per check and exits non-zero if any check fails.

```bash
mingw32-make tests
mingw32-make run-tests
# or
g++ -std=c++17 -Wall -Wextra -pedantic -Iinclude tests/test_hub.cpp src/*.cpp -o bin/tests
bin/tests.exe
```

Covered scenarios: inventory management, allocation, dispatch, overweight cargo,
insufficient fuel, damaged corridor, duplicate cargo, cancelled dispatch, invalid
inputs, refuelling with automatic trimming and boundary cases, registration and
lookup, duplicate IDs, dispatch rollback/retry, save/load across restart, virtual
destructor invocation, deep copy correctness, a 500-object memory stress test,
inline utilities and manipulators, inheritance access modes, aggregation,
supporting language features, and polymorphism by parameter.

## Scripted UI runs

Input scripts and captured output:

- `test_inputs/menu_smoke.txt` → `test_outputs/menu_smoke.out`
  (covers menu options 1–14 and 0 with a ground convoy dispatch).
- `test_inputs/restart_load.txt` → `test_outputs/restart_load.out`
  (fresh process loads the saved file and verifies inventory, statistics,
  history, audits and fleet).
- `test_inputs/import_record.txt` → `test_outputs/import_record.out`
  (menu option 1 imports a cargo record through `ReliefCargo::operator>>`).
- `test_inputs/refuel_trim.txt` → `test_outputs/refuel_trim.out`
  (50 L requested with 20 L free → 20 L added, final 200 L).
- `test_inputs/refuel_bounds.txt` → `test_outputs/refuel_bounds.out`
  (exact fit 150 + 50 → 200 L; already full 200 + 25 → 0 L added; load/review).

Reproduce from the project root:

```bash
cmd /c "bin\erlhub.exe < tests\test_inputs\menu_smoke.txt > tests\test_outputs\menu_smoke.out 2>&1"
cmd /c "bin\erlhub.exe < tests\test_inputs\restart_load.txt > tests\test_outputs\restart_load.out 2>&1"
cmd /c "bin\erlhub.exe < tests\test_inputs\import_record.txt > tests\test_outputs\import_record.out 2>&1"
cmd /c "bin\erlhub.exe < tests\test_inputs\refuel_trim.txt > tests\test_outputs\refuel_trim.out 2>&1"
cmd /c "bin\erlhub.exe < tests\test_inputs\refuel_bounds.txt > tests\test_outputs\refuel_bounds.out 2>&1"
```

Run the smoke script first: it writes the `data/erlhub.dat` file that the restart
script loads.

## Persistence fixtures

`test_outputs/save_test.dat`, `test_outputs/refuel_test.dat`,
`test_outputs/refuel_bounds_test.dat` and
`test_outputs/refuel_rollback_test.dat` are binary files written and read by the
automated harness. They contain only explicit field data (no object addresses).
They are safe to delete; the harness recreates them.

## Quality builds

```bash
# Debug STL (portable memory/iterator checking)
g++ -std=c++17 -Wall -Wextra -pedantic -D_GLIBCXX_DEBUG -D_GLIBCXX_ASSERTIONS \
    -Iinclude tests/test_hub.cpp src/*.cpp -o bin/tests_debugstl.exe
bin/tests_debugstl.exe

# Sanitizers, only where the toolchain ships libasan/libubsan
mingw32-make asan-tests
bin/tests_asan.exe
```

See `../docs/TEST_REPORT.md` for actual results and the sanitizer limitation in
the development environment.
