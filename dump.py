import urllib.request
import tarfile
import numpy as np
import sys
import pickle

# Download the CIFAR-10 dataset
url = 'https://www.cs.toronto.edu/~kriz/cifar-10-python.tar.gz'
filename = url.split('/')[-1]
urllib.request.urlretrieve(url, filename)

# Extract the dataset
with tarfile.open(filename, 'r:gz') as tar:
    tar.extractall()

# Load the dataset
x_train = []
y_train = []
for i in range(1, 6):
    filename = f'cifar-10-batches-py/data_batch_{i}'
    with open(filename, 'rb') as f:
        data = pickle.load(f, encoding='bytes')
    x_train.append(data[b'data'])
    y_train.append(data[b'labels'])
x_train = np.concatenate(x_train)
y_train = np.concatenate(y_train)
x_test = []
y_test = []
filename = 'cifar-10-batches-py/test_batch'
with open(filename, 'rb') as f:
    data = pickle.load(f, encoding='bytes')
x_test.append(data[b'data'])
y_test.append(data[b'labels'])
x_test = np.concatenate(x_test)
y_test = np.concatenate(y_test)

# Dump the dataset in the standard system pipe
np.save(sys.stdout.buffer, ((x_train, y_train), (x_test, y_test)))
