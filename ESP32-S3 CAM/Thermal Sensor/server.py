from Crypto.Cipher import AES
from Crypto.Util.Padding import pad, unpad
from flask import Flask, request
from human_heatmap_classifier import *
import rsa
from PIL import Image
import numpy as np
import matplotlib.pyplot as plt
import time
import base64
import pymongo
import face_recognition
import random
import string
import time
import requests
import argparse
import io
import os


parser = argparse.ArgumentParser()
parser.add_argument("-hs","--hotspot", help="The word you want to count the occurrences of")
args = parser.parse_args()
with open ('privKeyServer.pem', 'rb') as f:
        privKeyServer=f.read()
privKeyServer=rsa.PrivateKey.load_pkcs1(privKeyServer)
with open ("pubKeyESP.pem", "rb") as f:
        pubKeyESP=f.read()
  
pubKeyESP=rsa.PublicKey.load_pkcs1_openssl_pem(pubKeyESP)
mlx_shape = (24,32)
plt.ion() # enables interactive plotting
fig,ax = plt.subplots(figsize=(12,7))
therm1 = ax.imshow(np.zeros(mlx_shape),vmin=0,vmax=60) #start plot with zeros
cbar = fig.colorbar(therm1) # setup colorbar for temps
cbar.set_label('Temperature [$^{\circ}$C]',fontsize=14) # colorbar label
frame = np.zeros((24*32,)) # setup array for storing all 768 temperatures
t_array = []

def getRandomStr(length):
    letters = string.ascii_lowercase
    result_str = ''.join(random.choice(letters) for i in range(length))
    return result_str

app = Flask(__name__)
app.config['MAX_CONTENT_LENGTH'] = 100 * 1024 * 1024
@app.route('/',methods = ['POST'])
def index():
    while True:
        try:
            if request.method == 'POST':
                res=request.form['res']
                key=request.form['key']
                mlx=request.form['mlx']
                ip=request.environ['REMOTE_ADDR']
                key=key.replace(" ", "+") 
                res=res.replace(" ", "+")
                mlx=mlx.replace(" ", "+")
                key=base64.b64decode(key)
                passw=rsa.decrypt(key, privKeyServer)
                res=base64.b64decode(bytes(res, 'utf-8'))
                mlx=base64.b64decode(bytes(mlx, 'utf-8'))
                cipher=AES.new(passw, AES.MODE_ECB)
                fin=cipher.decrypt(res)


                mlx=mlx.decode('utf-8')
                mlx=mlx.split(',')
                mlx=mlx[0:-1]
                mlx=[float(i) for i in mlx]

                """ The output of the MLX sensor is an array of temperature that can to be represented as an image. 
                    As I mentioned in the ReadMe, the mlx array can't be send in a secure fashion.
                """
                frame = np.array(mlx)
                data_array = (np.reshape(frame,mlx_shape)) # reshape to 24x32
                therm1.set_data(np.fliplr(data_array)) # flip left to right
                therm1.set_clim(vmin=np.min(data_array),vmax=np.max(data_array)) # set bounds
                time.sleep(0.01)
                print("Image received")
                fig.savefig('temp.png',dpi=300,facecolor='#FCFCFC',bbox_inches='tight')
                image = Image.open("temp.png")
                new_image = image.resize((1339, 975))
                """
                My computer couldn't handle tha training with the full sized images, so i had to resize the images
                """
                new_image.save("temp.png")
                if (evaluate("temp.png")!=True):
                     print("Non human detected")
                     raise Exception("Non human detected") 
                os.remove("temp.png")


                client = pymongo.MongoClient()
                db = client.Database
                collection = db.People
                non_reference_image = face_recognition.load_image_file(io.BytesIO(fin))
                non_reference_encoding = face_recognition.face_encodings(non_reference_image)[0]
                cursor = collection.find({})
                for document in cursor:
                    binary_data = base64.b64decode(document["image"])
                    reference_image = face_recognition.load_image_file(io.BytesIO(binary_data))
                    reference_encoding = face_recognition.face_encodings(reference_image)[0]
                    distance = np.linalg.norm(reference_encoding - non_reference_encoding)
                    if distance < 0.6:
                        with open ("log.txt", "a") as log:
                            log.write("Ip Address: "+ip+" - "+document["name"]+" "+document["surname"]+" - "+ time.strftime("%H:%M:%S")+ "\n")
                        newkey=getRandomStr(16)
                        cipher=AES.new(bytes(newkey, 'utf-8'), AES.MODE_ECB)
                        flag=cipher.encrypt(b'alsoeyatdfguinbg')
                        newkey=rsa.encrypt(bytes(newkey, 'utf-8'), pubKeyESP)
                        newkey=base64.b64encode(newkey).decode('utf-8')
                        flag=base64.b64encode(flag).decode('utf-8')
                        send = {'flag' : flag, 'key' : newkey}
                        url='http://'+ip+':80/esp'
                        requests.post(url, json=send)
                        return "OK"
                   
        except Exception as e:
                print(e)
                with open ("log.txt", "a") as log:
                    log.write("Ip Address: "+request.environ['REMOTE_ADDR'] +" - Unknown - "+ time.strftime("%H:%M:%S")+ "\n")
                newkey=getRandomStr(16)
                cipher=AES.new(bytes(newkey, 'utf-8'), AES.MODE_ECB)
                flag=cipher.encrypt(b'aaaaaaaaaaaaaaaa')
                newkey=rsa.encrypt(bytes(newkey, 'utf-8'), pubKeyESP)
                newkey=base64.b64encode(newkey).decode('utf-8')
                flag=base64.b64encode(flag).decode('utf-8')
                send = {'flag' : flag, 'key' : newkey}
                url='http://'+request.environ['REMOTE_ADDR']+':80/esp'
                requests.post(url, json=send)
               
                return 'OK'

if __name__ == '__main__':
    if (args.hotspot=='smartphone'):
        app.run(host='***.***.***.***', port=****)
    else:
        app.run(host='***.***.***.***', port=****)
