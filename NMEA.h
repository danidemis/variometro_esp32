#ifndef NMEA_H
#define NMEA_H

#include "Sensor.h" // Per usare la struttura SensorData

// Inizializza la comunicazione Bluetooth Serial
void nmea_init();

// Formatta e invia la frase NMEA
void nmea_send(const SensorData &data);

#endif