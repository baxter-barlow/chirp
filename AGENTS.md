# Repository Guidelines

## Project Structure & Module Organization
- `packages/ti/` contains the SDK source, libraries, and device-specific code (drivers, datapath, control, demos, and algorithms).
- `packages/ti/**/test/` holds unit and integration test projects, with device-specific variants (e.g., `xwr68xx`).
- `docs/` includes the SDK user guide, unit test procedure, and release notes.
- `tools/` provides host utilities and build helpers.
- `firmware/` contains radar subsystem binaries used during image creation and flashing.

## Build, Test, and Development Commands
- Initialize the environment before building:
  ```sh
  source packages/scripts/unix/setenv.sh
  ```
  This sets toolchain and SDK paths (e.g., `XDC_INSTALL_PATH`, `R4F_CODEGEN_INSTALL_PATH`).
- Build a demo or module using its makefile. Example:
  ```sh
  make -f packages/ti/demo/xwr68xx/mmw/mss/mmw_mss.mak
  ```
- Build a test target using its test makefile. Example:
  ```sh
  make -f packages/ti/drivers/gpio/test/xwr68xx/mssTest.mak
  ```
- For tool/path validation, run:
  ```sh
  packages/scripts/unix/checkenv.sh
  ```

## Coding Style & Naming Conventions
- C/C++ code uses 4-space indentation and K&R-style braces; keep this consistent.
- File names are lower_snake_case for modules and `xwr68xx_*` for device-specific artifacts.
- Macro names are uppercase with underscores (e.g., `SOC_XWR68XX_GPIO_0`).
- Avoid introducing non-ASCII characters; the codebase is ASCII-first.

## Testing Guidelines
- Tests live under `packages/ti/**/test/` and are typically built via `.mak` files per device.
- Follow the unit test procedure in `docs/mmwave_sdk_unit_test_procedure.pdf` when running full suites.
- Name new tests to match existing patterns (e.g., `mssTest.mak`, `dssTest.mak`).

## Commit & Pull Request Guidelines
- This checkout is not a Git repository, so no commit history is available to infer conventions.
- If you add Git history, keep commits scoped and descriptive, and include build/test notes in the message.
- For PRs, include a short summary, affected devices, and the exact build/test commands used.

## Configuration & Tooling Notes
- `packages/scripts/unix/setenv.sh` is the authoritative place for toolchain paths and device selection (`MMWAVE_SDK_DEVICE`).
- Secure-device flows require `MMWAVE_SECDEV_INSTALL_PATH` and `MMWAVE_SECDEV_HSIMAGE_CFG` to be set.
