const int BUTTON_PIN = 2;
const int LED_PIN    = 9;
const int DEFAULT_BRIGHTNESS = 128;  // Named constant for default brightness

void setup()
{
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
}

void loop()
{
  static int lastButtonState = LOW;
  static bool lightIsOn = false;
  static unsigned long lastChangeTime = 0;
  static int brightness = DEFAULT_BRIGHTNESS; // Persists between loop iterations

  int currentButtonState = digitalRead(BUTTON_PIN);

  if (currentButtonState != lastButtonState) {
    if (millis() - lastChangeTime > 50) {   // enough time elapsed?
      if (currentButtonState == HIGH) {
        lightIsOn = !lightIsOn;
      }
      lastChangeTime = millis();
    }
    lastButtonState = currentButtonState;
  }

  // Uses the brightness variable dynamically instead of a hardcoded literal
  analogWrite(LED_PIN, lightIsOn ? brightness : 0);
}
