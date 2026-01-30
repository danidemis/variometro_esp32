#ifndef STORAGE_H
#define STORAGE_H

#include "config.h"
#include <Preferences.h>

// Carica i 10 voli salvati nella memoria Flash
void storage_load_history(FlightRecord history[]);

// Salva un nuovo volo in cima alla lista (logica FIFO)
void storage_save_flight(const FlightRecord &newFlight);

// Resetta tutta la memoria (opzionale)
void storage_clear_all();

void storage_get_flight(int index, FlightRecord &record);

#endif