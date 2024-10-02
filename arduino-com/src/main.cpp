#include <Arduino.h>

const int id = 1; // Unique ID for each Arduino board
uint8_t _inputBuffer[64]; 
uint8_t _bufferIndex = 0;
bool _startByteReceived = false;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  digitalWrite(LED_BUILTIN, LOW);
}

/*
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
        digitalWrite(LED_BUILTIN, HIGH);
      } else {
        Serial.print("E" + String(idByte)); // Response to indicate connection failed
      }
    }
  }

  delay(100);
}
*/

void WriteBackHandshake()
{
  uint8_t buffer[2] = {0, 0};
  buffer[0] = 0xAA;
  buffer[1] = id;
  Serial.write(buffer, 2);
}

void loop()
{
  while(Serial.available())
  {
    uint8_t byteReceived = Serial.read();
    if (!_startByteReceived)
    {
      if (byteReceived == 0xAA) {
        _startByteReceived = true;
        _bufferIndex = 0;
        _inputBuffer[_bufferIndex++] = byteReceived;
      }
    }
    else
    {
      _inputBuffer[_bufferIndex++] = byteReceived;
      if (_bufferIndex == 2) {
        if (_inputBuffer[1] < 4 || _inputBuffer[1] > sizeof(_inputBuffer)) {
          // Invalid length, reset buffer
          _startByteReceived = false;
          _bufferIndex = 0;
        }
      }

      if (_startByteReceived && _bufferIndex >= _inputBuffer[1]) {
        uint8_t length = _inputBuffer[1];
        uint8_t identifier = _inputBuffer[2];
        uint8_t checksum = _inputBuffer[length - 1];
        
        // Verify checksum
        uint8_t calculatedChecksum = 0;
        for (int i = 0; i < length - 1; i++) {
          calculatedChecksum ^= _inputBuffer[i];
        }

        if (checksum != calculatedChecksum) {
          _startByteReceived = false;
          _bufferIndex = 0;

          return;
        }

        if (identifier == 0x01)
        {
          // Write back to HSP
          WriteBackHandshake();
          digitalWrite(LED_BUILTIN, HIGH);
        }
      }
    }

    // Prevent buffer overflow
    if (_bufferIndex >= sizeof(_inputBuffer)) {
      _startByteReceived = false;
      _bufferIndex = 0;
    }
  }
}