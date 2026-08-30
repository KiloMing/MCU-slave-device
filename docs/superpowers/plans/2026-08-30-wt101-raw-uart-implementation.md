# WT101 Raw UART Readout Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Read WT101 register 0x3F as two original bytes and transmit status, L, H, RAW, and original-formula YAW through USART2 every 200 ms.

**Architecture:** Extend the focused WT101 hardware driver with a status-returning raw sample API while retaining the existing `WT101_ReadYaw()` compatibility function. The isolated WT101 application mode consumes the new API, waits 1000 ms before its first read, and emits either one success line containing the full sample or one error line containing the HAL status.

**Tech Stack:** STM32F103C8, STM32 HAL, I2C1, USART2 TTL, C99, Keil MDK/ARMCC 5.06.

**Spec:** `docs/superpowers/specs/2026-08-30-wt101-raw-uart-design.md`

## Global Constraints

- Work only in `new_project`; `src` and `source` remain unchanged reference material.
- I2C1 remains PB6 SCL and PB7 SDA at 100 kHz.
- WT101 remains at 7-bit address `0x50`; yaw register remains `0x3F`.
- Preserve low-byte-first combination: `RAW = L | (H << 8)`.
- Preserve conversion: `YAW = RAW / 32768.0 × 180.0`.
- USART2 remains PA2 TX and PA3 RX at 115200 8N1 TTL.
- Send one line every 200 ms and wait 1000 ms before the first read.
- Do not enable motors, heading PID, mecanum kinematics, or upper-computer parsing.
- Every modified source/header file must begin with pin resources, peripheral, function, purpose, and migration source.
- Do not delete or clean existing generated files, and do not commit or push unless the user separately requests it.

## File Structure

- Modify `new_project/Hardware/WT101.h`: define the raw sample record and the status-returning read interface.
- Modify `new_project/Hardware/WT101.c`: perform the original two-byte read once, expose its HAL status, fill the sample only on success, and retain the compatibility wrapper.
- Modify `new_project/App/App_Test.c`: add the startup wait and format the confirmed success/error UART lines.
- Modify `new_project/App/App_Test.h`: update the file header so it describes the raw WT101 output accurately; exported test function names remain unchanged.
- Modify `new_project/User/main.c`: update header documentation only so it no longer claims the test emits only `YAW=xx.xx`.
- Do not modify `new_project/project.uvprojx`: all required files are already in the Keil project.

---

### Task 1: Add a status-aware WT101 raw sample interface

**Files:**
- Modify: `new_project/Hardware/WT101.h`
- Modify: `new_project/Hardware/WT101.c`
- Modify temporarily, then complete in Task 2: `new_project/App/App_Test.c`

**Interfaces:**
- Consumes: global `I2C_HandleTypeDef hi2c1` from `I2C.h`; HAL `HAL_I2C_Mem_Read()`.
- Produces: `WT101_YawSample_t` and `HAL_StatusTypeDef WT101_ReadYawRaw(WT101_YawSample_t *sample)`.
- Preserves: `float WT101_ReadYaw(void)` for later original-code migration compatibility.

- [ ] **Step 1: Declare the sample type and new driver API first**

Replace the declaration section of `WT101.h` with the following, retaining and updating its required file header:

```c
#ifndef WT101_H
#define WT101_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

typedef struct
{
    uint8_t low_byte;
    uint8_t high_byte;
    uint16_t raw;
    float yaw_deg;
} WT101_YawSample_t;

HAL_StatusTypeDef WT101_ReadYawRaw(WT101_YawSample_t *sample);
float WT101_ReadYaw(void);

#endif
```

- [ ] **Step 2: Switch the application call temporarily and verify the missing implementation is detected**

In `App_Test_WT101_RunStep()`, temporarily declare `WT101_YawSample_t sample;` and call `WT101_ReadYawRaw(&sample);` without formatting it yet.

Run:

```powershell
& 'E:\K\UV4\UV4.exe' -b 'E:\project_M\new_project\project.uvprojx' -j0 -o 'E:\project_M\new_project\Objects\codex_wt101_raw_build.log'
```

Expected: the build must fail at link time with an undefined reference for `WT101_ReadYawRaw`. This proves the new application dependency is real before implementing it.

- [ ] **Step 3: Implement the minimal raw read and preserve the original wrapper**

