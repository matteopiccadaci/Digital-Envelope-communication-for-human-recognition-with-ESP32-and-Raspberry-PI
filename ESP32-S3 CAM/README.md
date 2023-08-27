This is the version for ESP32S3-CAM. <br>
The code is basically the same of the ESP32 version, the only differences are on the camera settings and the LED, so for comments on the code please refer to the ones in the ESP32 folder. <br>
The first thing you have to do is create two pair of private keys, one pair for the ESP and another for the server. <br>
Then you have to put your SSID, password adn the IP address of the server in the .ino code and write the private key of the ESP and the public key of the server in the appropiate variables (my suggestion is to use the <em>cat</em> command on the terminal. Every time a new line starts, you have to write <em>\n</em>).<br>
After this, you're ready to upload the code on the ESP. This implementation is based on time, the ESP takes as many photos as it can. <br>
I noticed that when too many photos are taken, the ESP can't handle to take anymore so my suggestion is to use the button approach.<br>
When the code is uploaded, you just have to start the Python script on the computer. If the person is recognized, he server sends a particular string whereas, if it's not recognized, sends onther.<br>
The ESP then turns on and off the LEDs based on the string it has after decryption.

# MLX90640 Thermal sensor implementation
There is a version of this system in which the server performs a machine learning inference to understand if the subject of the photo is a human or if somone tried to trick the system by taking "a photo of a photo" of an authorized person. In the thermal sensor folder you will find the server an client script (the machine learning model and the code to train it are in the Raspberry folder of this repository).<br>
So, now the ESP sends the "normal" photo and the thermal one and the server makes the inference before trying to recognize the person.<br>
<strong>BEWARE</strong> that the thermal photo cannot be send in a secure fashion, beacuse the ESP can't handle another crypt due to its limited memory. Another trade off is that the probabilty that the ESP can't initialize the camera is very much higher, to be honest more than 50% of the times the photo isn't taken correctly.
