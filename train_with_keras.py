from keras import layers, models
import numpy as np

def read_cifar(file):
    images = np.zeros((16,32,32,3), dtype=int)
    labels = np.zeros(16, dtype=int)
    with open(file, 'rb') as f:
        for i in range(16):
            labels[i] = int.from_bytes(f.read1(1), byteorder='big')
            for j in range(3):
                for m in range(32):
                    for n in range(32):
                        images[i,m,n,j] = int.from_bytes(f.read1(1), byteorder='big')
    return images, labels

model = models.Sequential()
model.add(layers.Conv2D(32, (3, 3), activation='relu', input_shape=(32, 32, 3)))
model.add(layers.MaxPooling2D((2, 2)))
model.add(layers.Conv2D(64, (3, 3), activation='relu'))
model.add(layers.MaxPooling2D((2, 2)))
model.add(layers.Conv2D(64, (3, 3), activation='relu'))
model.add(layers.Flatten())
model.add(layers.Dense(64, activation='relu'))
model.add(layers.Dense(10))

# TODO: get weight from pipe and store in variable `weights`
weights = None
model.set_weights(weights)

# TODO: get images from pipe and store in variables `images` and `labels`
images = None
labels = None

model.compile(optimizer='adam',
              loss='categorical_crossentropy',
              metrics=['accuracy'])

model.fit(images, labels, epochs=1)

weights = model.get_weights()

# TODO: send weights through pipe to the C program

# first_layer_weights = model.layers[0].get_weights()[0]
# first_layer_biases  = model.layers[0].get_weights()[1]
# second_layer_weights = model.layers[1].get_weights()[0]
# second_layer_biases  = model.layers[1].get_weights()[1]