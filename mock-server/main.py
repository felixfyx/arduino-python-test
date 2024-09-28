import serial
import serial.tools.list_ports
import time

def handshake(ser, id):
    """Perform handshake with Arduino to get its ID and connect to a specific device"""
    ser.write(b'H')  # Send handshake request
    response = ser.readline().decode().strip()
    print(f"Received response: {response}")
    if response.startswith('A'):
        arduino_id = int(response[1:])
        print(f"arduino_id: {arduino_id}")
        if arduino_id == id:  # Check if the ID matches the desired device
            print("Sending C byte")
            ser.write(b'C')  # Send connection request
            ser.write(str(id).encode())  # Send the ID
            response = ser.readline().decode().strip()
            print(f"Received back response: {response}")
            if response == 'C':
                print(f"Connected to Arduino {id} on port {ser.name}")
                return True
            else:
                print("Connection failed")
                return False
        else:
            print("Arduino ID mismatch")
            return False
    else:
        print("Handshake failed")
        return False

def find_arduino(id):
    """Find the Arduino with the specified ID among all available COM ports"""
    ports = serial.tools.list_ports.comports()
    for port in ports:
        try:
            print(f"Attempting port: {port.device}")
            ser = serial.Serial(port.device, 9600, timeout=5)
            time.sleep(2)  # Wait for Arduino to initialize
            if handshake(ser, id):
                return ser
            else:
                ser.close()
        except serial.SerialException:
            print(f"Error opening port {port.device}")
    return None

def main():
    target_id = 1  # Replace with the ID of the Arduino you want to connect to

    ser = find_arduino(target_id)
    if ser:
        # Now you can send and receive data with the Arduino
        while True:
            user_input = input("Enter a message to send to Arduino: ")
            ser.write(user_input.encode())
            response = ser.readline().decode().strip()
            print(f"Arduino responded: {response}")
    else:
        print("Arduino not found")

if __name__ == "__main__":
    main()