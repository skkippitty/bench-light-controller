# Measurement Plan

This document specifies the measurements to be taken at each stage, the
predicted value for each, and the reasoning behind the prediction.
Results columns are filled in as measurements are performed.

Predictions are recorded **before** measurement deliberately. A
prediction that turns out wrong is more informative than one that turns
out right, and recording it afterwards defeats the purpose.

Simulated instruments are idealised. Measurements marked **[hardware]**
cannot be meaningfully taken in Tinkercad and are deferred until
physical components are available — see Known Limitations in the README.

---

## Stage 1 — Digital input levels

| Measurement | Node | Predicted | Measured | Reasoning |
|---|---|---|---|---|
| Input voltage, button released | D2 → GND | 0 V | | Pull-down ties the node to ground when the switch is open |
| Input voltage, button pressed | D2 → GND | 5 V | | A direct connection dominates a 10 kΩ resistor |
| `digitalRead()` return, released | D2 | 0 (LOW) | | Should follow from the voltage above |
| `digitalRead()` return, pressed | D2 | 1 (HIGH) | | |
| LED branch current | D13 → 220 Ω → LED → GND, meter in series | 13.6 mA | | (5 − 2.0) / 220, assuming V_f = 2.0 V |
| Pull-down current, pressed | 10 kΩ → GND, meter in series | 0.5 mA | | 5 / 10000 |
| Input voltage with pull-down removed | D2 → GND | Undefined | | **[hardware]** Expect drift and hand-proximity sensitivity; the simulator models an ideal open circuit and will not reproduce this |

### Notes on the predictions

The branch current figure assumes a red LED forward voltage of 2.0 V.
Real V_f for red LEDs spans roughly 1.8 V to 2.2 V, giving a current
range of about 12.7 mA to 14.5 mA through a fixed 220 Ω — an uncertainty
of around ±7% inherited entirely from the assumption. Measuring V_f
directly and recalculating would remove it.

The pull-down current is the quantitative answer to "why 10 kΩ and not
100 Ω". At 100 Ω the same node draws 50 mA continuously while held,
against a 200 mA total budget for the whole device.

---

## Stage 2 — PWM duty cycle and averaging

Firmware state: LED on pin 9 (pin 13 has no timer output and cannot
produce hardware PWM), driven via `analogWrite()`.

| Measurement | Node | Predicted | Measured | Reasoning |
|---|---|---|---|---|
| Average voltage at brightness 128 | D9 → GND | 2.51 V | | V_avg = duty × V_supply, duty = 128/255 |
| Average voltage at brightness 64 | D9 → GND | 1.25 V | | Linearity check |
| Average voltage at brightness 192 | D9 → GND | 3.76 V | | Linearity check |
| Average voltage at brightness 128 | LED anode → cathode | ~1.0 V | | See below — this is *not* 2.5 V |
| Waveform levels | D9 → GND, oscilloscope | 0 V and 5 V only | | The output stage cannot produce an intermediate level |
| High-portion width vs period at brightness 128 | D9 → GND, oscilloscope | Approximately equal | | 50% duty cycle |
| PWM period | D9 → GND, oscilloscope | ~2.04 ms | | Corresponds to the Uno's ~490 Hz default on pin 9 |
| Perceptually half-bright duty value | Visual, LED | Well below 128 | | Brightness perception is roughly logarithmic, not linear |

### Why the reading across the LED is predicted at 1 V, not 2.5 V

These two predictions describe different nodes and both should hold.

At pin 9 with respect to ground, the output genuinely alternates between
0 V and 5 V. A slow meter integrates over many cycles and reports
duty × V_supply, so 128/255 × 5 ≈ 2.51 V.

Across the LED the situation differs. The LED drops its forward voltage
— approximately 2 V — only while conducting, and essentially nothing
while the pin is held low. Averaged over the cycle at 50.2% duty, that
gives roughly 2 V × 0.502 ≈ 1.0 V.

The instrument therefore reports a value the LED is never actually at.
This is a bandwidth limitation rather than a fault: a handheld
multimeter integrates over a window far longer than the ~2 ms PWM
period and cannot resolve individual pulses. An oscilloscope on the same
node shows the underlying square wave and resolves the ambiguity.

If the averaging explanation is correct, the LED reading should scale
linearly with duty cycle — approximately 0.5 V at brightness 64 and
1.5 V at 192. That is the test that distinguishes this explanation from
a wiring fault.

The same instrument limitation is why a multimeter cannot capture the
inductive turn-off spike in Stage 4, where the event lasts microseconds.

