#include "Storage.h"

Preferences preferences;

void storage_load_history(FlightRecord history[]) {
  preferences.begin("logbook", true); // Modalità sola lettura
  // Leggiamo l'intero array di 10 record come blocco di dati binari
  preferences.getBytes("flights", history, sizeof(FlightRecord) * 10);
  preferences.end();
}

void storage_save_flight(const FlightRecord &newFlight) {
  FlightRecord history[10];
  
  // 1. Carichiamo i voli esistenti
  preferences.begin("logbook", false); // Modalità scrittura
  preferences.getBytes("flights", history, sizeof(FlightRecord) * 10);
  
  // 2. Facciamo scorrere i voli vecchi verso il basso (FIFO)
  for (int i = 9; i > 0; i--) {
    history[i] = history[i - 1];
  }
  
  // 3. Inseriamo il nuovo volo nella posizione 0
  history[0] = newFlight;
  history[0].valid = true;
  
  // 4. Salviamo l'intero array aggiornato
  preferences.putBytes("flights", history, sizeof(FlightRecord) * 10);
  preferences.end();
}

void storage_clear_all() {
  preferences.begin("logbook", false);
  preferences.clear();
  preferences.end();
}