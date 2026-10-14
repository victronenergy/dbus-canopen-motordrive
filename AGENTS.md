# AGENTS.md

Guidance for AI coding agents working in this repository.

## Never commit

**Never run `git commit`, `git push`, or any other command that creates or
publishes commits.** This includes amends, merges, rebases, cherry-picks,
reverts, tags and stashes applied on top of commits. It applies even when a
task looks finished and even if a commit seems to be requested. Leave all
changes uncommitted in the working tree. The maintainer reviews and commits
them.

## Do not use the Docker build environment

**Do not run `docker`, `docker compose`, `scripts/up.sh`, `scripts/shell.sh`,
`scripts/compile.sh` or `scripts/pack.sh`.** Do not build, start or exec into
the `dbus-canopen-motordrive-build` container. Those steps download the Venus
SDK, need a privileged container and produce release artifacts, and only the
maintainer runs them. To check your changes, use the host test suite
(`./scripts/test.sh`) and the formatter. If a task needs an ARM build or a
`venus-data.zip`, stop and ask the maintainer.

## What this is

`dbus-canopen-motordrive` is a Venus OS (Victron GX) service written in C. It
polls CANopen motor controllers over SDO on a VE.Can port and publishes
motordrive data (voltage, current, power, RPM, direction, temperatures, torque)
on D-Bus as `com.victronenergy.motordrive.<id>` services. Supported
controllers: Sevcon Gen4 AC, Curtis F series, Curtis 123X E/SE, DMC Sigma2N
(Full CANopen firmware V03.x).

It is built on **velib** (Victron's embedded C library, git submodule at
`velib/`), which provides the main loop, CAN drivers, `VeItem` value trees,
D-Bus bindings, timers and logging. `Makefile` and `mk` are symlinks into
`velib/mk`.

## Layout

- `src/task.c`: velib entry points (`taskEarlyInit`, `taskInit`, `taskUpdate`,
  `taskTick`). Scheduling: fast read every 125 ms, full read every 1 s,
  reconnect attempt every 10 s. All reads are skipped while a scan runs.
- `src/canopen.c`: a single global FIFO of pending SDO requests
  (`canOpenState.pendingSdoRequests`). Only the head request is in flight.
  50 ms timeout per request. Supports expedited read/write, segmented read, and
  `QUEUE_CALLBACK` entries that run a callback once everything queued before
  them is done. EMCY frames (`0x80 + nodeId`) go to a registered handler.
- `src/discovery.c`: identifies the driver for a node. It reads 0x1008 (device
  name, segmented). Names containing `Gen4` select Sevcon and names starting with
  `AC F` select Curtis F. If the read fails with anything other than a timeout,
  it falls back to 0x1018.1 (vendor ID): Curtis checks the model number at
  0x3464, and DMC checks the product code at 0x1018.2.
- `src/node.c`: `nodes[127]` (indexed `nodeId - 1`), connect/disconnect, read
  dispatch, and EMCY dispatch. On connect it reads the serial number from
  0x1018.4, or uses the node ID if the node aborts with "no object/subindex".
- `src/device.c`: creates the per-node D-Bus service, the VRM device instance
  and the `VeItem` tree (`Dc/0/*`, `Motor/*`, `Controller/Temperature`,
  `CustomName`, `Motor/DirectionInverted` settings proxy).
- `src/servicemanager.c`: the `com.victronenergy.canopenmotordrive.<canGw>`
  service. It exposes `/Scan`, `/ScanProgress` and `/DiscoveredNodes`, which
  is persisted in localsettings under `Settings/CanOpenMotordrive/<canGw>`.
- `src/notification.c` and `src/platform.c`: queue GX notifications and inject
  them through `com.victronenergy.platform /Notifications/Inject` after a 5 s
  delay. Notifications are dropped if the node has disconnected by then.
- `src/drivers/*.c` with `inc/drivers/*.h`: one file per controller family.
- `src/memory.c`: wrappers `_malloc`/`_realloc`/`_free`/`_strdup`, which tests
  can fake.
- `tests/`: GoogleTest suite with fff fakes, built with CMake.
- `tools/simulators/*.py` and `*.eds`: python-canopen simulators for each
  controller, used for manual testing on vcan or a real bus.
- `archive/`: the files packaged into `venus-data.zip` (runit `run` script and
  an rc post-hook that symlinks the service into `/opt/victronenergy/service`).

## Writing a driver

A driver is a `Driver` struct (`inc/driver.h`) with these members:

- `name`: used in the D-Bus identifier `<canGw>_<name>_<serial>`. Do not rename
  it, because that changes the VRM instance and the settings paths.
- `productId`: a `VE_PROD_ID_*` value from velib `products.h`.
- `readRoutine` / `fastReadRoutine`: these queue SDO reads with
  `canOpenReadSdoAsync(nodeId, index, sub, node, onResponse, onError)`. The fast
  routine normally reads RPM only.
- `createDriverContext` / `destroyDriverContext`: optional per-node state. May
  be NULL.
- `onEMCYMessage`: decodes faults and calls `queueNotification(...)`.