Replace the functional body of `WT101.c` with the following, retaining and updating its required file header:

```c
#include "WT101.h"
#include "I2C.h"

#define WT101_ADDR_7BIT 0x50U
#define WT101_YAW_REG   0x3FU

HAL_StatusTypeDef WT101_ReadYawRaw(WT101_YawSample_t *sample)
{
    uint8_t data[2] = {0U, 0U};
    HAL_StatusTypeDef status;

    if (sample == NULL)
    {
        return HAL_ERROR;
    }

    sample->low_byte = 0U;
    sample->high_byte = 0U;
    sample->raw = 0U;
    sample->yaw_deg = 0.0f;

    status = HAL_I2C_Mem_Read(&hi2c1,
                              (WT101_ADDR_7BIT << 1),
                              WT101_YAW_REG,
                              I2C_MEMADD_SIZE_8BIT,
                              data,
                              2U,
                              100U);
    if (status != HAL_OK)
    {
        return status;
    }

    sample->low_byte = data[0];
    sample->high_byte = data[1];
    sample->raw = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
    sample->yaw_deg = ((float)sample->raw / 32768.0f) * 180.0f;
    return HAL_OK;
}

float WT101_ReadYaw(void)
{
    WT101_YawSample_t sample;

    if (WT101_ReadYawRaw(&sample) != HAL_OK)
    {
        return 0.0f;
    }

    return sample.yaw_deg;
}
```

- [ ] **Step 4: Compile the driver interface and inspect the build result**

Run the same Keil build command from Step 2.

Expected: `WT101.c` compiles and the previous undefined-reference error disappears. `App_Test.c` may still require the Task 2 formatting completion, but there must be no type mismatch involving `WT101_YawSample_t` or `WT101_ReadYawRaw`.

- [ ] **Step 5: Review the scoped driver diff without committing it**

Run:

```powershell
git -C 'E:\project_M' diff -- new_project/Hardware/WT101.h new_project/Hardware/WT101.c
```

Expected: only the status-aware raw API, original byte combination/formula, compatibility wrapper, and accurate file headers appear. Do not commit or push.

---

### Task 2: Emit the confirmed raw sample line every 200 ms

**Files:**
- Modify: `new_project/App/App_Test.c`
- Modify: `new_project/App/App_Test.h`
- Modify: `new_project/User/main.c`

**Interfaces:**
- Consumes: `HAL_StatusTypeDef WT101_ReadYawRaw(WT101_YawSample_t *sample)` from Task 1; `UART_SendString(const char *text)`; `HAL_Delay(uint32_t delay)`.
- Produces: unchanged `void App_Test_WT101_Init(void)` and `void App_Test_WT101_RunStep(void)` behavior with the confirmed UART protocol.

- [ ] **Step 1: Add the 1000 ms startup wait**

Make `App_Test_WT101_Init()` exactly:

```c
void App_Test_WT101_Init(void)
{
    MX_I2C1_Init();
    UART_Init(115200U);
    UART_SendString("WT101 I2C TEST READY\r\n");
    HAL_Delay(1000U);
}
```

This keeps the existing ready message and ensures the first sensor transaction occurs at least one second after initialization.

- [ ] **Step 2: Format success and failure as separate protocol lines**

Replace `App_Test_WT101_RunStep()` with:

```c
void App_Test_WT101_RunStep(void)
{
    char line[80];
    WT101_YawSample_t sample;
    HAL_StatusTypeDef status = WT101_ReadYawRaw(&sample);

    if (status == HAL_OK)
    {
        uint32_t centideg = (uint32_t)(sample.yaw_deg * 100.0f);

        (void)snprintf(line,
                       sizeof(line),
                       "WT101 OK L=0x%02X H=0x%02X RAW=%u YAW=%lu.%02lu\r\n",
                       (unsigned int)sample.low_byte,
                       (unsigned int)sample.high_byte,
                       (unsigned int)sample.raw,
                       (unsigned long)(centideg / 100U),
                       (unsigned long)(centideg % 100U));
    }
    else
    {
        (void)snprintf(line,
                       sizeof(line),
                       "WT101 ERR=%u\r\n",
                       (unsigned int)status);
    }

    UART_SendString(line);
    HAL_Delay(200U);
}
```

- [ ] **Step 3: Update the required file headers**

