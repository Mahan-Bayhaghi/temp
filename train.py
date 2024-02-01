import sys
import pickle

# load and train CIFAR-10 dataset from pipe
# ...

# serialize the trained model
trained_data = pickle.dumps(model)

# send serialized data to C program to be rerouted
sys.stdout.buffer.write(trained_data)
sys.stdout.flush()
