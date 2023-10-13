import requests
import base64
import numpy as np
import matplotlib.pyplot as plt
import time
import board,busio
import adafruit_mlx90640

url="http://***.***.***.***:****/"
i2c = busio.I2C(board.SCL, board.SDA, frequency=400000)
mlx = adafruit_mlx90640.MLX90640(i2c)
mlx.refresh_rate = adafruit_mlx90640.RefreshRate.REFRESH_2_HZ 
mlx_shape = (24,32)
plt.ion() # enables interactive plotting
fig,ax = plt.subplots(figsize=(12,7))
therm1 = ax.imshow(np.zeros(mlx_shape),vmin=0,vmax=60)
cbar = fig.colorbar(therm1) # setup colorbar for temps
cbar.set_label('Temperature [$^{\circ}$C]',fontsize=14)


def take_photo():
    frame = np.zeros((24*32,))
    mlx.getFrame(frame) # read MLX temperatures into frame var
    data_array = (np.reshape(frame,mlx_shape))
    therm1.set_data(np.fliplr(data_array)) # flip left to right
    therm1.set_clim(vmin=np.min(data_array),vmax=np.max(data_array)) # set bounds
    cbar.on_mappable_changed(therm1) # update colorbar range
    plt.pause(0.001) # required
    fig.savefig('temp.png',dpi=300,facecolor='#FCFCFC', bbox_inches='tight') 

while True:
    try:
        take_photo()
        with open ('temp.png', 'rb') as f:
                    img=f.read()
        img=base64.b64encode(img)
        requests.post(url, data={'photo':img})
        time.sleep(2)
  
    except Exception as e:
        print(e)
        continue
  