In `App_Test.c`, `App_Test.h`, and `main.c`, keep the existing pin/peripheral sections and make their WT101 behavior state all of the following exactly in meaning:

```text
I2C1: PB6 SCL, PB7 SDA; WT101 address 0x50, yaw register 0x3F.
USART2 TTL: PA2 TX, PA3 RX; 115200 8N1.
Purpose: isolated WT101 raw-register and serial-output validation.
Output: status plus L/H/RAW/YAW every 200 ms after a 1000 ms startup wait.
Migration: original address, register, low-byte-first order, and yaw formula remain unchanged.
```

Do not alter UART, motor, or mode-selection function declarations while updating those comments.

- [ ] **Step 4: Build the complete firmware**

Run:

```powershell
& 'E:\K\UV4\UV4.exe' -b 'E:\project_M\new_project\project.uvprojx' -j0 -o 'E:\project_M\new_project\Objects\codex_wt101_raw_build.log'
Get-Content -LiteralPath 'E:\project_M\new_project\Objects\codex_wt101_raw_build.log' -Tail 40
```

Expected: the final build line reports `0 Error(s), 0 Warning(s)` and generates `new_project/Objects/project.axf`.

- [ ] **Step 5: Check the exact output literals and timing constants statically**

Run:

```powershell
rg -n "WT101 OK|WT101 ERR|HAL_Delay\(1000U\)|HAL_Delay\(200U\)|WT101_ReadYawRaw" 'E:\project_M\new_project\App' 'E:\project_M\new_project\Hardware'
```

Expected: exactly one success format, one error format, one 1000 ms startup delay, one 200 ms loop delay, and matching declaration/call/definition sites for `WT101_ReadYawRaw`.

- [ ] **Step 6: Review the complete source-only diff without committing it**

Run:

```powershell
git -C 'E:\project_M' diff -- new_project/Hardware/WT101.h new_project/Hardware/WT101.c new_project/App/App_Test.h new_project/App/App_Test.c new_project/User/main.c
```

Expected: no motor values, motor timing, UART pin mapping, I2C address, yaw register, byte order, or conversion constant changed. Do not commit or push.

---

### Task 3: Perform the hardware acceptance test

**Files:**
- Observe: `new_project/Objects/project.axf`
- Observe: serial terminal output at 115200 8N1

**Interfaces:**
- Consumes: the built firmware from Task 2 and the physical WT101/USB-TTL wiring.
- Produces: evidence that valid samples change with rotation, failures are distinguishable, and communication recovers after wiring restoration.

- [ ] **Step 1: Confirm safe wiring before power-up**

Use these exact connections:

```text
WT101 SCL  -> PB6
WT101 SDA  -> PB7
WT101 GND  -> development-board GND
WT101 VCC  -> the supply voltage specified by the user's WT101 module documentation

Board PA2 TX -> USB-TTL RX
Board PA3 RX -> USB-TTL TX
Board GND    -> USB-TTL GND
```

Keep the four-motor mode disabled and do not run a motor acceptance test in this phase.

- [ ] **Step 2: Flash and observe normal output**

Flash `new_project/Objects/project.axf`, reset the board, and open a 115200 8N1 terminal.

Expected sequence:

```text
WT101 I2C TEST READY
WT101 OK L=0x.. H=0x.. RAW=.... YAW=...
```

The first sample must appear about one second after the ready line, followed by approximately one line every 200 ms.

- [ ] **Step 3: Verify live raw-data response**

Hold WT101 still for several samples, then rotate it slowly through different headings.

Expected: `L`, `H`, `RAW`, and `YAW` vary with rotation; the displayed `RAW` equals `L | (H << 8)` for captured lines; no motor moves.

- [ ] **Step 4: Verify explicit failure and recovery**

With power safely controlled, interrupt SDA or SCL and observe at least two output periods, then restore the connection.

Expected during interruption:

```text
WT101 ERR=1
```

`ERR=2` or `ERR=3` is also acceptable if HAL reports busy or timeout. No failed transaction may be printed as a successful zero-valued sample.

Expected after restoration: later cycles return automatically to `WT101 OK ...` without a firmware reset.

- [ ] **Step 5: Record the acceptance result without changing program data**

Record the build result, observed period, at least one still sample, at least one rotated sample, the observed error code, and recovery result in the user's migration log location when requested. Do not adjust addresses, formulas, or timing based only on one reading, and do not commit or push without a separate request.
