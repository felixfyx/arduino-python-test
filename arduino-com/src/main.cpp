#include <Arduino.h>

const int id = 1; // Unique ID for each Arduino board

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
  if (Serial.available() > 0) {
    char incomingByte = Serial.read();
    if (incomingByte == 'H') { // Handshake request from Python
      Serial.print("A"); // Response to indicate Arduino is connected
      Serial.print(id); // Send unique ID
    } else if (incomingByte == 'C') { // Connection request from Python
      char idByte = Serial.read(); // Read the ID sent by Python
      if (String(idByte) == String(id)) { // Check if the ID matches
        Serial.print("C"); // Response to indicate connection established
      } else {
        Serial.print("E" + String(idByte)); // Response to indicate connection failed
      }
    }
  }

  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
  digitalWrite(LED_BUILTIN, HIGH);
}