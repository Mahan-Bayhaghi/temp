import os

fifo_path = "/tmp/myfifo"

# Open the named pipe for reading
with open(fifo_path, "rb") as fifo:
    while True:
        # Read the image data (assuming it is a 32x32 RGB image)
        image_data = fifo.read(32 * 32 * 3)
        
        # Break the loop if there is no more data
        if not image_data:
            break
        
        # Read the label
        label_data = fifo.read(1)
        
        # Process or print the received CIFAR-10 data
        print("Received CIFAR-10 Image:")
        print("Image Data:", image_data)  # Adjust as needed
        print("Label:", label_data)  # Adjust as needed
        print()

# Note: This is a basic example. You should customize it based on the actual format and processing you need.
