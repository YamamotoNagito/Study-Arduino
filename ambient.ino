//Wi-Fiに関するヘッダをインクルード
#include <WiFi.h>
#include <WiFiClient.h>

#include <M5StickC.h>
#include <driver/i2s.h>
#define PIN_CLK  0
#define PIN_DATA 34
#define READ_LEN (2 * 256)

uint8_t BUFFER[READ_LEN] = {0};
uint16_t oldy[160];
int16_t *adcBuffer = NULL;

int y = 0;

//Ambientのヘッダをインクルード
#include "Ambient.h"  
  
WiFiClient client;
Ambient ambient; //Ambientオブジェクトを定義]

//Wi-Fiに接続するためにSSID, パスワードを入力

const char* ssid = "";
const char* password = "";

unsigned int channelId = ; //AmbientのチャネルID
const char* writeKey = "";  //ライトキー

void i2sInit(){
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
    .sample_rate =  44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // is fixed at 12bit, stereo, MSB
    .channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 2,
    .dma_buf_len = 128,
  };
   
   i2s_pin_config_t pin_config;
   pin_config.bck_io_num   = I2S_PIN_NO_CHANGE;
   pin_config.ws_io_num    = PIN_CLK;
   pin_config.data_out_num = I2S_PIN_NO_CHANGE;
   pin_config.data_in_num  = PIN_DATA;
   
   i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
   i2s_set_pin(I2S_NUM_0, &pin_config);
   i2s_set_clk(I2S_NUM_0, 44100, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
}
  
void mic_record_task (void* arg){   
  size_t bytesread;
  while(1){
    i2s_read(I2S_NUM_0,(char*) BUFFER, READ_LEN, &bytesread, (100 / portTICK_RATE_MS));
    adcBuffer = (int16_t *)BUFFER;
    showSignal();
    vTaskDelay(100 / portTICK_RATE_MS);
  }
}
void setup() {
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE, BLACK);
  //M5.Lcd.println("mic test");
  //M5.Lcd.println("########################################");
  WiFi.begin(ssid, password); //Wi-Fi APに接続 ----A
    while(WiFi.status() != WL_CONNECTED){ //Wi-Fi AP接続待ち
      delay(500);
      M5.Lcd.print(".");
  }
  M5.Lcd.print(1);

  M5.Lcd.print("WiFi connected\r\nIP address: ");
  M5.Lcd.println(WiFi.localIP());
  M5.Lcd.println("\n");
  i2sInit();
  xTaskCreate(mic_record_task, "mic_record_task", 2048, NULL, 1, NULL);

  ambient.begin(channelId, writeKey, &client); // チャネルIDとライトキーを指定してAmbientの初期化
}

void showSignal(){
 int32_t offset_sum = 0;
  for (int n = 0; n < 160; n++) {
    offset_sum += (int16_t)adcBuffer[n];
  }
  int offset_val = -( offset_sum / 160 );
  // Auto Gain
  int max_val = 200;
  for (int n = 0; n < 160; n++) {
    int16_t val = (int16_t)adcBuffer[n] + offset_val;
    //M5.Lcd.println(val);
    if ( max_val < abs(val) ) {
      max_val = abs(val);
    }
  }
  
  // int y;
  for (int n = 0; n < 160; n++){
    y = adcBuffer[n] + offset_val;
    y = map(y, -max_val, max_val, 10, 70);
    M5.Lcd.drawPixel(n, oldy[n],BLACK); //グラフを描画
    M5.Lcd.drawPixel(n,y,WHITE);        //グラフを消去
    oldy[n] = y;
    /*
    ambient.set(1, String(y).c_str());

    ambient.send();
    */
    
  }
}

void loop() {
  printf("loop cycling\n");
  ambient.set(1, String(y).c_str());

  ambient.send();
  vTaskDelay(1000 / portTICK_RATE_MS); // otherwise the main task wastes half of the cpu cycles
}
