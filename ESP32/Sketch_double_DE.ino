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


#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define LED_GPIO_NUM      33


const char *SSID = "*******";
const char *PWD = "*******";
const char* url = "http://***.***.***.***:8000/";

int lastState = HIGH; 
int currentState;

// Those two are the state of the button

WebServer server(80);  


StaticJsonDocument<500> jsonDocument;
char buffer[250];
// This is the declaration of the JSON document to store the information coming from the server


void getRandomStr(char* output, int len){
    char* eligible_chars = "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz1234567890";
    for(int i = 0; i< len; i++){
        uint8_t random_index = random(0, strlen(eligible_chars));
        output[i] = eligible_chars[random_index];
    }
}


void sendPhoto() {
    Serial.printf("Pronto per foto\n");
    char random_key[32];
    getRandomStr(random_key, 32);
    char *public_key_hc="-----BEGIN RSA PUBLIC KEY-----\n*****************************\n**********************\n/************************\n************************\n********************\n-----END RSA PUBLIC KEY-----\n";
    unsigned char encrypted_key[MBEDTLS_MPI_MAX_SIZE];
    size_t encrypted_len = 0;
    mbedtls_pk_context pk;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    int ret=0;
    mbedtls_pk_init(&pk);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);
    // Ctr_drbg and entropy are vital for RSA to work, so don't delete them
  
    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)random_key, strlen(random_key))) != 0) {
        printf("mbedtls_ctr_drbg_seed returned %d\n", ret);
        ESP.restart();
    }

    // The random string is also used to initialize the random seed

    if(ret=(mbedtls_pk_parse_public_key(&pk, (const unsigned char *) public_key_hc, strlen(public_key_hc) + 1)) != 0)
    {
      printf( " failed\n  ! mbedtls_pk_parse_public_key returned -0x%04x\n", -ret );
      ESP.restart();
    }

    // This is the initialization of the RSA envirronment

    if (ret=(mbedtls_pk_encrypt(&pk, (const unsigned char*)random_key, sizeof(random_key), encrypted_key, &encrypted_len, sizeof(encrypted_key), mbedtls_ctr_drbg_random, &ctr_drbg)) !=0)
    { printf( " failed\n  ! mbedtls_pk_encrypt returned -0x%04x\n", -ret );
      Serial.println(ret);
      ESP.restart();
    } 
    
    String encrypted_key64= base64::encode(encrypted_key, encrypted_len);
    // There you crypt (and then encode in base64) the key you're going to use with AES encryption
    
    mbedtls_pk_free(&pk);

    // We free some space as we no longer need RSA encryption

    delay(1000);
    camera_fb_t * fb = NULL;
    digitalWrite(4, HIGH);
    fb = esp_camera_fb_get();
    digitalWrite(4, LOW);
    if (!fb) {
      Serial.println("Photo not taken");
      ESP.restart();
    }
    esp_camera_fb_return(fb);

    // This is where you actually take the photo
  
    DynamicJsonDocument doc(20000);
    int padding_bytes = 16 - (fb->len % 16);
    uint8_t *padded_data = (uint8_t *) malloc(fb->len + padding_bytes);
    memcpy(padded_data, fb->buf, fb->len);
    for (int i = 0; i < padding_bytes; i++)
    {
        padded_data[fb->len + i] = (uint8_t)padding_bytes;
    }
    
    uint8_t *encrypted_data = (uint8_t *) malloc(fb->len + padding_bytes);

    // Padding is pretty much always necessary
  
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, (const unsigned char*) random_key, 256);
    for (int i = 0; i < fb->len + padding_bytes; i += 16){
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT,fb->buf + i, encrypted_data+i);
    }
    String img64= base64::encode((uint8_t *) encrypted_data, fb->len + padding_bytes);
    if (img64.length()<9000){
      Serial.println("Errore nella codifica dell'immagine, riavvio\n");
      ESP.restart();
    }

    /* I noticed that when the photo is incorrect (probably because it's too dark or out of focus) the lenght of the base64 encoding
      is much lower (when the photo is correct it's usually long somewhere between 11k and 15k), so 9k is the minimum lenght to
      judge the photo as correct, otherwise it has to be taken again.
    */
    
    String json_string="res="+img64+"&key="+encrypted_key64;
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
    /* The application/json content-type doesn't work. This content type doesn't recognize the "+" sign 
    leaving a blank space instead. I substituted the spaces with the pluses on the server script */
    
    int httpResponseCode = http.POST(json_string);
    mbedtls_aes_free(&aes);
    http.end();
    encrypted_key64.clear();
    img64.clear();
    json_string.clear();
    free(padded_data);
    free(encrypted_data);
    }

void receiveFeedback(){
  String body = server.arg("plain");
  deserializeJson(jsonDocument, body);
  String input=jsonDocument["flag"];
  String key=jsonDocument["key"];
  Serial.println(input);
  Serial.println(key);

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
  String priv_key_espS=priv_key_esp1+priv_key_esp2+priv_key_esp3+priv_key_esp4+priv_key_esp5+priv_key_esp6+priv_key_esp7+priv_key_esp8;
  char* priv_key_esp=&priv_key_espS[0];

  /* As the private key is very long, i suggest to slice it in 8 pieces and then build the string.
  Because we need it as a char array, you have to "cast" it then from the String type */
  

  
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

  // Decoding from base64

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
        Serial.printf("Error during decryption: %s\n", errorBuf);
  
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

  // The result string has some redundant characters, so we discard them

  char* cmp="alsoeyatdfguinbg";
  int str_len = resultString.length() + 1; 
  char char_array[str_len];
  resultString.toCharArray(char_array, str_len);

  // The strings are compared
  
  if ((strcmp(char_array, cmp))==0)
  {
  Serial.println("Riuscito");
  digitalWrite(15, LOW);
  digitalWrite (14, HIGH); 
  //Quando l'utente viene riconosciuto, il led Rosso si spegne mentre quello verde si accende per due secondi
  delay(2000);

  }
  else
  {
  Serial.println("Non riuscito");
  digitalWrite(14, LOW);
  digitalWrite (15, LOW);
  delay (500);
  digitalWrite(15, HIGH);
  delay(500);
  digitalWrite(15, LOW);
  delay(500);
  digitalWrite(15, HIGH); 
  //Quando l'utente non viene riconosciuto, il Led verde rimane spento mentre quello rosso lampeggia
  delay(1000);
  }
  server.send(200); 
  resultString.clear();
}

void setup() {
  pinMode(4, OUTPUT);
  pinMode(13, INPUT_PULLUP);
  pinMode(15, OUTPUT);
  pinMode(14, OUTPUT);
  
  // On PIN 4 there is the flash led, on the 13 the button and on 14 and 15 two LEDs
  
  Serial.flush();
  Serial.begin(115200);
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
  config.pixel_format = PIXFORMAT_JPEG;
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 5;
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 5;
    config.fb_count = 1;
  }

  // You can try various combinations for quality

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  Serial.begin(115200);
  SPIFFS.begin(true);
  Serial.println("Connecting to Wi-Fi");
  WiFi.begin(SSID, PWD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());   
  server.on("/esp", HTTP_POST, receiveFeedback);
  server.begin();
}

void loop() {
  digitalWrite(14, LOW);
  digitalWrite(15, HIGH);

  /* currentState = digitalRead(13);
  if(lastState == LOW && currentState == HIGH){
  sendPhoto();
  server.handleClient();
  }
  lastState = currentState;} */

  // The commented code is the version with the button
  
  sendPhoto();
  server.handleClient();
  delay(5000);
}
