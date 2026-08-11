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

- Measurements have not yet been performed. docs/measurements.md sets
  out the test plan and the predicted values; the results columns are
  filled in as each measurement is taken.
- Debounce logic is written but unvalidated. Tinkercad models the
  switch as ideal, so no contact bounce is generated and the timing
  window has never actually rejected anything. To be characterised on
  hardware.
- The debounce implementation updates the stored button state outside
  the timing check, so a transition rejected on timing still advances
  it. See docs/design-decisions.md for the more robust alternative and
  why it was deferred.
- LED forward voltage is assumed at 2.0 V rather than measured, giving
  roughly ±7% uncertainty in the predicted branch current.
- Stage 4 MOSFET selection is unresolved. The 2N7000 to hand is
  logic-level but rated ~200 mA; the BUZ10 has ample current capacity
  but is not logic-level and needs V_DS measured at 5 V gate drive
  before use with an inductive load.

## References
