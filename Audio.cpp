#include "Audio.h"
#include "config.h"
#include <Arduino.h>

// Il vario deve superare la soglia per questo numero di letture consecutive prima di suonare.
const int LIFT_CONFIRMATION_COUNT = 5; // 3 letture = 300ms
const int SINK_CONFIRMATION_COUNT = 5; // Per la discesa possiamo essere più rapidi

static int lift_counter = 0; // Contatore per la salita
static int sink_counter = 0; // Contatore per la 

// Variabili per il timer non-bloccante del bip di salita
const int BEEP_DURATION_MS = 80;   // Durata di ogni singolo bip
static unsigned long last_event_time = 0; // Memorizza l'ultimo cambio di stato (inizio bip o fine bip)
static bool is_beeping = false;           // Flag per sapere se stiamo emettendo un suono

void audio_init() {
  pinMode(BUZZER_PIN, OUTPUT);
}

void audio_update(float vario_mps) {
  
  // Logica di conferma (invariata)
  if (vario_mps > LIFT_THRESHOLD_MPS) {
    lift_counter++;
    sink_counter = 0;
  } else if (vario_mps < SINK_THRESHOLD_MPS) {
    sink_counter++;
    lift_counter = 0;
  } else {
    lift_counter = 0;
    sink_counter = 0;
  }

  // --- Decisione Finale: Suonare o No? ---

  unsigned long current_time = millis();

  if (lift_counter >= LIFT_CONFIRMATION_COUNT) {
    // --- Logica per il suono di ASCESA ---
    // Calcoliamo l'intervallo tra i bip. Più forte la salita, più corto l'intervallo.
    // La funzione map() è perfetta per questo.
    // Mappiamo una salita da 0.3m/s a 5m/s in un intervallo tra 600ms e 80ms.
    long beep_interval = map(vario_mps * 100, LIFT_THRESHOLD_MPS * 100, 500, 600, 80);
    // Ci assicuriamo che l'intervallo non sia mai troppo piccolo
    beep_interval = constrain(beep_interval, 80, 600);

    // Calcoliamo anche un tono più acuto per le salite più forti
    int frequency = map(vario_mps * 100, LIFT_THRESHOLD_MPS * 100, 500, 800, 1500);
    frequency = constrain(frequency, 800, 1500);

    if (!is_beeping && (current_time - last_event_time > beep_interval)) {
      // È ora di iniziare un nuovo bip
      tone(BUZZER_PIN, frequency);
      is_beeping = true;
      last_event_time = current_time;
    } else if (is_beeping && (current_time - last_event_time > BEEP_DURATION_MS)) {
      // È ora di terminare il bip corrente
      noTone(BUZZER_PIN);
      is_beeping = false;
      last_event_time = current_time;
    }
  }
  else if (sink_counter >= SINK_CONFIRMATION_COUNT) {
    // --- Logica per il suono di DISCESA ---
    // Tono grave e continuo
    if (!is_beeping) {
        tone(BUZZER_PIN, 300); // Frequenza bassa per la discesa
        is_beeping = true;
    }
  }
  else {
    // --- Silenzio ---
    if (is_beeping) { // Spegni il buzzer solo se era acceso
        noTone(BUZZER_PIN);
        is_beeping = false;
    }
    // Resettiamo i contatori se non siamo in una condizione confermata
    //lift_counter = 0;
    //sink_counter = 0;
  }
}