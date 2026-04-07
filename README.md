# ZeyBack R2RDAC

High-performance DIY USB Audio / FPGA based R-2R DAC platform.

This repository contains:
- STM32H743 USB Audio Class 2.0 (UAC2) firmware (PCM + DSD related work)
- FPGA logic used for audio data path / clocking / I2S/DSD processing
- Hardware design files (KiCad schematics/PCB, BOM, Gerbers)

## Repository structure

- `Firmware/Stm32/`  
  STM32H743 project (STM32CubeIDE). Includes STM32Cube/HAL and middleware components.

- `Firmware/Fpga/`  
  FPGA project files (Vivado project + sources).

- `Hardware/`  
  KiCad project files, BOM/CSV, and manufacturing outputs (Gerbers).

## Build / Tools

- STM32: STM32CubeIDE (or gcc-arm + Make/CMake if you prefer)
- FPGA: Xilinx Vivado
- Hardware: KiCad

## DSD Native / Volumio notes

Goal: DSD Native playback on Linux (Volumio / Raspberry Pi) with proper USB descriptors and, if required,
a targeted `snd-usb-audio` quirk limited to this device (VID+PID).

## Licensing (IMPORTANT)

This repository is **multi-licensed**:

- **Firmware** (our original code): **GPL-3.0-or-later**
- **Hardware design files** (KiCad schematics/PCB): **CERN-OHL-S-2.0**
- **Documentation** (if/when added under `docs/`): **CC BY-SA 4.0**
- **Third-party components** (e.g., STM32Cube/HAL, middleware): keep their **original licenses**
  as stated in file headers and vendor license texts.

See:
- `LICENSE` (GPLv3 for the main project codebase)
- `Firmware/LICENSE` (firmware scope)
- `Hardware/LICENSE` (hardware scope)
- `THIRD_PARTY_NOTICES.md`

## Status

Work in progress.
Hardware design files in this directory (KiCad schematics/PCB and related design sources)
are licensed under CERN-OHL-S-2.0 (Strongly Reciprocal).

Manufacturing outputs (Gerbers) are provided for convenience.

See THIRD_PARTY_NOTICES.md for any third-party footprints/symbols (if applicable).
Hardware design files in this directory (KiCad schematics/PCB and related design sources)
are licensed under CERN-OHL-S-2.0 (Strongly Reciprocal).

Manufacturing outputs (Gerbers) are provided for convenience.

See THIRD_PARTY_NOTICES.md for any third-party footprints/symbols (if applicable).
