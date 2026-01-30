#include "Storage.h"

Preferences preferences;

void storage_load_history(FlightRecord history[]) {
  preferences.begin("logbook", true);
  size_t res = preferences.getBytes("flights", history, sizeof(FlightRecord) * 10);
  if (res == 0) { // Se non ci sono dati, inizializza a zero
    memset(history, 0, sizeof(FlightRecord) * 10);
  }
  preferences.end();
}

void storage_save_flight(const FlightRecord &newFlight) {
  FlightRecord history[10];
  memset(history, 0, sizeof(FlightRecord) * 10); // Pulizia iniziale
  
  preferences.begin("logbook", false);
  preferences.getBytes("flights", history, sizeof(FlightRecord) * 10);
  
  for (int i = 9; i > 0; i--) {
    history[i] = history[i - 1];
  }
  
  history[0] = newFlight;
  history[0].valid = true;
  
  preferences.putBytes("flights", history, sizeof(FlightRecord) * 10);
  preferences.end();
}

void storage_get_flight(int index, FlightRecord &record) {
  FlightRecord history[10];
  memset(history, 0, sizeof(FlightRecord) * 10);
  
  preferences.begin("logbook", true);
  preferences.getBytes("flights", history, sizeof(FlightRecord) * 10);
  if (index >= 0 && index < 10) {
    record = history[index];
  }
  preferences.end();
}