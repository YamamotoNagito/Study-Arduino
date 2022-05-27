#include <M5StickC.h>
#include <driver/i2s.h>

#include <SD.h>  //SDカードを使用するためのライブラリ

SPIClass SPI2;

#define PIN_CLK  0
#define PIN_DATA 34
#define READ_LEN (2 * 256)

uint8_t BUFFER[READ_LEN] = {0};
uint16_t oldy[160];
int16_t *adcBuffer = NULL;

int y = 0;

File file;  //ファイルを保存するための変数を用意する

//閾値を定義しておく

#define threshold 70
int threshold_count = 0;

#define GPI0_NUM_10 10

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

void showSignal(){
  int32_t offset_sum = 0;
  for (int n = 0; n < 160; n++) {
    offset_sum += (int16_t)adcBuffer[n];
  }
  int offset_val = -( offset_sum / 160 );
  // Auto Gain
  int max_val = 300;
  for (int n = 0; n < 160; n++) {
    int16_t val = (int16_t)adcBuffer[n] + offset_val;
    //M5.Lcd.println(val);
    if ( max_val < abs(val) ) {
      max_val = abs(val);
    }
  }
  
  // int y;
  int max = 0;
  for (int n = 0; n < 160; n++){
    y = adcBuffer[n] + offset_val;

//    最大値の更新
    if(max < y){
      max = y;
    }
    
    y = map(y, -max_val, max_val, 10, 70);
//  M5.Lcd.drawPixel(n, oldy[n],BLACK); //グラフを消去
//  M5.Lcd.drawPixel(n,y,WHITE);        //グラフを描画
    oldy[n] = y;

    
  }
    
//    M5.Lcd.print(max);
//    M5.Lcd.fillScreen(BlACK);
    
//    for分の外であれば可能

  if(y < threshold){
        threshold_count += 1;
  }
  M5.Lcd.print(threshold_count);
  
  if(threshold_count > 20){
    M5.Lcd.setTextColor(RED);
    M5.Lcd.print("error: Below the threshold");
    pinMode(GPI0_NUM_10, OUTPUT);

    digitalWrite(GPIO_NUM_10, LOW);
    delay(1000);
    digitalWrite(GPIO_NUM_10, HIGH);
    delay(1000);
    threshold_count = 0;
  }
  else{
      threshold_count = 0;
  }
    
    file = SD.open("/test.txt", FILE_WRITE);

    
//  もしファイルを開くことができたら、値を書き込む
    if(file){
      //ファイルを開けているかデバッグ
      M5.Lcd.println("11111111111111111111111111111111111111111111111111111111111");
      file.println(max);
      file.close();

      M5.Lcd.println(max);
    }
//  ファイルを開くことができなっかた場合に、エラー表示をする
    else{
      M5.Lcd.println("error open test.txt");
    }
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


 //SDカード用SPIポート初期化（SCK:HAT_G0, MISO:HAT_G36, MOSI:HAT_G26, SS:物理ピン無し"-1"）
  SPI2.begin(0, 36, 26, -1);

   //SDカード初期設定
  if (!SD.begin(-1, SPI2)){
    M5.Lcd.println("SD Card Mount Failed!");
    return;
  }

  //SDカード種別取得
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    M5.Lcd.println("None SD Card!");
    return;
  } else if (cardType == CARD_MMC) {
    M5.Lcd.println("SD Card Type: MMC");
  } else if (cardType == CARD_SD) {
    M5.Lcd.println("SD Card Type: SDSC");
  } else if (cardType == CARD_SDHC) {
    M5.Lcd.println("SD Card Type: SDHC");
  } else {
    M5.Lcd.println("SD Card Type: UNKNOWN");
  }

  //SDカード容量取得
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  M5.Lcd.printf("SD Card Size: %lluMB\n", cardSize);
  M5.Lcd.println();

  //SDカードのルートディレクトリを開く
  File dir = SD.open("/");

  i2sInit();
  xTaskCreate(mic_record_task, "mic_record_task", 2048, NULL, 1, NULL);
}


void loop() {
// M5.Lcd.print("3 ");
  
//  ambient.set(1, String(y).c_str());
//
//  ambient.send();

   delay(1000);

//  vTaskDelay(1000 / portTICK_RATE_MS); // otherwise the main task wastes half of the cpu cycles
}
