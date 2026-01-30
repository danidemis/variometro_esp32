#include "NMEA.h"
#include <Arduino.h>
#include <cstdio>

// Includiamo le librerie per il Bluetooth Low Energy (BLE)
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Oggetti globali per il server BLE
BLEServer* pServer = NULL;
BLECharacteristic* pTxCharacteristic = NULL; // Caratteristica per trasmettere dati (TX)
bool deviceConnected = false;

// UUIDs standard per il servizio Nordic UART (NUS)
// XCTrack e altre app li riconoscono automaticamente
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// Classe per gestire gli eventi di connessione e disconnessione
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Dispositivo connesso via BLE.");
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Dispositivo disconnesso.");
      // Ricominciamo l'advertising per essere di nuovo visibili
      pServer->getAdvertising()->start();
    }
};

// Funzione helper per il checksum NMEA (INVARIATA)
byte calculate_checksum(const char *str) {
  byte checksum = 0;
  while (*str) {
    checksum ^= *str++;
  }
  return checksum;
}

void nmea_init() {
  // 1. Inizializza il dispositivo BLE e imposta il nome
  BLEDevice::init("VarioESP32");

  // 2. Crea il Server BLE
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // 3. Crea il Servizio UART
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // 4. Crea una Caratteristica per la trasmissione (TX)
  pTxCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  // Aggiungiamo un descrittore standard alla caratteristica
  pTxCharacteristic->addDescriptor(new BLE2902());

  // 5. Avvia il servizio
  pService->start();

  // 6. Avvia l'advertising, così il telefono può trovarci
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();
  
  Serial.println("Server BLE avviato e in attesa di connessioni.");
}

void nmea_send(const SensorData &data) {
  char payload[80]; // Buffer leggermente più grande per la nuova frase
  char final_sentence[90];

  // 1. Convertiamo i dati nei formati richiesti dal formato LK8EX1
  int pressure_pa = data.pressure * 100;    // Pressione in Pascal (intero)
  int altitude_m = data.filtered_altitude; // Altitudine in metri (intero)
  int vario_cmps = data.vario_mps * 100;     // Vario in cm/s (intero)
  float temp_c = data.temperature;         // Temperatura in Celsius (float)
  int battery_level = 100;                   // Valore fisso per la batteria (100%)

  // 2. Creiamo il "payload" con il nuovo formato LK8EX1
  // $LK8EX1,pressure,altitude,vario,temperature,battery,*checksum
  snprintf(payload, sizeof(payload), "LK8EX1,%d,%d,%d,%.1f,%d", 
           pressure_pa, 
           altitude_m, 
           vario_cmps, 
           temp_c, 
           battery_level);

  // 3. Calcoliamo il checksum (la funzione è la stessa)
  byte checksum = calculate_checksum(payload);

  // 4. Assembliamo la frase finale completa
  snprintf(final_sentence, sizeof(final_sentence), "$%s*%02X\r\n", payload, checksum);

  // Inviamo sempre sulla seriale USB per debug
  Serial.print("Invio dati: ");
  Serial.print(final_sentence);

  // Se un dispositivo è connesso, invia i dati tramite notifica BLE
  if (deviceConnected) {
    pTxCharacteristic->setValue((uint8_t*)final_sentence, strlen(final_sentence));
    pTxCharacteristic->notify();
  }
}