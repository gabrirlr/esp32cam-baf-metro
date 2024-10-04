#include "SD_MMC.h"
#include "WiFi.h"  
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <BLEAdvertisedDevice.h>
#include "Audio.h"

static BLEAddress deviceAddress("22:34:50:10:7c:86");

static BLEUUID characteristicUUID("0000ffe2-0000-1000-8000-00805f9b34fb");

const uint8_t DEVICE_STATE_STARTED[] = {0xaa, 0x05, 0xa2, 0x00, 0xfe};
const uint8_t DEVICE_STATE_WAITING[] = {0xaa, 0x05, 0xa3, 0x00, 0xfe};
const uint8_t DEVICE_STATE_READY[] = {0xaa, 0x05, 0xa4, 0x00, 0xfe};
const uint8_t DEVICE_STATE_BLOW[] = {0xaa, 0x05, 0xa5, 0x00, 0xfe};

BLEClient* client;
BLERemoteCharacteristic* remoteCharacteristic;
BLEAdvertisedDevice* advertisedDevice;

#define I2S_DOUT 12     
#define I2S_BCLK 13     
#define I2S_LRC 4       

Audio audio;
const char* sdAudioFilePath1 = "/audio1.mp3"; //será necessario realizar o teste
const char* sdAudioFilePath2 = "/audio2.mp3"; //coloque o bocal
const char* sdAudioFilePath3 = "/audio3.mp3"; //assopre
const char* sdAudioFilePath4 = "/audio4.mp3"; //teste finalizado com sucesso

float convertToMgL(uint8_t* data, size_t length) {
  if (length < 9 || data[0] != 0xaa || data[1] != 0x09) {
    Serial.println("Formato de dados inválido ou incompleto.");
    return -1.0;
  }
  uint32_t reading = 0;
  for (int i = 2; i < 6; i++) {
    reading = (reading << 8) | data[i];
  }
  return reading / 1000.0;
}

void notificationCallback(BLERemoteCharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify) {
  if (memcmp(data, DEVICE_STATE_STARTED, sizeof(DEVICE_STATE_STARTED)) == 0) {
    Serial.println("O dispositivo foi ligado.");
    
  } else if (memcmp(data, DEVICE_STATE_WAITING, sizeof(DEVICE_STATE_WAITING)) == 0) {
    Serial.println("Aguarde até utilizar o dispositivo.");
    audio.connecttoFS(SD_MMC, sdAudioFilePath2);
  } else if (memcmp(data, DEVICE_STATE_READY, sizeof(DEVICE_STATE_READY)) == 0) {
    Serial.println("O dispositivo está pronto.");
    audio.connecttoFS(SD_MMC, sdAudioFilePath3);
  } else if (memcmp(data, DEVICE_STATE_BLOW, sizeof(DEVICE_STATE_BLOW)) == 0) {
    Serial.println("Dispositivo sendo utilizado.");
  } else {
    float reading = convertToMgL(data, length);
    if (reading >= 0) {
      Serial.print("Leitura de álcool: ");
      Serial.print(reading);
      Serial.println(" mg/L");
      audio.connecttoFS(SD_MMC, sdAudioFilePath4);
    }
  }
}

void audio_info(const char* info) {
  Serial.print("info        "); Serial.println(info);
}
void audio_id3data(const char* info) {  
  Serial.print("id3data     "); Serial.println(info);
}
void audio_eof_mp3(const char* info) {  
  Serial.print("eof_mp3     "); Serial.println(info);
}
bool connectToServer() {
  client = BLEDevice::createClient();
  audio.connecttoFS(SD_MMC, sdAudioFilePath1);
  if (!client->connect(deviceAddress)) {
    Serial.println("Falha na conexão ao dispositivo.");
    return false;
  };
  
  BLERemoteService* remoteService = client->getService(BLEUUID("0000ffe0-0000-1000-8000-00805f9b34fb"));
  if (remoteService == nullptr) {
    Serial.println("Falha ao encontrar o serviço.");
    client->disconnect();
    return false;
  }

  remoteCharacteristic = remoteService->getCharacteristic(characteristicUUID);
  if (remoteCharacteristic == nullptr) {
    Serial.println("Falha ao encontrar a characteristic.");
    client->disconnect();
    return false;
  }

  if (remoteCharacteristic->canNotify()) {
    remoteCharacteristic->registerForNotify(notificationCallback);
  }
  
  Serial.println("Conectado e monitorando bafômetro...");
  return true;
}
void setup() {
  Serial.begin(115200);
  BLEDevice::init("");

  while (!connectToServer()) {
    Serial.println("Tentando novamente em 5 segundos...");
    delay(5000);
  }
  if (!SD_MMC.begin("/sdcard", true)) {  
    Serial.println("Falha na inicialização do cartão SD");
    return;
  }
  Serial.println("Cartão SD inicializado com sucesso no modo 1-bit.");

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);

  audio.setVolume(40);  
}

void loop() {
  audio.loop();
}