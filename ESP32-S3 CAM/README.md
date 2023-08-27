This is the version for ESP32S3-CAM.
The code is basically the same of the ESP32 version, the only differences are on the camera settings and the LED, so for comments on the code please refer to the ones in the ESP32 folder.
The first thing you have to do is create two pair of private keys, one pair for the ESP and another for the server.
Then you have to put your SSID, password adn the IP address of the server in the .ino code and write the private key of the ESP and the public key of the server in the appropiate variables (my suggestion is to use the <em>cat</em> command on the terminal. Every time a new line starts, you have to write <em>\n</em>).
After this, you're ready to upload the code on the ESP. This implementation is based on time, the ESP takes as many photos as it can. 
I noticed that when too many photos are taken, the ESP can't handle to take anymore so my suggestion is to use the button approach.
When the code is uploaded, you just have to start the Python script on the computer. If the person is recognized, he server sends a particular string whereas, if it's not recognized, sends onther.
The ESP then turns on and off the LEDs based on the string it has after decryption.
