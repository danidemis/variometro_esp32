#include "Sensor.h"
#include "config.h"
#include <Wire.h>
#include <MS5611.h>

MS5611 ms5611(MS5611_I2C_ADDRESS);

// --- Variabili Filtri ---
#define FILTER_SIZE 22
static float altitude_buffer[FILTER_SIZE];
static int buffer_index = 0;
static float altitude_sum = 0;

// EMA
static float ema_altitude = 0;
const float EMA_ALPHA = 0.15;

// Kalman Semplificato
static float kalman_altitude = 0;
static float kalman_p = 1.0;
static float kalman_q = 0.1; // Rumore di processo
static float kalman_r = 0.5; // Rumore di misura
static float kalman_k = 0;

static float last_filtered_altitude = 0;
static unsigned long last_measurement_time = 0;

const float SEALEVELPRESSURE_HPA = 1013.25;

int calculate_battery_percentage(float voltage) {
  if (voltage >= 4.2) return 100;
  if (voltage <= 3.4) return 0;
  return (int)((voltage - 3.4) * 125); 
}

void sensor_read_battery(SensorData &data) {
  uint32_t mv = analogReadMilliVolts(PIN_BATT_ADC);
  data.battery_voltage = (mv * BATT_DIVIDER_RATIO) / 1000.0;
  data.battery_percent = calculate_battery_percentage(data.battery_voltage);
}

float calculate_altitude(float pressure_hpa) {
  return 44330.0 * (1.0 - pow(pressure_hpa / SEALEVELPRESSURE_HPA, 0.1903));
}

bool sensor_init() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  ms5611.begin();
  if (ms5611.isConnected()) {
    ms5611.reset();
    delay(100);
    int result = ms5611.read(0);
    if (result == 0) {
      float initial = calculate_altitude(ms5611.getPressure());
      for (int i = 0; i < FILTER_SIZE; i++) altitude_buffer[i] = initial;
      altitude_sum = initial * FILTER_SIZE;
      ema_altitude = initial;
      kalman_altitude = initial;
      last_filtered_altitude = initial;
      last_measurement_time = millis();
    }
    return true;
  }
  return false;
}

void sensor_process_data(SensorData &data, float avg_pressure_hpa) {
  data.pressure = avg_pressure_hpa;
  data.temperature = ms5611.getTemperature();
  
  // Applichiamo l'offset di calibrazione impostato manualmente
  float raw_alt = calculate_altitude(data.pressure) + config.altitude_offset;
  data.altitude = raw_alt;

  // 1. Algoritmo MEDIA MOBILE
  altitude_sum -= altitude_buffer[buffer_index];
  altitude_buffer[buffer_index] = raw_alt;
  altitude_sum += raw_alt;
  buffer_index = (buffer_index + 1) % FILTER_SIZE;
  float media_alt = altitude_sum / FILTER_SIZE;

  // 2. Algoritmo EMA
  ema_altitude = (EMA_ALPHA * raw_alt) + ((1.0 - EMA_ALPHA) * ema_altitude);

  // 3. Algoritmo KALMAN
  kalman_p = kalman_p + kalman_q;
  kalman_k = kalman_p / (kalman_p + kalman_r);
  kalman_altitude = kalman_altitude + kalman_k * (raw_alt - kalman_altitude);
  kalman_p = (1 - kalman_k) * kalman_p;

  // Selezione dell'algoritmo basata sul menu
  if (config.filter_type == MEDIA_MOBILE) data.filtered_altitude = media_alt;
  else if (config.filter_type == EMA) data.filtered_altitude = ema_altitude;
  else data.filtered_altitude = kalman_altitude;

  unsigned long current_time = millis();
  long delta_time_ms = current_time - last_measurement_time;
  if (delta_time_ms > 0) {
    float delta_altitude = data.filtered_altitude - last_filtered_altitude;
    data.vario_mps = (delta_altitude * 1000.0) / delta_time_ms;
  }

  sensor_read_battery(data);
  last_filtered_altitude = data.filtered_altitude;
  last_measurement_time = current_time;
}

void sensor_reset_filter(float new_altitude) {
    // 1. Reset MEDIA MOBILE
    // Riempiamo tutto il buffer con la nuova quota per azzerare la media
    for (int i = 0; i < FILTER_SIZE; i++) {
        altitude_buffer[i] = new_altitude;
    }
    // Ricalcoliamo la somma totale del buffer
    altitude_sum = new_altitude * FILTER_SIZE;
    buffer_index = 0;

    // 2. Reset EMA
    ema_altitude = new_altitude;

    // 3. Reset KALMAN
    kalman_altitude = new_altitude;
    kalman_p = 1.0; // Resettiamo l'incertezza per far sì che il filtro si riagganci subito

    // 4. FONDAMENTALE: Reset riferimenti velocità
    // Dobbiamo aggiornare last_filtered_altitude, altrimenti al prossimo ciclo
    // il calcolo del vario (data.filtered_altitude - last_filtered_altitude)
    // vedrebbe comunque il "salto" rispetto alla lettura precedente.
    last_filtered_altitude = new_altitude;

    Serial.print("Filtri resettati e sincronizzati a: "); 
    Serial.print(new_altitude); 
    Serial.println(" m");
}