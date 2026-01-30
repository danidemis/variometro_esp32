#ifndef SENSOR_H
#define SENSOR_H

#include <MS5611.h> // <-- NUOVA RIGA: Includiamo la libreria del sensore qui

// --- NUOVA RIGA: Dichiariamo l'oggetto ms5611 come 'extern'. ---
// Questo dice al compilatore: "Questo oggetto esiste da qualche parte nel progetto,
// fidati e permetti agli altri file di usarlo".
extern MS5611 ms5611;

// Struttura per contenere i dati letti dal sensore
struct SensorData {
  float temperature;
  float pressure;
  float altitude;          // Altitudine istantanea, "rumorosa"
  float filtered_altitude; // Altitudine filtrata, più stabile
  float vario_mps;         // Velocità verticale in metri al secondo
  float battery_voltage; // <-- AGGIUNTO: Tensione in Volt
  int battery_percent;   // <-- AGGIUNTO: Percentuale 0-100%
};

// Funzioni pubbliche del nostro modulo
bool sensor_init();
//void sensor_read_data(SensorData &data);
void sensor_process_data(SensorData &data, float avg_pressure_hpa);

#endif