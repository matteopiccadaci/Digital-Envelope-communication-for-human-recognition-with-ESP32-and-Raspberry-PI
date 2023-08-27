from Crypto.Cipher import AES
from Crypto.Util.Padding import pad, unpad
from flask import Flask, request
import rsa
import time
import base64
import pymongo
import face_recognition
import random
import string
import time
import requests
import io
import socket
import os

with open ('privKeyServer.pem', 'rb') as f:
        privKeyServer=f.read()
privKeyServer=rsa.PrivateKey.load_pkcs1(privKeyServer)
with open ("pubKeyESP.pem", "rb") as f:
        pubKeyESP=f.read()
pubKeyESP=rsa.PublicKey.load_pkcs1_openssl_pem(pubKeyESP)


def getRandomStr(length):
    letters = string.ascii_lowercase
    result_str = ''.join(random.choice(letters) for i in range(length))
    return result_str

app = Flask(__name__)
@app.route('/',methods = ['POST'])
def index():
    while True:
        try:
            if request.method == 'POST':
                res=request.form['res']
                key=request.form['key']
                ip=request.environ['REMOTE_ADDR']
                key=key.replace(" ", "+") 
                res=res.replace(" ", "+")
                
               """ As I mentioned in the other code, you have to replace the blank
               spaces with the '+' signs """
                
                key=base64.b64decode(key)
                passw=rsa.decrypt(key, privKeyServer)
                res=base64.b64decode(bytes(res, 'utf-8'))
                cipher=AES.new(passw, AES.MODE_ECB)
                fin=cipher.decrypt(res)
                
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
                            log.write("IP Address: "+ip+" - "+document["name"]+" "+document["surname"]+" - "+ time.strftime("%H:%M:%S")+ "\n")
                        newkey=getRandomStr(16)
                        cipher=AES.new(bytes(newkey, 'utf-8'), AES.MODE_ECB)
                        flag=cipher.encrypt(b'alsoeyatdfguinbg')
                        newkey=rsa.encrypt(bytes(newkey, 'utf-8'), pubKeyESP)
                        newkey=base64.b64encode(newkey).decode('utf-8')
                        flag=base64.b64encode(flag).decode('utf-8')
                        send = {'flag' : flag, 'key' : newkey}
                        url='http://'+ip+':****/esp'
                        requests.post(url, json=send)
                        break

                    """ For every person in the db, I compare the weights of the image of the ESP and the one in the 
                    Database """


        except Exception as e:
                with open ("log.txt", "a") as log:
                    log.write("IP Address: "+request.environ['REMOTE_ADDR'] +" - Unknown - "+ time.strftime("%H:%M:%S")+ "\n")
                newkey=getRandomStr(16)
                cipher=AES.new(bytes(newkey, 'utf-8'), AES.MODE_ECB)
                flag=cipher.encrypt(b'aaaaaaaaaaaaaaaa')
                newkey=rsa.encrypt(bytes(newkey, 'utf-8'), pubKeyESP)
                newkey=base64.b64encode(newkey).decode('utf-8')
                flag=base64.b64encode(flag).decode('utf-8')
                send = {'flag' : flag, 'key' : newkey}
                url='http://'+request.environ['REMOTE_ADDR']+':****/esp'
                requests.post(url, json=send)
                
                
        return 'ok'

if __name__ == '__main__':  
        app.run(host='***.***.***.***', port=****)
