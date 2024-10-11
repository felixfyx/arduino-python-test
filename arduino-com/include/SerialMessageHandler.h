#include "globals.h"

#ifndef SERIAL_MESSAGE_HANDLER_H
#define SERIAL_MESSAGE_HANDLER_H

#define MAX_BUFFER 64
#define HEADER_SIZE 4 // Size of header: Sync byte + length byte + ID byte + Checksum byte

#define HANDSHAKE_REQUEST 0x00
#define HANDSHAKE_COMMAND 0xFF
#define HANDSHAKE_OK 0xAA
#define HANDSHAKE_ERROR 0XFF

/*
Currently this is running on a single thread too so we want to make it multithreaded to 
support future implementation of websocket support
*/

// A generic function that takes in a series of bytes to process
// This means it's up to the developer to create their own function to take
// care of byte processing
typedef void(*Command)(std::vector<uint8_t>);

class SerialMessageHandler {
private:
    SerialMessageHandler(HardwareSerial& serial, const uint32_t& baudRate, const uint8_t& id);

    static SerialMessageHandler* ptr_instance;

    HardwareSerial& serial;
    std::map<uint8_t, Command> cmdList;
    uint8_t id;

    /*
    VERIFICATION FUNCTIONS
    */
    bool VerifySyncByte(const uint8_t& sync);
    int VerifyLengthByte(const uint8_t& len);
    Command VerifyIdentifierByte(const uint8_t &id);
    bool VerifyChecksum(const std::vector<uint8_t>& packetData);
public:
    static SerialMessageHandler* GetInstance();

    // Delete other constructor
    SerialMessageHandler(const SerialMessageHandler&) = delete;
    SerialMessageHandler(SerialMessageHandler&&) = delete;
    SerialMessageHandler& operator=(const SerialMessageHandler&) = delete;
    SerialMessageHandler& operator=(SerialMessageHandler&&) = delete;

    void Update();
    void SendMessage(const uint8_t& identifier, const std::vector<uint8_t>& payload);
    void AddCommand(const uint8_t& identifier, Command function);
    uint8_t GetID();
};

void AttemptHandshake(std::vector<uint8_t> payload);

#endif
