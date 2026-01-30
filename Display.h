#ifndef DISPLAY_H
#define DISPLAY_H

#include "Sensor.h" // Includiamo Sensor.h per poter usare la struttura SensorData

void display_init();
void display_update(const SensorData &data, float start_altitude, float relative_gain);

#endif