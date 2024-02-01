import os
import sys

def python_learning_process(pipe_read_end):
    # This function receives data from C program, performs computation, and sends back results

    results = []

    while True:
        try:
            # Read data from the pipe
            data = os.read(pipe_read_end, 4)
            if not data:
                break
            # Perform computation (e.g., learning weights)
            result = int.from_bytes(data, byteorder='little') * 2
            # Append the result to the list
            results.append(result)
        except Exception as e:
            print(f"Error: {e}")

    # Send the results back to the C program
    sys.stdout.buffer.write(bytearray(results))

if __name__ == "__main__":
    # Read the pipe file descriptor from the command line
    pipe_read_end = sys.stdin.fileno()
    # Call the function for processing data
    python_learning_process(pipe_read_end)
