import sys
import struct

if __name__ == "__main__":

    packetHeader = b"IMPROV"
    packetVersion = b"\x01"
    packetType = b"\x03"
    commandId = b"\x01"
    ssid = sys.argv[1].encode("ascii")
    ssidLength = struct.pack(">B", len(ssid))
    password = sys.argv[2].encode("ascii")https://bit.ly/pio-monitor-filters
    passwordLength = struct.pack(">B", len(password))
    commandData = ssidLength + ssid + passwordLength + password
    commandDataLength = struct.pack(">B", len(commandData))
    packetData = commandId + commandDataLength + commandData
    packetDataLength = struct.pack(">B", len(packetData))
    packet = packetHeader + packetVersion + packetType + packetDataLength + packetData 
    packetChecksum = struct.pack(">B", sum(packet) % 256)
    packet += packetChecksum
  
    # print("header")
    # print(header)
    # print(version)
    # print(cmdType)
    # print(length)
    # print(ssidLength)
    # print(ssid)
    # print(passwordLenght)
    # print(password)

    # checksum2 = bytes(b"\x00")
    # for byte in command:
    #     print(checksum2[0], byte)
    #     checksum2 = bytes([(checksum2[0] + byte)%256])
    #     # print(hex(byte))
    #     print(checksum2.hex())
    #     i = i + 1

    print((packet).hex())