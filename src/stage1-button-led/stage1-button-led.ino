void setup()
{
  Serial.begin(9600);
  pinMode(2, INPUT);
  pinMode(13, OUTPUT);
}

void loop()
{
  int buttonState = digitalRead(2);

  if (buttonState == 1) {
    digitalWrite(13, HIGH);
  } else {
    digitalWrite(13, LOW);
  }

  Serial.println(buttonState);

  delay(10);
}
