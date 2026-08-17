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

## Gate drive components

Two resistors sit on the gate, each solving a distinct problem.

Series resistor, R2, 220 Ω, between D9 and the gate. A MOSFET gate is capacitive. Charging a capacitor through zero resistance draws a large instantaneous current, potentially exceeding the pin's rating on every switching transition. R2 limits that surge. It slows switching slightly, an acceptable trade at the Uno's default PWM frequency of roughly 490 Hz where the period is around 2 ms and the transition is orders of magnitude shorter.

Pull-down resistor, R3, 10 kΩ, gate to source. Before setup() runs, the Arduino's pins are inputs — high impedance, effectively floating. A floating gate on a capacitive node can leave the device in an indeterminate state, so the load could twitch or run at power-up. R3 guarantees the gate sits at ground until firmware deliberately drives it.

R3 spans the gate column to the source column rather than running a separate jumper to the ground rail. The source is already tied to ground, so this reaches the same net topology with one fewer wire.

This is the same floating-input principle as the Stage 1 button pull-down, appearing in a different context. The value follows the same reasoning — weak enough not to waste current, strong enough to dominate leakage. With R2 at 220 Ω the divider leaves the gate at approximately 4.9 V when driven.

## Load switching topology

Separate supply rails, common ground. The Arduino's 5 V rail powers the potentiometer and button. A separate 6 V pack — four 1.5 V cells in series — powers the motor. The two positive rails are never connected.

The grounds must be common, and this is not optional. A MOSFET switches on V_GS, the potential difference between gate and source. If the two supplies' grounds float relative to each other, V_GS has no defined value and the circuit behaves erratically or not at all. On a full-size breadboard the upper and lower ground rails are electrically separate, so an explicit jumper links them.

The corresponding hazard: the load supply positive must never reach the Arduino's 5 V pin. That pin is an output from the onboard regulator, and back-feeding it puts the regulator outside its design conditions with no protection in the path.

Flyback diode, D1, across the motor. A motor is an inductor, and V = L(dI/dt) means its current cannot change instantaneously. Switching the MOSFET off attempts to take that current to zero rapidly, and the inductor generates whatever voltage is needed to maintain it — potentially hundreds of volts, opposite in polarity to the supply.

D1 sits across the motor, reverse-biased in normal operation, banded end (cathode) toward the positive rail. It does nothing while the motor runs. At switch-off, when the motor drives its low side above the supply, the diode forward-biases and provides a circulating path for the decaying current, clamping the spike to roughly one diode drop above the rail.

The 1N4001 to hand is adequate here: 50 V reverse, 1 A forward, against a 6 V supply and a circulating current that cannot exceed the motor's running current. Worth noting as a limitation that the 1N400x family are standard rectifiers with reverse recovery around 30 µs. At 490 Hz that is negligible against a 2 ms period, but at the tens of kilohertz used for motor control it would consume a meaningful fraction of each cycle, and a fast-recovery or Schottky part would be needed instead.

## MOSFET selection: unresolved

Two parts are available and neither is straightforwardly correct.

2N7000 (TO-92). Logic-level: V_GS(th) around 0.8–3 V, specified at V_GS = 4.5 V, so a 5 V pin turns it on properly. But continuous drain current is roughly 200 mA and R_DS(on) around 5 Ω at 4.5 V gate drive. A hobby motor draws hundreds of milliamps running and can exceed an amp at stall, so this part sits outside its rating on every start-up.

BUZ10 (TO-220). Ample capacity — around 23 A continuous, 50 V drain-source, R_DS(on) approximately 0.1 Ω. But that figure is specified at V_GS = 10 V and the part is not logic-level. Its threshold spread of roughly 2.1 V to 4 V is a manufacturing tolerance rather than a selectable range, so behaviour at 5 V gate drive cannot be predicted from the datasheet alone.

Decision deferred pending measurement. The resolution is empirical: run at full duty and measure V_DS drain to source. Under about 0.3 V indicates adequate enhancement. Above 1 V, or a warm tab, indicates linear-region operation rather than switching, dissipating power as heat.

Effective R_DS(on) can then be derived from V_DS and the measured load current and compared against the datasheet figure. The gap between them quantifies the cost of under-driving the gate.

If the BUZ10 proves marginal, the textbook remedy is a gate driver — a small transistor switching the gate from a higher rail. That is a worthwhile extension in itself, since gate drive recurs throughout power electronics.

## Dead zone handling: clamp versus remap

A motor needs a minimum torque to overcome static friction, so below some duty cycle it draws current and produces heat without turning. Part of the potentiometer's travel therefore produces no useful output.

Clamp (chosen). Map the ADC reading to the full 0–255 range, then force any non-zero value below MIN_DUTY up to MIN_DUTY. A potentiometer at zero still means fully off; anything above zero jumps to the slowest speed the motor can hold.

Remap (rejected). Change the mapping's output range so the potentiometer's full sweep spans MIN_DUTY to 255. Smooth throughout, but the motor can then never be stopped by the dial alone — only by the button.

The clamp was chosen because it preserves "off" as a state reachable from the potentiometer, matching how a physical dimmer or speed control behaves. The cost is that the bottom of the dial produces identical output, visible in the serial log as POT climbing while MAP stays pinned.

MIN_DUTY is currently 0, which makes the clamp a no-op, because the stall threshold has not yet been measured. The constant is deliberately left at 0 rather than populated with a plausible guess: a value that looks empirical but is not is more misleading than one that is obviously unset.

## Firmware unchanged from Stage 3

The only firmware changes between Stage 3 and Stage 4 were renaming LED_PIN to GATE_PIN, brightness to dutyCycle and lightIsOn to motorIsOn, plus adding the clamp.

That is the intended result rather than a shortcut. The firmware controls a duty cycle on pin 9. Whether that duty cycle switches 14 mA through an LED or amps through a motor is an electrical concern the software never sees. The load was replaced; the control logic was not.

The renames were still worth making. Names that lag behind what the code does are a recurring source of confusion, and LED_PIN driving a MOSFET gate is exactly that.
