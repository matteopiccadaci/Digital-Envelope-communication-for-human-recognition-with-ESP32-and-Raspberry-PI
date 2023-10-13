import tensorflow as tf
from tensorflow.keras.preprocessing import image
import numpy as np

model = tf.keras.models.load_model('human_heatmap_raspberry.h5')

def evaluate(img):
    img = image.load_img(img, target_size=(670, 488))
    img_array = image.img_to_array(img)
    img_array = np.expand_dims(img_array, axis=0)
    img_array = img_array / 255.0

    predictions = model.predict(img_array)
    class_index = np.argmax(predictions[0])
    class_labels = ['non_human', 'human']
    class_label = class_labels[class_index]

    if (class_label == 'human'):
        print("Human detected")
        return True
    else:
        print("Non human detected")
        return False
