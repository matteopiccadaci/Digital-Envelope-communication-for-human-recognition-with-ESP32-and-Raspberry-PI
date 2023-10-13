## Raspberry PI
This is the implementation for Raspberry PI 4. It enhances the ESP32 version by adding hashing and Social Engeneering protection using the MLX90640 thermal sensor.
This sensor is to take thermal photos, so a Machine Learning model was trained to decide whether the bare face was captured or if there is an object beyond it.
You can read this [guide](https://makersportal.com/blog/2020/6/8/high-resolution-thermal-camera-with-raspberry-pi-and-mlx90640) to see how to connect the thermal sensor to the Raspberry.
In order to make the system work, you have to train the model (further informations are into the "Neural Network" folder), the import the files named
"human_heatmap_classifier.py", "Camera.py" and "Listener.py" onto the Raspberry, then start the "Camera.py" and "Listener.py" scripts. After that, you can start the "server.py" script into your other device.
