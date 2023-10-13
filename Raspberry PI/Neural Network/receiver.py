from flask import Flask, request
from PIL import Image
import base64
import numpy as np
import matplotlib.pyplot as plt
import time


app = Flask(__name__)
app.config['MAX_CONTENT_LENGTH'] = 100 * 1024 * 1024
@app.route('/',methods = ['POST'])
def index():
    while True:
        try:
            if request.method == 'POST':
                photo=request.form['photo']
                mlx=base64.b64decode(bytes(photo, 'utf-8'))
                with open('temp.png', 'wb') as f:
                    f.write(mlx)
                image = Image.open("temp.png")
                new_image = image.resize((670, 488))
                new_image.save("temp.png")
        except Exception as e:
                print(e)
                return 'OK'

if __name__ == '__main__':
    app.run(host='172.20.10.7', port=8000)