Follow these conventions, which every existing driver uses:

- Every response callback starts with
  `node = (Node *)request->context; if (!node->connected) return;`. Requests
  can complete after a disconnect.
- On an SDO error, the driver calls `disconnectFromNode(nodeId)`. The 10 s
  connect task reconnects it later.
- Set values with `veItemOwnerSet`. Power is recomputed from the voltage and
  current responses. Changes are flushed by the `QUEUE_CALLBACK` that `node.c`
  appends after each routine (`veItemSendPendingChanges`).
- Direction is 0 for neutral, 1 for reverse and 2 for forward. Apply the
  `Motor/DirectionInverted` setting to it.
- The request queue is strictly ordered. Drivers can rely on this, for example
  DMC reads the fault code (0x3840) and then the subcode (0x3841).
- Allocate with `_malloc` and check the result with `CHECK_ALLOC(ptr)`, which
  exits on failure.
- Log with `info()`, `warning()` and `error()` from `logger.h`.

To add a new driver:

1. Add `src/drivers/<name>.c` and `inc/drivers/<name>.h`.
2. Add the `.c` file to `SRCS` in `src/drivers/rules.mk` and to
   `tests/CMakeLists.txt`.
3. Hook detection into `src/discovery.c`.
4. Add `tests/test_<name>.cpp` and register it in `tests/CMakeLists.txt`.
5. Add a simulator and EDS file under `tools/simulators/`.
6. Document wiring and the SDOs used in `README.md`.

## Build, test, format

- **Tests (on the host):** `./scripts/test.sh` runs the CMake build in
  `build-tests/`, runs ctest, and writes an lcov HTML report to `coverage/`.
  The tests need gcc, cmake, `libdbus-1-dev`, `libevent-dev` and lcov. Coverage
  requires GCC. CI (`.github/workflows/tests.yml`) builds `tests/` with CMake
  and runs `dbus-canopen-motordrive-test`.
  - To run a single test:
    `./build-tests/dbus-canopen-motordrive-test --gtest_filter='DmcTest.*'`.
- **Target binary (ARM, Venus), for the maintainer only. Agents must not run
  these (see above).** `./scripts/up.sh` downloads the Venus SDK toolchain into
  `build/` and starts the Docker build container. `./scripts/shell.sh` opens a
  shell in it, where `./scripts/compile.sh` builds
  `obj/linux-arm-gnueabi-release/dbus-canopen-motordrive`. `./scripts/pack.sh`
  produces `dist/venus-data.zip`.
- **Formatting:** clang-format with LLVM style and 4-space indent
  (`.clang-format`). CI uses clang-format 21 through
  `./scripts/check-formatting.sh` over `src/`, `inc/` and `tests/`. Run
  `clang-format -i` on the files you touch before committing.

## Tests

- Fixtures: `BaseFixture` (`tests/BaseFixture.hpp`) resets the fff fakes for
  memory, D-Bus, CAN, time and notifications. `CanFixture`
  (`tests/CanFixture.hpp`) records sent frames in `canMsgSentLog` and feeds
  `canMsgReadQueue` to `veCanRead`.
- The usual pattern is: queue a request, call `canOpenTx()`, push the response
  frame(s) to `canMsgReadQueue`, call `canOpenRx()`, then assert on the `VeItem`
  values or the sent frames. Advance time with `pltGetCount1ms_fake.return_val`
  to trigger timeouts.
- New velib or system functions called from `src/` may need a fake in
  `tests/environment.hpp` and `tests/environment.cpp`. Velib sources compiled
  into the test library are listed in `tests/CMakeLists.txt`.
- `task.c`, `platform.c` and `memory.c` are not compiled into the tests.
- Prefer real captured frames in tests, with a comment giving their origin
  (see `tests/test_dmc.cpp`).

## Conventions

- C99-style C with velib types (`un8`, `un16`, `sn32`, `veBool`, ...).
  Declare variables at the top of functions, as the existing code does.
  Functions are camelCase, types are PascalCase, and file-local functions are
  `static`.
- Comments explain *why*, especially controller firmware quirks. Cite the
  manual and section, or the firmware version observed.
- The maintainer's commits use Conventional Commits with a scope. If you are
  asked to suggest a commit message, write it in this style, for example
  `fix(dmc): ...`, `feat(sevcon): ...`, `test(dmc): ...`,
  `chore(release): 1.14`.
- The version lives in `rules.mk` (`VERSION`).
- Keep `README.md` in sync when SDOs, wiring or supported models change.
- Do not modify `velib/` here. It is a submodule, so change it upstream and bump
  the submodule.

## Manual testing on a GX device (for the maintainer)

- Start a scan:
  `dbus -y com.victronenergy.canopenmotordrive.<canGw> /Scan SetValue 1`.
- Set the discovered nodes directly: `... /DiscoveredNodes SetValue <ids>`.
- The simulators connect at 250 kbit/s, for example
  `python3 tools/simulators/dmc_sigma2n.py` (see `--help` for the bus and
  channel options).
