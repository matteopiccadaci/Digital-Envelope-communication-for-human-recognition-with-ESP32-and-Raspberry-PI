from Crypto.Cipher import AES
from Crypto.Util.Padding import pad, unpad
from flask import Flask, request
import rsa
from human_heatmap_classifier import *
from PIL import Image
import numpy as np
import matplotlib.pyplot as plt
import time
import base64
import random
import string
import time
import requests
import io
import os
import cv2
from time import sleep
import board,busio
import adafruit_mlx90640
import RPi.GPIO as GPIO
import subprocess
import hashlib
import json


green_led=17
red_led=27
GPIO.setwarnings(False)
GPIO.cleanup()
GPIO.setmode(GPIO.BCM)
GPIO.setup(green_led, GPIO.OUT)
GPIO.setup(red_led, GPIO.OUT)
camera = cv2.VideoCapture(0)

i2c = busio.I2C(board.SCL, board.SDA, frequency=400000)
mlx = adafruit_mlx90640.MLX90640(i2c)
mlx.refresh_rate = adafruit_mlx90640.RefreshRate.REFRESH_2_HZ 
mlx_shape = (24,32)
plt.ion() # enables interactive plotting
fig,ax = plt.subplots(figsize=(12,7))
therm1 = ax.imshow(np.zeros(mlx_shape),vmin=0,vmax=60)
cbar = fig.colorbar(therm1) # setup colorbar for temps
cbar.set_label('Temperature [$^{\circ}$C]',fontsize=14)

url='http://***.***.***.***:****/'

def take_photo():
    result, image = camera.read()
    sleep(0.5)
    if result:
        cv2.imwrite("temp.jpg", image)
    return 1

def evaluate_thermal_image():
    frame = np.zeros((24*32,))
    while True:
        try:
            mlx.getFrame(frame) # read MLX temperatures into frame var
            data_array = (np.reshape(frame,mlx_shape))
            therm1.set_data(np.fliplr(data_array)) # flip left to right
            therm1.set_clim(vmin=np.min(data_array),vmax=np.max(data_array)) # set bounds
            cbar.on_mappable_changed(therm1) # update colorbar range
            plt.pause(0.001) # required
            fig.savefig('temp.png',dpi=300,facecolor='#FCFCFC', bbox_inches='tight') 
            image = Image.open("tempT.png")
            new_image = image.resize((670, 488)) 
            new_image.save("tempT.png")
            if (evaluate("tempT.png")!=True):
                os.remove("tempT.png")
                return False   
            else:
                os.remove("tempT.png")
                return True
        except ValueError:
            continue 

def get_random_string(length):
    letters = string.ascii_lowercase+string.ascii_uppercase+string.digits
    result_str = ''.join(random.choice(letters) for i in range(length))
    return result_str

with open ('******.pem', 'rb') as f:
        pubKeyServer=f.read()
pubKeyServer=rsa.PublicKey.load_pkcs1_openssl_pem(pubKeyServer)

print("Ready")
while True:
    try:
        random_key=get_random_string(32)
        random_key_cripted=base64.b64encode(rsa.encrypt(bytes(random_key, 'utf-8'), pubKeyServer))
        if (evaluate_thermal_image()==True):
            take_photo()
            with open ('temp.jpg', 'rb') as f:
                    img=f.read()
            cipher=AES.new(bytes(random_key, 'utf-8'), AES.MODE_CBC)
            img=base64.b64encode(cipher.encrypt(pad(img, AES.block_size)))
            iv = base64.b64encode(rsa.encrypt(cipher.iv, pubKeyServer))
            data={'res':img.decode('utf-8'),'key':random_key_cripted.decode('utf-8'), 'iv':iv.decode('utf-8')}
            dataHash=hashlib.sha256(json.dumps(data).encode()).digest()
            cryptHash=base64.b64encode(cipher.encrypt(dataHash))
            requests.post(url, data={'res':img,'key':random_key_cripted, 'iv':iv, 'hash':cryptHash})
        else:
            GPIO.output(red_led, GPIO.HIGH)
            time.sleep(0.5)
            GPIO.output(red_led, GPIO.LOW)
            time.sleep(0.5)
            GPIO.output(red_led, GPIO.HIGH)
            time.sleep(0.5)
            GPIO.output(red_led, GPIO.LOW)
            raise Exception("Non Human")
        time.sleep(2)

    except Exception as e:
        print(e)
        continue
