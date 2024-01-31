import numpy as np
import sys
import pickle

# Load the CIFAR-10 dataset
(x_train, y_train), (x_test, y_test) = np.load(sys.stdin.buffer, allow_pickle=True)

# Normalize the data
x_train = x_train.astype('float32') / 255
x_test = x_test.astype('float32') / 255

# Define the model architecture
model = np.zeros((32, 32, 3), dtype=object)

# Train the model
for epoch in range(10):
    for i in range(len(x_train)):
        x = x_train[i]
        y = y_train[i]
        # Forward pass
        z = np.dot(x, model)
        # Compute loss
        loss = np.sum((z - y) ** 2)
        # Backward pass
        grad = 2 * (z - y)
        model -= 0.01 * np.outer(x, grad)

# Evaluate the model
correct = 0
for i in range(len(x_test)):
    x = x_test[i]
    y = y_test[i]
    z = np.dot(x, model)
    if np.argmax(z) == y:
        correct += 1
accuracy = correct / len(x_test)
print(f'Test accuracy: {accuracy:.2%}')

# Receive input of dataset from standard system pipe
input_data = sys.stdin.read()
