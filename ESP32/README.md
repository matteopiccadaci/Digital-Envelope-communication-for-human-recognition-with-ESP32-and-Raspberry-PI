## ESP32 CAM
This is the version for ESP32-CAM. <br>
The first thing you have to do is create two pair of private keys, one pair for the ESP and another for the server. <br>
Then you have to put your SSID, password adn the IP address of the server in the .ino code and write the private key of the ESP and the public key of the server in the appropiate variables (my suggestion is to use the <em>cat</em> command on the terminal. Every time a new line starts, you have to write <em>\n</em>).<br>
After this, you're ready to upload the code on the ESP. This implementation is based on time, the ESP takes as many photos as it can. <br>
I noticed that when too many photos are taken, the ESP can't handle to take anymore so my suggestion is to use the button approach.<br>
When the code is uploaded, you just have to start the Python script on the computer. If the person is recognized, he server sends a particular string whereas, if it's not recognized, sends onther.<br>
The ESP then turns on and off the LEDs based on the string it has after decryption.
