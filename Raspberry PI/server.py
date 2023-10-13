from Crypto.Cipher import AES
from Crypto.Util.Padding import pad, unpad
from flask import Flask, request
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
import hashlib
import json


parser = argparse.ArgumentParser()
parser.add_argument("-hs","--hotspot", help="SSID name")
args = parser.parse_args()
with open ('*******.pem', 'rb') as f:
        privKeyServer=f.read()
privKeyServer=rsa.PrivateKey.load_pkcs1(privKeyServer)
with open ("*******.pem", "rb") as f:
        pubKeyRasp=f.read()
pubKeyRasp=rsa.PublicKey.load_pkcs1_openssl_pem(pubKeyRasp)


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
                iv=request.form['iv']
                cryptHash=request.form['hash']
                dataTemp={'res': res, 'key': key, 'iv': iv}
                ip=request.environ['REMOTE_ADDR']
                passw=rsa.decrypt(base64.b64decode(key), privKeyServer)
                iv=rsa.decrypt(base64.b64decode(iv), privKeyServer)
                cipher=AES.new(passw, AES.MODE_CBC, iv)
                fin=cipher.decrypt(base64.b64decode(bytes(res, 'utf-8')))
                finHash=cipher.decrypt(base64.b64decode(bytes(cryptHash, 'utf-8')))
                if (hashlib.sha256(json.dumps(dataTemp).encode()).digest()!=finHash):
                     raise Exception("Hash non corrispondente")
                client = pymongo.MongoClient()
                db = client.Tesi
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
                            log.write("Ip Address: "+ip+" - "+document["nome"]+" "+document["cognome"]+" - "+ time.strftime("%H:%M:%S")+ "\n")
                        newkey=getRandomStr(16)
                        cipher=AES.new(bytes(newkey, 'utf-8'), AES.MODE_ECB)
                        flag=cipher.encrypt(b'alsoeyatdfguinbg')
                        newkey=rsa.encrypt(bytes(newkey, 'utf-8'), pubKeyRasp)
                        newkey=base64.b64encode(newkey).decode('utf-8')
                        flag=base64.b64encode(flag).decode('utf-8')
                        send = {'flag' : flag, 'key' : newkey}
                        url='http://'+ip+':80/raspberry'
                        requests.post(url, json=send)
                        break
        except Exception as e:
                print(e)
                with open ("log.txt", "a") as log:
                    log.write("Ip Address: "+request.environ['REMOTE_ADDR'] +" - Unknown - "+ time.strftime("%H:%M:%S")+ "\n")
                newkey=getRandomStr(16)
                cipher=AES.new(bytes(newkey, 'utf-8'), AES.MODE_ECB)
                flag=cipher.encrypt(b'aaaaaaaaaaaaaaaa')
                newkey=rsa.encrypt(bytes(newkey, 'utf-8'), pubKeyRasp)
                newkey=base64.b64encode(newkey).decode('utf-8')
                flag=base64.b64encode(flag).decode('utf-8')
                send = {'flag' : flag, 'key' : newkey}
                url='http://'+request.environ['REMOTE_ADDR']+':80/raspberry'
                requests.post(url, json=send)
        return 'ok'

if __name__ == '__main__':
    if (args.hotspot=='smartphone'):
        app.run(host='***.***.***.***', port=8000)
    else:
        app.run(host='***.***.***.***', port=8000)
