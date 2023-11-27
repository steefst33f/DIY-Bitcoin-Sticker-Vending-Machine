import sys

if __name__ == "__main__":
    i = 0
    checksum = bytes(b"\x00")
    for byte in bytes.fromhex(sys.argv[1]):
        print(checksum[0], byte)
        checksum = bytes([(checksum[0] + byte)%256])
        # print(hex(byte))
        print(checksum.hex())
        i = i + 1

    print(sys.argv[1]+checksum.hex())
    print(i)