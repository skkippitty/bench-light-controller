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
- MOSFET selection unresolved: the 2N7000 is logic-level but rated
  ~200 mA; the BUZ10 has ample capacity but is not logic-level and needs
  V_DS measured at 5 V gate drive before it can be trusted.
- Gate voltage after the fix has not been re-measured. Expected ~4.9 V.
- D1 has not been checked for damage following the reversed-polarity test.
- Stage 1 and Stage 2 measurement columns remain unfilled; the plan and
  predictions are recorded but the readings have not been taken.
- Debounce timing has not been characterised against real contact bounce.

## References
