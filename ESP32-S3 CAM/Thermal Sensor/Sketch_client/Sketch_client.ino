#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <stdlib.h>
#include "SPIFFS.h"
#include "mbedtls/aes.h"
#include "mbedtls/pk.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/base64.h"
#include "esp_camera.h"
#include "base64.h"
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"


#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 15
#define SIOD_GPIO_NUM 4
#define SIOC_GPIO_NUM 5

#define Y2_GPIO_NUM 11
#define Y3_GPIO_NUM 9
#define Y4_GPIO_NUM 8
#define Y5_GPIO_NUM 10
#define Y6_GPIO_NUM 12
#define Y7_GPIO_NUM 18
#define Y8_GPIO_NUM 17
#define Y9_GPIO_NUM 16

#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM 7
#define PCLK_GPIO_NUM 13

#define NUM_LEDS 1 
#define LED_PIN 48 
#define LED_TYPE NEO_GRB 

const char *SSID = "*******";
const char *PWD = "*******";
const char* url = "http://***.***.***.***:****/";
int lastState = HIGH; 
int currentState;     
WebServer server(80);  

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, LED_TYPE);
StaticJsonDocument<500> jsonDocument;
char buffer[250];

const byte MLX90640_address = 0x33;

#define TA_SHIFT 8 

float mlx90640To[768];
paramsMLX90640 mlx90640;


void getRandomStr(char* output, int len){
    char* eligible_chars = "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz1234567890";
    for(int i = 0; i< len; i++){
        uint8_t random_index = random(0, strlen(eligible_chars));
        output[i] = eligible_chars[random_index];
    }
}

int cameraSetup(void) {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_QVGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 50;
  config.fb_count = 1;
  
  if(psramFound()){
    config.jpeg_quality = 5;
    config.fb_count = 1;
    //config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    // Limit the frame size when PSRAM is not available
    config.frame_size = FRAMESIZE_SVGA;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return 0;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  s->set_brightness(s, 1); // up the brightness just a bit
  return 1;
}


void sendPhoto() {
    DynamicJsonDocument doc(30000);
    size_t fblen;
    Serial.printf("Ready to shoot\n");
    char random_key[32];
    getRandomStr(random_key, 32);
    //char iv[16];
    //getRandomStr(iv, 16);
    char *public_key_hc="-----BEGIN RSA PUBLIC KEY-----**************-----END RSA PUBLIC KEY-----\n";
    unsigned char encrypted_key[MBEDTLS_MPI_MAX_SIZE];
    size_t encrypted_len = 0;
    size_t encrypted_len_iv=0;
    mbedtls_pk_context pk;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    int ret=0;
    mbedtls_pk_init(&pk);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);
    
  
    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)random_key, strlen(random_key))) != 0) {
        printf("mbedtls_ctr_drbg_seed returned %d\n", ret);
        ESP.restart();
    }

    if(ret=(mbedtls_pk_parse_public_key(&pk, (const unsigned char *) public_key_hc, strlen(public_key_hc) + 1)) != 0)
    {
      printf( " failed\n  ! mbedtls_pk_parse_public_key returned -0x%04x\n", -ret );
      ESP.restart();
    }
   

    if (ret=(mbedtls_pk_encrypt(&pk, (const unsigned char*)random_key, sizeof(random_key), encrypted_key, &encrypted_len, sizeof(encrypted_key), mbedtls_ctr_drbg_random, &ctr_drbg)) !=0)
    { printf( " failed\n  ! mbedtls_pk_encrypt returned -0x%04x\n", -ret );
      ESP.restart();
    }


    String encrypted_key64= base64::encode(encrypted_key, encrypted_len);
  
    mbedtls_pk_free(&pk);
    delay(1000);
      for (byte x = 0 ; x < 2 ; x++)
      {
        uint16_t mlx90640Frame[834];
        int status = MLX90640_GetFrameData(MLX90640_address, mlx90640Frame);

        float vdd = MLX90640_GetVdd(mlx90640Frame, &mlx90640);
        float Ta = MLX90640_GetTa(mlx90640Frame, &mlx90640);

        float tr = Ta - TA_SHIFT; //Reflected temperature based on the sensor ambient temperature
        float emissivity = 0.95;

        MLX90640_CalculateTo(mlx90640Frame, &mlx90640, emissivity, tr, mlx90640To);
      }
        String mlxs;
        for (int i=0; i < 768; i++){
        mlxs+=String(mlx90640To[i])+", ";
        }
        String mlx64=base64::encode(mlxs);
        mlxs.clear();

    delay (1000);
    camera_fb_t * fb = NULL;
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Foto non riuscita");
      ESP.restart();
    }
    esp_camera_fb_return(fb);
    fblen=fb->len;
    int padding_bytes = 16 - (fblen % 16);
    uint8_t *padded_data = (uint8_t *) malloc(fblen + padding_bytes);
    memcpy(padded_data, fb->buf, fblen);
    for (int i = 0; i < padding_bytes; i++)
    {
        padded_data[fblen + i] = (uint8_t)padding_bytes;
    }
    
    uint8_t *encrypted_data = (uint8_t *) malloc(fblen + padding_bytes);
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, (const unsigned char*) random_key, 256);
    for (int i = 0; i < fblen + padding_bytes; i += 16){
    //mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT,sizeof(fb->buf +i), (unsigned char*)iv, fb->buf + i, encrypted_data+i);
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT,fb->buf + i, encrypted_data+i);
    }
    Serial.println(sizeof(encrypted_data));
    free(padded_data);
    free(fb->buf);
    String img64= base64::encode((uint8_t *) encrypted_data, fblen + padding_bytes);

    mbedtls_aes_free(&aes);
    
    if (img64.length()<9000){
      Serial.println("Error in image encoding, restart\n");
      ESP.restart();
    }
  
    String json_string="res="+img64+"&mlx="+mlx64+"&key="+encrypted_key64;
    Serial.println(json_string.length());

    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpResponseCode = http.POST(json_string);
    http.end();
    encrypted_key64.clear();
    mlx64.clear();
    img64.clear();
    json_string.clear();   
    }

