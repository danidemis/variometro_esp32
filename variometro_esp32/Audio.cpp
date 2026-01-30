#include "Audio.h"
#include "config.h"
#include <Arduino.h>

// Il vario deve superare la soglia per questo numero di letture consecutive prima di suonare.
const int LIFT_CONFIRMATION_COUNT = 5; 
const int SINK_CONFIRMATION_COUNT = 5; 

static int lift_counter = 0; 
static int sink_counter = 0; 

// Variabili per il timer non-bloccante del bip di salita
const int BEEP_DURATION_MS = 80;   
static unsigned long last_event_time = 0; 
static bool is_beeping = false;           

void audio_init() {
  pinMode(BUZZER_PIN, OUTPUT);
}

void audio_update(float vario_mps) {
  
  // Logica di conferma basata sulla configurazione dinamica
  if (vario_mps > config.lift_threshold) {
    lift_counter++;
    sink_counter = 0;
  } else if (vario_mps < config.sink_threshold) {
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
    // Usiamo config.lift_threshold per calcolare l'intervallo e la frequenza
    long beep_interval = map(vario_mps * 100, config.lift_threshold * 100, 500, 600, 80);
    beep_interval = constrain(beep_interval, 80, 600);

    int frequency = map(vario_mps * 100, config.lift_threshold * 100, 500, 800, 1500);
    frequency = constrain(frequency, 800, 1500);

    if (!is_beeping && (current_time - last_event_time > beep_interval)) {
      tone(BUZZER_PIN, frequency);
      is_beeping = true;
      last_event_time = current_time;
    } else if (is_beeping && (current_time - last_event_time > BEEP_DURATION_MS)) {
      noTone(BUZZER_PIN);
      is_beeping = false;
      last_event_time = current_time;
    }
  }
  else if (sink_counter >= SINK_CONFIRMATION_COUNT) {
    // --- Logica per il suono di DISCESA ---
    if (!is_beeping) {
        tone(BUZZER_PIN, 300); 
        is_beeping = true;
    }
  }
  else {
    // --- Silenzio ---
    if (is_beeping) { 
        noTone(BUZZER_PIN);
        is_beeping = false;
    }
  }
}