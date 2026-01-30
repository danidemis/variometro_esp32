#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- Configurazioni Schermo OLED ---
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_I2C_ADDRESS 0x3C 

// --- Definizioni Pinout ---
#define PIN_UP 5
#define PIN_DOWN 3
#define PIN_SELECT 4
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9
#define BUZZER_PIN 10
#define PIN_BATT_ADC 1 

// --- Monitoraggio Batteria ---
#define BATT_DIVIDER_RATIO 2.0   

// --- Sensore MS5611 ---
#define MS5611_I2C_ADDRESS 0x77

// --- Stati del Dispositivo ---
enum DeviceMode { 
  MODE_FLIGHT,  // Visualizzazione dati di volo
  MODE_MENU,    // Navigazione lista impostazioni
  MODE_EDIT     // Modifica di un valore specifico
};

// --- Tipi di Filtro ---
enum FilterType { 
  MEDIA_MOBILE, 
  EMA, 
  KALMAN 
};

// --- Struttura di Configurazione Globale ---
struct DeviceConfig {
  float lift_threshold = 0.3;
  float sink_threshold = -0.3;
  float starting_altitude = 0.0;
  FilterType filter_type = MEDIA_MOBILE;
  bool show_battery_info = true;
  float altitude_offset = 0.0;
  bool show_voltage = false; // true = Volt, false = %
  bool thermal_sniffer = false;
  int volume = 4;
};

// Rendiamo la configurazione accessibile a tutti i file .cpp
extern DeviceConfig config;

#endif