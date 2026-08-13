const int BUTTON_PIN = 2;
const int LED_PIN    = 9;
const int POT_PIN    = A0;

void setup()
{
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  // No pinMode() needed for POT_PIN
}

void loop()
{
  // ---- STATE ----
  static int lastButtonState = LOW;
  static bool lightIsOn = false;
  static unsigned long lastChangeTime = 0;
  static int brightness = 0; // Overwritten immediately by the ADC read

  // ---- BUTTON ----
  int currentButtonState = digitalRead(BUTTON_PIN);

  if (currentButtonState != lastButtonState) {
    if (millis() - lastChangeTime > 50) {
      if (currentButtonState == HIGH) {
        lightIsOn = !lightIsOn;
      }
      lastChangeTime = millis();
    }
    lastButtonState = currentButtonState;
  }

  // ---- ADC READ ----
  int potValue = analogRead(POT_PIN);

  // ---- RANGE MAPPING ----
  brightness = map(potValue, 0, 1023, 0, 255);

  // ---- DIAGNOSTICS ----
  Serial.print("POT: ");
  Serial.print(potValue);
  Serial.print(" \t MAP: ");
  Serial.print(brightness);
  Serial.print(" \t STATE: ");
  Serial.println(lightIsOn ? "ON" : "OFF");

  // ---- OUTPUT ----
  analogWrite(LED_PIN, lightIsOn ? brightness : 0);
}
