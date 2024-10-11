/**
 * SerialMessageHandler.cpp
 */

#include "SerialMessageHandler.h"

SerialMessageHandler* SerialMessageHandler::ptr_instance = nullptr;

SerialMessageHandler* SerialMessageHandler::GetInstance()
{
    if (ptr_instance == nullptr)
    {
        // HACK: Hardcoded for now, need to figure out a way
        // to make it more flexible
        ptr_instance = new SerialMessageHandler(Serial, 115200, 1);
    }

    return ptr_instance;
}

SerialMessageHandler::SerialMessageHandler(HardwareSerial& serial, const uint32_t& baudRate, const uint8_t& id)
    : serial(serial), id(id)
{
    cmdList = std::map<uint8_t, Command>();
    serial.begin(baudRate);

    // Register reserved command
    AddCommand(HANDSHAKE_COMMAND, AttemptHandshake);
}

/*
The update loop reads any incoming packet data and process them automatically
*/
void SerialMessageHandler::Update()
{
    bool errorFound = false;
    int messageLength = -1;
    Command cmdFunctionPtr = nullptr;

    std::vector<uint8_t> incomingMessage = std::vector<uint8_t>();
    std::vector<uint8_t> payload = std::vector<uint8_t>();

    while (serial.available() && serial.getWriteError() == 0)
    {
        // NOTE: If all else fails, fill up the buffer then process message
        uint8_t incomingByte = serial.read();

        // TODO: Fix this to fit more of Kim's packet data structure
        // <sync><src><dest><payload length><payload><checksum>
        switch(incomingMessage.size())
        {
            case 0: // The starting byte
                if (!VerifySyncByte(incomingByte))
                {
                    errorFound = true;
                }
                break;
            case 1: // Length
                messageLength = VerifyLengthByte(incomingByte);
                if (messageLength <= -1)
                {
                    errorFound = true;
                }
                break;
            case 2: // Identifier/Command
                cmdFunctionPtr = VerifyIdentifierByte(incomingByte);
                if (cmdFunctionPtr == nullptr)
                {
                    errorFound = true;
                }
                break;
            default: // Last remaining bytes consisting of payload + checksum
                if (incomingMessage.size() < messageLength - 1)
                {
                    payload.push_back(incomingByte);
                }
                break;
        }

        // Found an error, we can ignore everything from here
        if (errorFound)
        {
            errorFound = false;
            incomingMessage.clear();
            continue;
        }

        incomingMessage.push_back(incomingByte);

        if (incomingMessage.size() == messageLength)
        {
            if (VerifyChecksum(incomingMessage))
            {
                cmdFunctionPtr(payload);
            }

            return;
        }

    }
}

// Wraps a message into a packet and send it to the serial writer
// Takes in an identifier which corresponds to the function to be called and
// a payload which is purely only the data to be sent
void SerialMessageHandler::SendMessage(const uint8_t& identifier, const std::vector<uint8_t>& payload)
{
    // Start constructing the message byte by byte
    std::vector<uint8_t> message = std::vector<uint8_t>();

    message.push_back(0xAA); // Sync byte
    message.push_back(payload.size() + HEADER_SIZE); // Length byte
    message.push_back(identifier); // Command/Identifier byte

    // Push back payload
    for (int i = 0; i < payload.size(); i++)
    {
        message.push_back(payload[i]);
    }

    // Calculate checksum to push
    uint8_t calculatedChecksum = 0;
    for (int i = 0; i < message.size(); i++)
    {
        calculatedChecksum ^= message[i];
    }
    message.push_back(calculatedChecksum); // Finally, the checksum
    
    serial.write(message.data(), message.size());
    serial.flush();
}

// Bind a function for an identifier
void SerialMessageHandler::AddCommand(const uint8_t& identifier, Command function)
{
    // TODO: Add error handling when a identifier is already bound
    cmdList[identifier] = function;
}

uint8_t SerialMessageHandler::GetID()
{
    return id;
}

/*
Attempt handshake. The response is based off what is being received in the payload

0xAA = OK
0xFF = !OK
*/
void AttemptHandshake(std::vector<uint8_t> payload)
{
    SerialMessageHandler* serialMessageHandler = SerialMessageHandler::GetInstance();
    std::vector<uint8_t> message = std::vector<uint8_t>();
    const uint8_t currID = serialMessageHandler->GetID();
    switch(payload[0])
    {
        case HANDSHAKE_REQUEST:
            // Reply with ID
            message.push_back(currID);
            break;
        default:
            if (payload[0] == currID)
            {
                message.push_back(HANDSHAKE_OK);
            }
            else
            {
                message.push_back(HANDSHAKE_ERROR);
            }
            break;
    }
    serialMessageHandler->SendMessage(HANDSHAKE_COMMAND, message);
}

/*
VERIFICATION FUNCTIONS
*/

// TODO: Change the sync byte
// Check the first byte
bool SerialMessageHandler::VerifySyncByte(const uint8_t& sync)
{
    return sync == 0xAA;
}

// This will -1 if invalid. Else, it will return the same length
int SerialMessageHandler::VerifyLengthByte(const uint8_t& len)
{
    if (len > MAX_BUFFER)
    {
        return -1;
    }

    return len;
}

// Find the function pointer. Return nullptr if not found
Command SerialMessageHandler::VerifyIdentifierByte(const uint8_t& id)
{
    if (cmdList.find(id) != cmdList.end())
    {
        return cmdList[id];
    }
    
    return nullptr;
}

bool SerialMessageHandler::VerifyChecksum(const std::vector<uint8_t>& packetData)
{
    uint8_t calculatedChecksum = 0;
    uint8_t packetChecksum = packetData[packetData.size() - 1];

    // Calculate checksum. Ignoring the last byte
    for (int i = 0; i < packetData.size() - 1; i++)
    {
        calculatedChecksum ^= packetData[i];
    }

    if (calculatedChecksum == packetChecksum)
    {
        return true;
    }

    return false;
}

/*
TODO: Usage example

#include "SerialMessageHandler.h"

SerialMessageHandler serialHandler(Serial, 9600);

void setup() {
  serialHandler.begin();
}

void loop() {
  serialHandler.update();

  if (serialHandler.available()) {
    String message = serialHandler.readMessage();
    Serial.println("Received message: " + message);
  }

  // Send a message every 5 seconds
  static unsigned long lastSendMessageTime = 0;
  if (millis() - lastSendMessageTime >= 5000) {
    lastSendMessageTime = millis();
    serialHandler.sendMessage("Hello from Arduino!");
  }
}
*/

/*
NOTE: Example command
byte startByte = 0xAA;
byte length = 6; /// Start byte + length byte + identifier + axisId + lock state + checksum
byte identifier = 0x02; /// Command identifier
byte state = lockState ? (byte)1 : (byte)0;
byte checksum = (byte)(startByte ^ length ^ identifier ^ axisId ^ state);
byte[] command = new byte[6] { startByte, length, identifier, axisId, state, checksum };
*/