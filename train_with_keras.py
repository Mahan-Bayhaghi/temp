from keras import layers, models
import numpy as np
import sys

images_recieve_pipe, weights_recieve_pipe, weights_send_pipe = sys.argv[1], sys.argv[2], sys.argv[3]

def read_cifar(file, n_images):
    images = np.zeros((n_images,32,32,3), dtype=int)
    labels = np.zeros(n_images, dtype=int)
    with open(file, 'rb') as pipe:
        for i in range(n_images):
            labels[i] = int.from_bytes(pipe.read1(1), byteorder='big')
            for j in range(3):
                for m in range(32):
                    for n in range(32):
                        images[i, m, n, j] = int.from_bytes(pipe.read1(1), byteorder='big')
    return images, labels

def read_weights(file):
    weights = None

    # TODO: write a function to convert byte string to numpy arrays
    with open(file, "rb") as pipe:
        pass # use pipe.read()

    return weights

def write_weights(file, weights):

    # TODO: write a function to convert numpy arrays in `weights` into byte data and write it in the file
    with open(file, "wb") as pipe:
        pass # use pipe.write()



model = models.Sequential()
model.add(layers.Conv2D(32, (3, 3), activation='relu', input_shape=(32, 32, 3)))
model.add(layers.MaxPooling2D((2, 2)))
model.add(layers.Conv2D(64, (3, 3), activation='relu'))
model.add(layers.MaxPooling2D((2, 2)))
model.add(layers.Conv2D(64, (3, 3), activation='relu'))
model.add(layers.Flatten())
model.add(layers.Dense(64, activation='relu'))
model.add(layers.Dense(10))

while (True):
    weights = read_weights(weights_recieve_pipe) 

    model.set_weights(weights)

    images, labels = read_cifar(images_recieve_pipe)

    model.compile(optimizer='adam', loss='categorical_crossentropy', metrics=['accuracy'])

    model.fit(images, labels, epochs=1)

    weights = model.get_weights()

    write_weights(weights_send_pipe, weights)
