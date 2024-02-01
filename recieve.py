import os
import sys
import struct

def main():
    read_fd = int(sys.argv[1])
    write_fd = int(sys.argv[2])

    with os.fdopen(read_fd, 'rb') as f:
        # Read the sums from the pipe
        sums = []
        while True:
            sum_bytes = f.read(4)  # Assuming the sums are 4-byte integers
            if not sum_bytes:
                break
            sums.append(struct.unpack('i', sum_bytes)[0])

    # Compute the average of the sums
    average = sum(sums) / len(sums)

    with os.fdopen(write_fd, 'wb') as f:
        # Write the average to the pipe
        f.write(struct.pack('f', average))

if __name__ == "__main__":
    main()
