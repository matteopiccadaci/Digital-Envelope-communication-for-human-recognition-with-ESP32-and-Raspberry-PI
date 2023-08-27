# Digital Envelope communication for human recognition with ESP32 and Raspberry PI
In this repo you will find the code for transfering images (but this method is also good for any kind of data) for those two devices. The architecure is client-server, where the client is, for istance, the ESP32 and the server is your computer, with Python3 installed. In your computer you should have the photos of the people you want to recognize and some way to bind the photo to their name. You could just rename the photo as their name and surname (i.e. LukeWhites.jpg), but my suggestion is to store the images on a Mongo database, as I did in the code. I have a Database named, with much creativity, "Database" where there is a collection named "People". In this every entry is formed by: Name, surname and photo coded in base64 of the subject. On your computer you just need to create the Database, download the Python modules, change the IP address in the script and you are ready. As for the code for the devices, inside each folder you'll find futher description.