void receiveFeedback(){
  String body = server.arg("plain");
  deserializeJson(jsonDocument, body);
  String input=jsonDocument["flag"];
  String key=jsonDocument["key"];

  char inputCh[input.length() + 1];
  strncpy(inputCh, input.c_str(), input.length());
  inputCh[input.length()] = '\0';

  char keyCh[key.length() + 1];
  strncpy(keyCh, key.c_str(), key.length());
  keyCh[key.length()] = '\0';

   String priv_key_esp1="-----BEGIN RSA PRIVATE KEY-----\n***********\n";
  String priv_key_esp2="*********************************\n";
  String priv_key_esp3="";
  String priv_key_esp4="";
  String priv_key_esp5="";
  String priv_key_esp6="";
  String priv_key_esp7="";
  String priv_key_esp8="*********-----END RSA PRIVATE KEY-----\n";
  
  unsigned char encryptedRSA[256];
  size_t outlen;
  int ret = 0;
  if ((ret=mbedtls_base64_decode(encryptedRSA, sizeof(encryptedRSA), &outlen, reinterpret_cast<unsigned char*>(inputCh), strlen(inputCh))) != 0){
    printf("  failed\n ! mbedtls_base64_decode returned -ox%04x\n", -ret);
  }

  unsigned char encryptedKey[256];
  size_t outlenKey;
  ret = 0;
  if ((ret=mbedtls_base64_decode(encryptedKey, sizeof(encryptedKey), &outlenKey, reinterpret_cast<unsigned char*>(keyCh), strlen(keyCh))) != 0){
    printf("  failed\n ! mbedtls_base64_decode returned -ox%04x\n", -ret);
  }

  ret=0;
  mbedtls_pk_context pk;
  mbedtls_pk_init(&pk);
  mbedtls_ctr_drbg_context ctr_drbg;  
  mbedtls_ctr_drbg_init(&ctr_drbg);
  mbedtls_entropy_context entropy;
  mbedtls_entropy_init(&entropy);
  char random_key[32];
  getRandomStr(random_key, 32);
  if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)random_key, strlen(random_key))) != 0) {
        printf("mbedtls_ctr_drbg_seed returned %d\n", ret);
        ESP.restart();
  }
  ret=0;
  if((ret = mbedtls_pk_parse_key(&pk, (reinterpret_cast<unsigned char*>(priv_key_esp)), (strlen(priv_key_esp)+1), NULL, NULL)) != 0 )
    {
        printf(" failed\n  ! mbedtls_pk_parse_key returned -0x%04x\n", -ret);
        char errorBuf[100];
        mbedtls_strerror(ret, errorBuf, sizeof(errorBuf));
        Serial.printf("Errore durante la decrittazione: %s\n", errorBuf);
  
    }
  unsigned char resultKey[MBEDTLS_MPI_MAX_SIZE];
  size_t olen = 0;
  
    if((ret = mbedtls_pk_decrypt(&pk, encryptedKey, sizeof(encryptedKey), resultKey, &olen, sizeof(resultKey), mbedtls_ctr_drbg_random, &ctr_drbg)) !=0)
    {
        printf( " failed\n  ! mbedtls_pk_decrypt returned -0x%04x\n", -ret );
        char errorBuf[100];
        mbedtls_strerror(ret, errorBuf, sizeof(errorBuf));
        Serial.printf("Errore durante la decrittazione: %s\n", errorBuf);
    }
  

  unsigned char* output_final = (unsigned char*) malloc (16*(sizeof(unsigned char)));
  mbedtls_aes_context aes; 
  mbedtls_aes_init(&aes);	
  mbedtls_aes_setkey_dec(&aes, resultKey, 128);
  int result=mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, encryptedRSA, output_final);
  if (result != 0) {
    printf("Error AES: %d\n", result);
    return;
}
  mbedtls_aes_free(&aes);
  mbedtls_pk_free(&pk);

  String resultString=reinterpret_cast<char*>(output_final);
  resultString=resultString.substring(0,16);
  Serial.println(resultString);

  char* cmp="alsoeyatdfguinbg";
  int str_len = resultString.length() + 1; 
  char char_array[str_len];
  resultString.toCharArray(char_array, str_len);
  if ((strcmp(char_array, cmp))==0)
  {
  Serial.println("Recognized");
  strip.setPixelColor(0, 0, 255, 0); // Imposta il colore del primo LED a verde
  strip.show(); // Applica il cambio di colore ai LED
  delay(2000);

  }
  else
  {
  Serial.println("Not recognized");
  strip.setPixelColor(0, 255, 0, 0);
  strip.show();
  delay (500); 
  strip.setPixelColor(0, 0, 0, 0);
  strip.show();
  delay (500);
  strip.setPixelColor(0, 255, 0, 0);
  strip.show();
  delay (500);
  strip.setPixelColor(0, 0, 0, 0);
  strip.show();
  
  delay(1000);
  }
  server.send(200); 
  resultString.clear();
  ESP.restart();

  
}

void setup() {
  Serial.flush();
  Serial.begin(115200);
  cameraSetup();
  Wire.begin(36,42);
  Wire.setClock(400000); 
  Serial.println("Connecting to Wi-Fi");
  WiFi.begin(SSID, PWD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  int status;
  uint16_t eeMLX90640[832];
  status = MLX90640_DumpEE(MLX90640_address, eeMLX90640);
  if (status != 0)
    Serial.println("Failed to load system parameters");

  status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
  if (status != 0)
    Serial.println("Parameter extraction failed");

  MLX90640_SetRefreshRate(MLX90640_address, 0x03); //Set rate to 4Hz
  strip.begin();
  strip.show();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());   
  server.on("/esp", HTTP_POST, receiveFeedback);
  server.begin();
}

void loop() {
  strip.setPixelColor(0, 255, 255, 255); // Imposta il colore del primo LED a bianco
  strip.show();
  sendPhoto();
  server.handleClient();
  delay(10000);
}


