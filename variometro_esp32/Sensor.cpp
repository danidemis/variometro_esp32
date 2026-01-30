#include "Sensor.h"
#include "config.h"
#include <Wire.h>
#include <MS5611.h>

// # MODIFICA 1: Passiamo l'indirizzo I2C qui, quando l'oggetto viene creato.
MS5611 ms5611(MS5611_I2C_ADDRESS);

// Dimensione del buffer per la media mobile. Un valore più alto = più stabilità, meno reattività.
//#define FILTER_SIZE 22
#define FILTER_SIZE 22



static float altitude_buffer[FILTER_SIZE]; // Array per memorizzare le ultime N letture
static int buffer_index = 0;               // Indice per la posizione corrente nel buffer
static float altitude_sum = 0;             // Somma dei valori nel buffer per un calcolo efficiente della media

static float last_filtered_altitude = 0;   // Per calcolare la differenza di quota
static unsigned long last_measurement_time = 0; // Per calcolare la differenza di tempo

// Pressione a livello del mare standard (hPa).
const float SEALEVELPRESSURE_HPA = 1013.25;

// Funzione per mappare il voltaggio in percentuale (approssimazione curva LiPo)
int calculate_battery_percentage(float voltage) {
  if (voltage >= 4.2) return 100;
  if (voltage <= 3.4) return 0;
  // Mappa lineare semplice tra 3.4V (vuota) e 4.2V (piena)
  return (int)((voltage - 3.4) * 125); 
}

void sensor_read_battery(SensorData &data) {
  // Legge il valore calibrato in millivolt direttamente dal pin
  uint32_t mv = analogReadMilliVolts(PIN_BATT_ADC);
  
  // Moltiplica per il rapporto del partitore e converte in Volt
  data.battery_voltage = (mv * BATT_DIVIDER_RATIO) / 1000.0;
  
  // Calcola la percentuale
  data.battery_percent = calculate_battery_percentage(data.battery_voltage);
}

// Funzione per calcolare l'altitudine dalla pressione
float calculate_altitude(float pressure_hpa) {
  return 44330.0 * (1.0 - pow(pressure_hpa / SEALEVELPRESSURE_HPA, 0.1903));
}

bool sensor_init() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  ms5611.begin();
  
  if (ms5611.isConnected()) {
    ms5611.reset();

    // "Riempiamo" il filtro con la prima lettura valida per evitare valori strani all'inizio
    int result = ms5611.read(0);
    if (result == 0) {
      float initial_altitude = calculate_altitude(ms5611.getPressure());
      for (int i = 0; i < FILTER_SIZE; i++) {
        altitude_buffer[i] = initial_altitude;
      }
      altitude_sum = initial_altitude * FILTER_SIZE;
      last_filtered_altitude = initial_altitude;
      last_measurement_time = millis();
    }
    return true;
  }
  return false;
}

void sensor_process_data(SensorData &data, float avg_pressure_hpa) {
  // 1. Usa la pressione media e leggi solo la temperatura
  data.pressure = avg_pressure_hpa;
  data.temperature = ms5611.getTemperature(); // La temperatura non necessita di media spinta
  data.altitude = calculate_altitude(data.pressure);

  // 2. Aggiorna il filtro a media mobile (logica invariata)
  altitude_sum -= altitude_buffer[buffer_index];
  altitude_buffer[buffer_index] = data.altitude;
  altitude_sum += data.altitude;
  buffer_index = (buffer_index + 1) % FILTER_SIZE;
  data.filtered_altitude = altitude_sum / FILTER_SIZE;

  // 3. Calcola la velocità verticale (logica invariata)
  unsigned long current_time = millis();
  long delta_time_ms = current_time - last_measurement_time;
  if (delta_time_ms > 0) {
    float delta_altitude = data.filtered_altitude - last_filtered_altitude;
    data.vario_mps = (delta_altitude * 1000.0) / delta_time_ms;
  }


  sensor_read_battery(data);
  // 4. Aggiorna le variabili per il prossimo ciclo (logica invariata)
  last_filtered_altitude = data.filtered_altitude;
  last_measurement_time = current_time;
}


// void sensor_read_data(SensorData &data) {
//   int result = ms5611.read(0); 
  
//   if (result == 0) {
//     // 1. Ottieni i dati grezzi
//     data.temperature = ms5611.getTemperature();
//     data.pressure = ms5611.getPressure();
//     data.altitude = calculate_altitude(data.pressure);

//     // 2. Aggiorna il filtro a media mobile
//     altitude_sum -= altitude_buffer[buffer_index]; // Sottrai il valore più vecchio
//     altitude_buffer[buffer_index] = data.altitude; // Inserisci il valore nuovo
//     altitude_sum += data.altitude;                 // Aggiungi il valore nuovo alla somma
//     buffer_index = (buffer_index + 1) % FILTER_SIZE; // Avanza l'indice in modo circolare

//     data.filtered_altitude = altitude_sum / FILTER_SIZE; // Calcola la nuova media

//     // 3. Calcola la velocità verticale
//     unsigned long current_time = millis();
//     long delta_time_ms = current_time - last_measurement_time;

//     // Esegui il calcolo solo se è passato un po' di tempo per evitare divisioni per zero
//     if (delta_time_ms > 0) {
//       float delta_altitude = data.filtered_altitude - last_filtered_altitude;
//       data.vario_mps = (delta_altitude * 1000.0) / delta_time_ms; // Calcola m/s
//     }

//     // 4. Aggiorna le variabili statiche per il prossimo ciclo
//     last_filtered_altitude = data.filtered_altitude;
//     last_measurement_time = current_time;
//   }
// }