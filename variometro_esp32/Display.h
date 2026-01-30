#ifndef DISPLAY_H
#define DISPLAY_H

#include "Sensor.h" 
#include "config.h"

void display_init();
void display_update(const SensorData &data, float start_altitude, float relative_gain);
void display_menu(int selectedIndex);
void display_edit(int selectedIndex, int subIndex); // Prototipo per Step 4

#endif