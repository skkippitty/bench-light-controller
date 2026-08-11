# Design Decisions

---

## Pull-down resistor sizing

A digital input pin has very high input impedance. Left unconnected it
floats: stray charge, capacitive coupling and mains pickup are enough to
swing it across the logic threshold, and it reads unpredictably. The
button alone cannot fix this, because a simple switch only defines the
pin's voltage in one of its two states — pressed. Released, the pin
connects to nothing.

A pull-down resistor from the pin to ground gives the node a defined
level whenever the switch is open, while the switch's direct connection
to 5 V dominates when closed.

**Value chosen: 10 kΩ.**

The choice is a compromise between two failure modes:

- **Too small.** At 100 Ω the node would draw 5 / 100 = 50 mA
  continuously while the button is held. The ATmega328P has a total
  budget of roughly 200 mA across the entire chip, so a single input
  would consume a quarter of it doing nothing useful.
- **Too large.** At 1 MΩ the resistor becomes weak enough that leakage
  currents and coupled noise compete with it, and the input drifts back
  toward floating behaviour.

At 10 kΩ the node draws 5 / 10000 = 0.5 mA when pressed — negligible
against the chip budget, and far stronger than any leakage path.

The general principle generalises well beyond this circuit: a biasing
resistor must be weak enough not to waste current, and strong enough to
dominate leakage.

### Fault found: pull-down bypassed by a redundant ground connection

The first schematic revision short-circuited the supply whenever the
button was pressed. In simulation the microcontroller failed
immediately.

The cause was a wire running directly from the junction of the switch
and the resistor to the ground rail, in addition to the resistor's own
path to ground. That junction is the sense node feeding D2. With a
second, unresisted path to ground:

- the 10 kΩ had ground on both terminals and carried no current at all,
  so the pull-down function was absent entirely;
- closing the switch connected 5 V to ground through bare wire.

The intended current on that path was 0.5 mA. With the resistor
bypassed, the only limit was the resistance of the wiring itself — a
fraction of an ohm — placing the current orders of magnitude above the
device's total rating.

**Fix:** the redundant wire was deleted, leaving the 10 kΩ as the sole
connection between the sense node and ground.

**Check adopted as a result:** before energising any circuit, trace every
path from supply to ground and identify what limits the current on each.
A path containing no resistance, no load and no open switch is a short.
On this circuit the two valid paths are through the 10 kΩ when the button
is closed, and through the 220 Ω and LED when pin 9 is driven high.

---

## Debounce approach

A mechanical switch does not close cleanly. The contacts bounce for
somewhere between 1 ms and 20 ms, producing a burst of transitions
before settling. Stage 1 was unaffected, because asking "is the button
currently held?" tolerates a few milliseconds of noise. Stage 2 asks the
button to *toggle* a state, where each spurious transition produces an
unwanted state change.

**Approach chosen: timestamp comparison using `millis()`, 50 ms window,
acting on the rising edge only.**

Three decisions sit inside that:

**Edge detection rather than level detection.** The toggle must fire once
per press, not continuously while held. This requires comparing the
current reading against the previous one, which in turn requires the
previous reading to survive between calls to `loop()`. A `static` local
was used rather than a file-scope global: it persists across calls, as a
global would, but its scope remains confined to the function, so no other
code can modify it.

**Rising edge only.** A state change fires on both press and release.
Acting on both would toggle twice per press. With a pull-down
configuration, a press drives the pin HIGH, so the toggle is gated on
`currentButtonState == HIGH`.

**`millis()` rather than `delay()`.** `delay()` blocks — nothing else
executes for its duration. That is tolerable in a circuit this simple,
but the same debounce pattern is needed in the environmental monitoring
station, where a blocking wait would stall sensor sampling. Recording a
timestamp and comparing elapsed time on subsequent passes is
non-blocking and reuses directly.

The timestamp is stored as `unsigned long`, matching the return type of
`millis()`. Storing it in an `int` would overflow after roughly 32
seconds and break the comparison silently.

### Known weakness in the current implementation

`lastButtonState` is updated inside the "state changed" branch but
outside the timing check. A bounce transition rejected on timing grounds
therefore still updates the stored state.

The alternative structure — used in Arduino's own Debounce example —
resets the timer on *every* observed transition and commits to a new
state only once the line has been quiet for the full window. That is
more robust under sustained contact noise.

The simpler structure was retained for now because it is easier to
reason about and adequate for normal presses. It has not been validated
against real contact bounce; see Known Limitations.

---

## Brightness gating

The output stage needs to honour two independent inputs: a toggle state
that says whether the light is on, and a brightness level. Two
placements are possible:

1. Gate the `analogWrite()` call — branch on the toggle state and write
   either the brightness value or zero.
2. Modify the brightness variable before writing — zero it when the
   toggle is off, then write unconditionally.

**Chosen: option 1**, expressed as a ternary at the point of output.

The reasoning is that brightness and power state remain independent
values throughout. Option 2 destroys the brightness setting when the
light is switched off, so the previous level cannot be restored on the
next press without storing it separately. Keeping the gate at the output
means the variable always holds the user's intended level regardless of
power state.

This also anticipates a planned extension: fading out on power-off
requires reading the current brightness at the moment the toggle
changes. That value must still exist.

---

## Rejected alternatives

**External pull-up instead of pull-down.** A pull-up is the more common
industry convention, partly because the ATmega328P provides internal
pull-ups requiring no external component, and partly because ground is
generally a cleaner reference than the supply rail in a noisy system.
A pull-down was used here deliberately: the inverted logic of a pull-up
(pressed reads LOW) obscures the relationship between circuit and code
while that relationship is still being learned. `INPUT_PULLUP` is
scheduled as a follow-up exercise.

**Hardware RC debounce instead of software.** A resistor–capacitor
network across the switch, optionally followed by a Schmitt trigger,
filters bounce before it reaches the pin. It was rejected here because
it costs components and because the software timing pattern transfers
directly to other timing problems in this project family. Hardware
debouncing is planned as a comparative exercise in a later tier, where
the two approaches can be measured against each other on a scope.

**Pin 13 for the LED.** Retained through Stage 1, then abandoned. Pin 13
is not connected to a hardware timer output and therefore cannot produce
hardware PWM; the LED was moved to pin 9. Pin 13 also carries an onboard
LED in parallel, which is convenient for confirming the output stage
works but adds a second load to any current measurement taken there.

**Software PWM by toggling a pin in a loop.** Possible on any pin, and
briefly considered to avoid moving the LED. Rejected because the timing
jitters whenever other code runs, and because the CPU is fully occupied
generating the waveform. Hardware timers produce the waveform
autonomously once configured, which is the more useful pattern to learn.

**`delay()`-based debounce.** Simpler to write and adequate in
isolation. Rejected for the reasons given under Debounce approach —
principally that the non-blocking pattern is required later and is worth
learning once.
