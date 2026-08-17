# Bench Light Controller

## Overview

## Hardware

## Stages

### Stage 1 — Digital Input

### Stage 2 — PWM Output

### Stage 3 — ADC and Analogue Input

### Stage 4 — MOSFET Load Switching

## Measurements

## Design Decisions

## Known Limitations

- Stage 4 root cause is recorded but the exact conduction path is
  unconfirmed — see the note in docs/measurements.md.
- `MIN_DUTY` is set to 0 pending a stall-threshold measurement on the
  motor, so the dead-zone clamp is currently inactive.
- The BUZ10 is not a logic-level part and is driven at 4.9 V rather than
  the 10 V at which its R_DS(on) is specified. It conducts adequately for
  this load, but V_DS at full duty has not been measured, so full
  enhancement is unverified.
- D1 has not been checked for damage following the reversed-polarity test.
- Stage 1 and Stage 2 measurement columns remain unfilled; the plan and
  predictions are recorded but the readings have not been taken.
- Debounce timing has not been characterised against real contact bounce.

## References