---

## Stage 3 — ADC readings and noise

Not yet built. The two experiments that carry the most weight here both
require hardware: Tinkercad's ADC model is noiseless, and its simulated
potentiometer reaches both supply rails exactly.

| Measurement | Node | Predicted | Measured | Reasoning |
|---|---|---|---|---|
| ADC resolution | — | 4.89 mV per step | — | 5 V / 1023 |
| Voltage represented by reading 512 | — | 2.50 V | — | (512 / 1023) × 5 |
| Wiper voltage sweep | A0 → GND | 0 V to 5 V | | Potentiometer as an adjustable voltage divider |
| ADC reading at each extreme | A0 | 0 and 1023 | | **[hardware]** Many real pots fall short of both rails; the mapping must tolerate this |
| Spread across 100 static readings, unfiltered | A0 | Last 1–2 bits unstable | | **[hardware]** Supply noise, coupled noise, source impedance |
| Spread across 100 static readings, 0.1 µF wiper to GND | A0 | Reduced | | **[hardware]** RC low-pass with the pot's source impedance |
| Spread across 100 static readings, 16× firmware averaging | A0 | ~4× improvement | | **[hardware]** SNR improves as √N |

### Quantisation

Quantisation error is bounded by ±½ LSB and is irreducible — it is the
cost of digitisation, not a defect, and it is distinct from the jitter
the mitigations above address. The two should not be conflated when
reporting results.

### Range mapping

The ADC produces 1024 distinct values; `analogWrite()` accepts 256. Four
ADC readings therefore collapse onto each PWM value, and `map()`'s
integer division truncates rather than rounds. Working the arithmetic by
hand for readings 0 through 4 shows exactly where the information is
lost.

---

## Stage 4 — Load switching

Not yet built. MOSFET selection is unresolved and depends on the first
two measurements below.

| Measurement | Node | Predicted | Measured | Reasoning |
|---|---|---|---|---|
| Motor winding resistance | Motor terminals, unpowered | — | | Take the lowest of several readings, rotating the shaft between each; brushed motors vary with commutator position |
| Calculated stall current | — | V_supply / R_winding | | Determines whether the 2N7000 (~200 mA) is viable at all |
| V_DS at full duty, BUZ10 | Drain → source | < 0.3 V if adequately enhanced | | **[hardware]** The BUZ10 is not logic-level; its R_DS(on) is specified at V_GS = 10 V, and only measurement establishes behaviour at 5 V |
| Effective R_DS(on) | Derived | Compare against 0.1 Ω datasheet figure | | V_DS / I_load; the gap quantifies the cost of under-driving the gate |
| MOSFET case temperature at full duty | — | Near ambient | | Warmth indicates linear-region operation rather than switching |
| Supply voltage at switch-off, no flyback diode | Load supply | Spike, magnitude unresolvable | | **[hardware]** V = L(dI/dt); a multimeter is too slow to capture it, which is itself the finding |

---

## Stage 4 — Debugging log: motor never turns

All readings in this section were taken on physical hardware.

**Symptom.** Potentiometer readings correct across the full ADC range, button toggle working, serial output reaching MAP: 255 at maximum, but the motor never turned at any duty cycle or toggle state.

Because the firmware and input side were demonstrably working, the fault had to lie downstream of analogWrite(). The measurements below were taken in that order deliberately: prove the load branch first, then the device, then the driving pin, then component values, then continuity.

### Measurements

| # | Measurement | Node | Reading | Interpretation |
|---|---|---|---|---|
| 1 | Voltage | Arduino GND → battery + | 6 V | Grounds are common. A floating supply pair would give an erratic or meaningless value, so this ruled out the most common cause of a non-switching MOSFET |
| 2 | Voltage | Gate → source | 0 V | V_GS = 0, so the device cannot conduct |
| 3 | Voltage | Gate → drain | 6 V | Drain correctly at supply potential through the motor; the load branch is intact |
| 4 | Voltage | D9 → ground, toggle on | 4 V | Pin is driving, but sagging below the expected 5 V — consistent with sourcing real current into a low-impedance path |
| 5 | Voltage | Gate → ground | 0 V | Gate at ground potential despite R2 feeding it from a driven pin |
| 6 | Bridge test | Drain to source | Motor turns | Motor, diode, battery, drain wiring and ground return all confirmed good |
| 7 | Bridge test | Gate to drain | Motor turns | Applying 6 V to the gate switches the device; the MOSFET itself is functional |
| 8 | Resistance, powered | Gate → ground | ~1 Ω, later ~250 Ω | Invalid — see below |
| 9 | Resistance, unpowered | Across R3 leads, in circuit | 0 Ω | Both leads on the same net; the resistor was bridging a single column and acting as a wire |
| 10 | Resistance, unpowered | R3, one lead lifted | 10 kΩ | Component correct; the fault is wiring, not a wrong part |
| 11 | Resistance, unpowered | Gate → ground, after reseating R3 | 10 kΩ | Pull-down correctly in circuit |
| 12 | Polarity test | Battery leads reversed | Motor runs continuously, no gate control | Drain below source forward-biases the MOSFET's intrinsic body diode, which conducts regardless of gate state. Confirms the original orientation was correct |

