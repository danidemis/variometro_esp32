#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_SSD1306.h>
#include "Sensor.h" 
#include "config.h"

extern Adafruit_SSD1306 display;

void display_init();
void display_update(const SensorData &data, float start_altitude, float relative_gain);
void display_menu(int selectedIndex);
void display_edit(int selectedIndex, int subIndex); // Prototipo per Step 4
void display_show_volume(); // <-- AGGIUNTO: Risolve l'errore di compilazione
void display_confirm_start();
void display_history(int flightIndex, const FlightRecord &record); // <-- Per lo Step 5

#endif