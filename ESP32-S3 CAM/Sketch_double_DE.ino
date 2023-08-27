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

/*
const char *SSID = "iPhone di Matteo Piccadaci";
const char *PWD = "CiaoMatteo";
const char* url = "http://172.20.10.7:8000/";
*/

const char *SSID = "Wind3 HUB - FC6A79";
const char *PWD = "7satetyyue34y9xa";
const char* url = "http://192.168.1.5:8000/";
int lastState = HIGH;
int currentState;     
WebServer server(80); //Inizializzazione del web server  

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, LED_TYPE);
StaticJsonDocument<500> jsonDocument;
char buffer[250];



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
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 20;
  config.fb_count = 1;
  
  if(psramFound()){
    config.jpeg_quality = 5;
    config.fb_count = 1;

  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return 0;
  }

  sensor_t * s = esp_camera_sensor_get();
  s->set_brightness(s, 1); 
  return 1;
}


void sendPhoto() {
  /* 
  È la prima routine che la scheda segue: al momento dell'attivazione (dunque alla pressione di un pulsante o 
  al passare di un determinato lasso di tempo), si inizializza il documento JSON che successivamente verrà popolato.
  Viene dunque generata casualmente la chiave simmetrica per poi effettuare la cifratura AES. A questo punto si cifra la chiave appena generata 
  con la chiave pubblica del server traimite l'algoritmo RSA e si codifica in base64. Adesso si scatta l'immagine e la si cifra tramite AES, impostando
  come chiave quella precentemente generata. Una volta terminato, l'output viene codificato in base64. Viene effettuato un controllo sulla lunghezza della
  codifica, come spiegato sotto. Una volta terminato il controllo, si prepara la stringa JSON e viene dunque inviata al Server.
  */
    DynamicJsonDocument doc(20000);
    size_t fblen;
    Serial.printf("Ready to shoot\n");
    char random_key[32];
    getRandomStr(random_key, 32);
    char *public_key_hc="-----BEGIN RSA PUBLIC KEY-----\nMIIBCgKCAQEArrFm6zDyYDd16BwixjbkLZjpoWi2Juwkoc1cQcfneDbgTMcPZ3Ay\nxCeLL324u0Hs3dG6wSQGRA989ar/tp13HjP1ARlKvYqrvEDwsEuWFUZDvqEhfd5F\n/z5WvDqlq01FLC6LwpimtJn5ciuXs01g1aLuzg/09vlFBhel5odl8KOJAzjCZlhf\njn8ngt22iuMX/e2ZhVH4tvx3bL7XGLQmu/djGoaq5lcxhanrPlEpYRldSejFurI+\naEr/XmXaJwU/eGukxwMc9MAb/QEQ3RFbethGy5VBgZPIQ+YDlq60BHUUzXm13ArW\n9oxeSWbUMzzFtxKIV2jSyOpFA82Zh9AH4QIDAQAB\n-----END RSA PUBLIC KEY-----\n";
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
    // Sia ctr_;drbg che entropy sono necessarie per il funzionamento dell'RSA
  
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
    }// Crittazione chiave


    String encrypted_key64= base64::encode(encrypted_key, encrypted_len);
    // codifico in B64 per inviare chiave criptata in JSON
    mbedtls_pk_free(&pk);
    delay(1000);
      
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
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT,fb->buf + i, encrypted_data+i);
    }
    Serial.println(sizeof(encrypted_data));
    String img64= base64::encode((uint8_t *) encrypted_data, fblen + padding_bytes);
    free(padded_data);
    free(fb->buf);
    //String img64= base64::encode((uint8_t *) encrypted_data, fblen + padding_bytes);

    mbedtls_aes_free(&aes);
    
    if (img64.length()<9000){
      Serial.println("Errore nella codifica dell'immagine, riavvio\n");
      ESP.restart();
    }
  
    /*È stato notato che quando la foto è sbagliata (prende ad esempio mezza faccia o l'obiettivo è coperto quindi la foto viene nera)
    la lunghezza della codifica in B64 è molto minore (Quando viene correttamente varia tra gli 11k e i 15k),
    quindi si è scelta 9000 come soglia minima per la quale la foto è giudicata accettabile*/


    String json_string="res="+img64+"&key="+encrypted_key64;
   
    Serial.println(img64);
    Serial.println("encrypted_key");
    Serial.println(encrypted_key64);
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpResponseCode = http.POST(json_string);
    http.end();
    encrypted_key64.clear();
    img64.clear();
    json_string.clear();   
    }

