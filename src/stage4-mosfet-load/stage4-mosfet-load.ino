const int BUTTON_PIN = 2;
const int GATE_PIN   = 9;
const int POT_PIN    = A0;

// Minimum PWM value that reliably turns the motor.
// Set to 0 until measured — Tinkercad does not simulate static friction,
// so no stall threshold exists in simulation. Measure on hardware and update.
const int MIN_DUTY   = 0;

void setup()
{
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(GATE_PIN, OUTPUT);
  // No pinMode() needed for POT_PIN
}

void loop()
{
  // ---- STATE ----
  static int lastButtonState = LOW;
  static bool motorIsOn = false;
  static unsigned long lastChangeTime = 0;
  static int dutyCycle = 0;

  // ---- BUTTON ----
  int currentButtonState = digitalRead(BUTTON_PIN);
  if (currentButtonState != lastButtonState) {
    if (millis() - lastChangeTime > 50) {
      if (currentButtonState == HIGH) {
        motorIsOn = !motorIsOn;
      }
      lastChangeTime = millis();
    }
    lastButtonState = currentButtonState;
  }

  // ---- ADC READ ----
  int potValue = analogRead(POT_PIN);

  // ---- RANGE MAPPING ----
  dutyCycle = map(potValue, 0, 1023, 0, 255);

  // ---- DEAD ZONE HANDLING (CLAMP STRATEGY) ----
  // If the knob is slightly turned but below minimum threshold, clamp it up to MIN_DUTY.
  // If the knob is fully at zero, leave it at zero so the user can turn it off via the dial.
  if (dutyCycle > 0 && dutyCycle < MIN_DUTY) {
    dutyCycle = MIN_DUTY;
  }

  // ---- DIAGNOSTICS ----
  Serial.print("POT: ");
  Serial.print(potValue);
  Serial.print(" \t MAP: ");
  Serial.print(dutyCycle);
  Serial.print(" \t STATE: ");
  Serial.println(motorIsOn ? "ON" : "OFF");

  // ---- OUTPUT ----
  analogWrite(GATE_PIN, motorIsOn ? dutyCycle : 0);
}
