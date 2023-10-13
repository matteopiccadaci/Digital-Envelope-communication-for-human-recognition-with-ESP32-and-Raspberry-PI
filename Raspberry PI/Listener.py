from Crypto.Cipher import AES
from Crypto.Util.Padding import pad, unpad
from flask import Flask, request
import rsa
import time
import base64
from time import sleep
import RPi.GPIO as GPIO

green_led=17
red_led=27
GPIO.setwarnings(False)
GPIO.cleanup()
GPIO.setmode(GPIO.BCM)
GPIO.setup(green_led, GPIO.OUT)
GPIO.setup(red_led, GPIO.OUT)

with open ("privKeyEsp.pem", "rb") as f:
        privKeyRasp=f.read()
privKeyRasp=rsa.PrivateKey.load_pkcs1(privKeyRasp)


app = Flask(__name__)
@app.route('/raspberry',methods = ['POST'])
def index():
    if request.method == 'POST':
        flag=request.json['flag']
        key=request.json['key']
        flag=base64.b64decode(flag)
        key=base64.b64decode(key)
        key=rsa.decrypt(key, privKeyRasp)
        cipher=AES.new(key, AES.MODE_ECB)
        flag=cipher.decrypt(flag).decode('utf-8')
        if flag=='alsoeyatdfguinbg':
            print ("Recognized")
            GPIO.output(green_led, GPIO.HIGH)
            time.sleep(2)
            GPIO.output(green_led, GPIO.LOW)
        else:
            print ("Not recognized")
            GPIO.output(red_led, GPIO.HIGH)
            time.sleep(0.5)
            GPIO.output(red_led, GPIO.LOW)
            time.sleep(0.5)
            GPIO.output(red_led, GPIO.HIGH)
            time.sleep(0.5)
            GPIO.output(red_led, GPIO.LOW)
        return "OK"

if __name__ == '__main__':
    app.run(host='***.***.***.***', port=80)
