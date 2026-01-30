#include "Audio.h"
#include "config.h"
#include <Arduino.h>

const int LIFT_CONFIRMATION_COUNT = 5; 
const int SINK_CONFIRMATION_COUNT = 8; 

static int lift_counter = 0; 
static int sink_counter = 0; 
static unsigned long last_event_time = 0; 
static bool is_beeping = false;           

void audio_init() {
  pinMode(BUZZER_PIN, OUTPUT);
}

void audio_update(float vario_mps) {
  unsigned long current_time = millis();

  // --- LOGICA THERMAL SNIFFER ---
  // Se attivo, suona tra -0.1 e la soglia di salita impostata
  if (config.thermal_sniffer && vario_mps > -0.1 && vario_mps <= config.lift_threshold) {
    if (!is_beeping && (current_time - last_event_time > 800)) {
      tone(BUZZER_PIN, 400, 40); // Bip corto e basso (400Hz)
      last_event_time = current_time;
    }
    return; // Esci per non sovrapporsi al vario normale
  }

  // --- LOGICA VARIO CLASSICA ---
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

  if (lift_counter >= LIFT_CONFIRMATION_COUNT) {
    long beep_interval = map(vario_mps * 100, config.lift_threshold * 100, 500, 600, 80);
    beep_interval = constrain(beep_interval, 80, 600);
    int frequency = map(vario_mps * 100, config.lift_threshold * 100, 500, 800, 1600);
    
    if (!is_beeping && (current_time - last_event_time > beep_interval)) {
      tone(BUZZER_PIN, frequency);
      is_beeping = true;
      last_event_time = current_time;
    } else if (is_beeping && (current_time - last_event_time > 100)) {
      noTone(BUZZER_PIN);
      is_beeping = false;
      last_event_time = current_time;
    }
  }
  else if (sink_counter >= SINK_CONFIRMATION_COUNT) {
    if (!is_beeping) {
        tone(BUZZER_PIN, 250); 
        is_beeping = true;
    }
  }
  else {
    if (is_beeping) { 
        noTone(BUZZER_PIN);
        is_beeping = false;
    }
  }
}