### Why measurements 2, 4 and 5 were contradictory

With R2 at 220 Ω from a driven 5 V pin and R3 at 10 kΩ to ground, the gate node should sit at

5 × 10000 / (10000 + 220) ≈ 4.9 V

Reading 0 V meant the node had a low-resistance path to ground that the divider calculation did not account for. The sag at D9 from 5 V to 4 V was the same fault seen from the other end: the pin was sourcing real current, which a capacitive MOSFET gate should never demand in steady state.

### Invalid measurement: resistance on a live circuit

Measurement 8 and the repeated ~250 Ω readings were taken with power applied and are meaningless.

A multimeter in resistance mode injects a known test current and infers resistance from the resulting voltage. That inference assumes nothing else is driving the node. With the circuit powered, the gate voltage is set by the Arduino rather than by the meter, so the displayed figure is an artefact. The meter reported an outright error once the button was pressed and current began flowing.

**Rule adopted:** resistance is measured only with all power disconnected. Live-circuit diagnosis uses voltage.

The invalid reading landing near R2's 220 Ω initially suggested a genuine parallel path, since two resistors sharing both nets would give 220 ∥ 10000 ≈ 215 Ω. That hypothesis was only eliminated by re-measuring unpowered, where the correct 10 kΩ appeared.

### Root cause

A supply line was connected the wrong way round. It was not visible on inspection because other jumpers were routed over it.

To be confirmed before this section is treated as final: a reversed supply across the load forward-biases the body diode and produces a permanently-on motor, which is measurement 12's result rather than the original symptom. The reversal was therefore most likely on a different node, and the exact conduction path should be reconstructed.

### Diagnostic sequence

The order proved more useful than any individual reading, and is worth reusing:

1. Prove the load branch by bridging drain to source. If the motor runs, everything from battery through motor to ground is sound and the fault is confined to the gate.
2. Prove the device by bridging gate to drain. If the motor runs, the MOSFET switches and the fault is in the gate drive rather than the transistor.
3. Check the driving pin in isolation. A sagging output voltage shows the pin is loaded, localising the fault to the node it drives.
4. Check component values and wiring against each other. Both were suspected here; measurement settled which.
5. Check continuity with power off — the only condition in which resistance readings mean anything.

### Same fault class as Stage 1

The Stage 1 short had the same underlying shape: a component bypassed by an unintended connection. There, a redundant wire from the sense node to ground shorted across the pull-down. Here, both of R3's leads occupied one breadboard net.

In both cases the component was present, correct, and electrically irrelevant. A resistor with both leads in one column is a wire — current takes the copper strip rather than the resistive element.

**Check adopted:** before applying power, verify every two-terminal component spans two distinct nets. Continuity across the component, with power disconnected, confirms it.

### Outstanding measurements

| Measurement | Node | Expected | Measured | Notes |
|---|---|---|---|---|
| Gate voltage, powered | Gate → ground | ~4.9 V | | Confirms the divider behaves as calculated |
| Duty cycle at first audible buzz | Motor | — | | Current flowing, torque insufficient to overcome static friction |
| Duty cycle at first rotation | Motor | — | | Sets MIN_DUTY, currently 0 pending this figure |
| Duty cycle for steady rotation | Motor | — | | Below this, expect stuttering |
| V_DS at full duty | Drain → source | < 0.3 V if adequately enhanced | | Decisive for the MOSFET selection question |
| Effective R_DS(on) | Derived | Compare against datasheet | | V_DS ÷ measured load current |
| MOSFET case temperature at full duty | — | Near ambient | | Warmth indicates linear-region operation rather than switching |
| D1 forward / reverse resistance | Across D1, unpowered | Conducts one way only | | Worth checking after the polarity error in case of damage |
| Motor winding resistance | Motor terminals, unpowered | — | | Lowest of several readings, rotating the shaft between each |