void receiveFeedback(){
  /* 
  Una volta che il server ha terminato le sue operazioni, verrà ricevuto un pacchetto JSON contenente
  la chiave AES criptata tramite la chiave pubblica della scheda e la stringa cifrata in AES tramite la chiave pubblica
  precedentemente menzionata, entrambe codificate in base64.
  Vengono dunque decodificati entrambi e, successivamente, si inizializza dunque l'ambiente di decifratura RSA con la chiave privata della scheda.
  Adesso verrà decifrata la chiave AES ed, infine, viene decifrata la stringa. Se essa corrisponderà a quella prevista, si accenderà il LED verde, altrimenti 
  lampeggierà il LED rosso.
  */
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

  /*
  Alcuni lavori per rendere utilizzabili le stringhe ricevute dal Server
  */

  String priv_key_esp1="-----BEGIN RSA PRIVATE KEY-----\nMIIEpAIBAAKCAQEA0olstSyHl9Iv1R0NEWL2oUXPhh3pzgS+QlxX0/kwpGJOnEWN\n";
  String priv_key_esp2="fDKz43DkMJwiy3VM+LLF17G01sP2xz5ZGHySg/MN1qb63iDd06B2udlSMYIqZo8V\netVaQX11BqgnmBAEqppAsup1NX03JpvKLq+G+O5RpRbxVhYAi/6VKL+HV+1yjUxL\n";
  String priv_key_esp3="Sxn476v8PINR8iMsvnjd0dLqKxAndhUtolMslkvMAOqTYsPdlEidonVb6YXdXt2/\nWqm+Byel84IKx4WBqw4+BO/1zFNhkDTLD0O/IhgBTdBU1fXEbTt0yLkJVoTB4Yzs\njhHLV8m+xyWp+t9wuFOY8i/vILj3F3VrRtC70QIDAQABAoIBAQCJv45Xj5X7uHLB\nTeuh540UAXgA+QtjVukgxAhW0WCI/SUPy9YBX68g7VjvkQiW891zjowxTrzSE48f\n";
  String priv_key_esp4="12Q1yDOYj2sLeV8D/J8GeqOTf2PuroqNZaqZHoSZ+rqZq2WUcU6MTLZuGUasw4tV\nAHMtTnzR3COhDzzBpU2gSuZOrdmgpbP6uBBTTsz3gsdzJpEHKpUqsiX/672uaFPi\n6ME53gPSH3UvjvYwdGSqvBx8xnBnbdUjn1sg4iRfu8UKcww84zK+6TQX+QBe0NwQ\nhT43zxBRA79iSCWlFuMyPB3M2CDzEadcxWPpFRDQ1CGH8xVmpUAN+yRRpSTbAznr\n";
  String priv_key_esp5="jj6xplPBAoGBAO5JbhkZmm6YzJyCyfC63VjtthWxeuiIS+KGbvYZvE8hmWtWGoQx\ndwQjKqj98orb+yrzyY8/gSkx2L5n4GuTfdgaUKxGlk87xGAt5YNSzpeKBltdfBiK\np//mQv4EHd3p2vfnUwxkJlaJ2/371cc2qRniwgvjFNm8xfPa2LXEwiLJAoGBAOIv\n6jzlVyfdEXiJ1huS937Ha+RY9KIg0sNtXiec8MqZ0YE9tG+zJoZ+8g4F4whf1z6d\n";
  String priv_key_esp6="p/vgZz52OXwDZREusiPiTpBPRDWT+/Oypmf8ww3ir2qqY9GDnksRI5rjcvYB0kJM\n05UmdQY2H10yhdG9paOcXC5JtRcSt4+X9OH5sgzJAoGAQtikozP0h8wjslBaeEbj\nq3vcFc/ZK/x4VU7jN/TWR9ikImFgRO9fdPCsmgXLkbrOhiknxSDKihTqudeINIWG\nyyXutbWDmyyoFVcqyKFlRUu6Js1d78COCpK8/meHPWbKP7tMJ/C8dJBt/50zRpSF\n";
  String priv_key_esp7="8bYUO6NziPWVDqi5HJza3UkCgYA+VI3sMhcOeTEzUmiaOKnTWgk1Z/4iH4F1MVJd\nADaq3jCJuQNcNLZNIfZ6Ps0NpbufGbsNAg3xfIrizdywG3ojwV19Dxrw2NL5mSWa\nSmFGFk3YyxOuzOJ2NUbmi/9GI8JZWuqDk5F0IL4L5LxTzDs1FwWgC9fHf/TtsSZe\nj3ytYQKBgQDPDz/kgB4jsNKJis3vU1xXQE6ufBCBOIqPtIQ1wAKhRB12qHA7sJG3\n";
  String priv_key_esp8="JKH6iWK3taU4dGKE5lt6oDHRXf0TxHN9YuezHiU9iTRmklo1mFZZ2nfLWkZyv3Kh\n9JWTRT1fgAHgHZMqOA63vZ0ynMzIVOIfMo7dqxeVRN8IOOUZGWjhyA==\n-----END RSA PRIVATE KEY-----\n";
  String priv_key_espS=priv_key_esp1+priv_key_esp2+priv_key_esp3+priv_key_esp4+priv_key_esp5+priv_key_esp6+priv_key_esp7+priv_key_esp8;
  char* priv_key_esp=&priv_key_espS[0];
  /*priv_key_esp1.clear();
  priv_key_esp2.clear();
  priv_key_esp3.clear();
  priv_key_esp4.clear();
  priv_key_esp5.clear();
  priv_key_espS.clear();
  */
  // Decifrazione
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
  //Quando l'utente non viene riconosciuto, il Led verde rimane spento mentre quello rosso lampeggia
  delay(1000);
  }
  server.send(200); 
  resultString.clear();
  ESP.restart();

  // A prescindere dal fatto che l'utente venga riconosciuto o meno, al server si invia ok per poter chiudere lo scambio
}

void setup() {
  Serial.flush();
  Serial.begin(115200);
  cameraSetup(); 
  Serial.println("Connecting to Wi-Fi");
  WiFi.begin(SSID, PWD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  strip.begin();
  strip.show();
  Serial.print("Connesso! Indirizzo IP: ");
  Serial.println(WiFi.localIP());   
  server.on("/esp", HTTP_POST, receiveFeedback);
  server.begin();
}

void loop() {

  /*currentState = digitalRead(13);
  if(lastState == LOW && currentState == HIGH){
  sendPhoto();
  server.handleClient();
  }
  lastState = currentState;}*/
  strip.setPixelColor(0, 255, 255, 255); // Imposta il colore del primo LED a bianco
  strip.show();
  sendPhoto();
  server.handleClient();
  delay(5000);
}


