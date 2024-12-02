#include <Arduino.h>
#ifdef QUICKTEST

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("Hi");
  pinMode(LED_BUILTIN, OUTPUT);
  for(int i = 0; i < LED_BUILTIN; i++) {
    pinMode(i, INPUT);    // sets the digital pin 7 as input
  }


}

void loop()
{

  Serial.print("voltages");
  // turn the LED on (HIGH is the voltage level)
  digitalWrite(LED_BUILTIN, HIGH);
  for(int i = 0; i < 8; i++) {
      Serial.print(",");
      Serial.print(analogRead(i));
  }
  Serial.println("");
  Serial.print("io");
  for(int i = 0; i < LED_BUILTIN; i++) {
      Serial.print(",");
      Serial.print(digitalRead(i));
  }
  Serial.println("");

  // wait for a second
  delay(100);
  // turn the LED off by making the voltage LOW
  digitalWrite(LED_BUILTIN, LOW);
   // wait for a second
  delay(100);
}

#endif