#include "Audio.h"
#include "config.h"
#include <Arduino.h>

// Configurazioni per il PWM dell'ESP32 C3
const int LEDC_CHANNEL = 0;
const int LEDC_RESOLUTION = 8; // Risoluzione 8 bit (0-255)

const int LIFT_CONFIRMATION_COUNT = 5; 
const int SINK_CONFIRMATION_COUNT = 8; 

static int lift_counter = 0; 
static int sink_counter = 0; 
static unsigned long last_event_time = 0; 
static bool is_beeping = false;           

void audio_init() {
  ledcAttach(BUZZER_PIN, 4000, LEDC_RESOLUTION);
}

void audio_play(int frequency) {
  if (frequency <= 0) {
    ledcWrite(BUZZER_PIN, 0);
    return;
  }
  ledcWriteTone(BUZZER_PIN, frequency);
  
  // Mappiamo i livelli 1-8 in un duty cycle da 10 a 127 (50%)
  int duty = map(config.volume, 1, 8, 10, 127);
  ledcWrite(BUZZER_PIN, duty);
}

void audio_stop() {
  ledcWrite(BUZZER_PIN, 0);
}

void audio_beep_feedback() {
  audio_play(1200); // Tono secco
  delay(30);        // Durata minima
  audio_stop();
}

void audio_update(float vario_mps) {
  unsigned long current_time = millis();

  // --- LOGICA THERMAL SNIFFER ---
  if (config.thermal_sniffer && vario_mps > -0.1 && vario_mps <= config.lift_threshold) {
    if (!is_beeping && (current_time - last_event_time > 800)) {
      audio_play(400); // Suono basso per sniffer
      delay(40);       // Bip molto corto
      audio_stop();
      last_event_time = current_time;
    }
    return;
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

    // A così (per un suono più forte e vicino alla risonanza):
    //int frequency = map(vario_mps * 100, config.lift_threshold * 100, 500, 2000, 3800);
    
    if (!is_beeping && (current_time - last_event_time > beep_interval)) {
      audio_play(frequency);
      is_beeping = true;
      last_event_time = current_time;
    } else if (is_beeping && (current_time - last_event_time > 100)) {
      audio_stop();
      is_beeping = false;
      last_event_time = current_time;
    }
  }
  else if (sink_counter >= SINK_CONFIRMATION_COUNT) {
    if (!is_beeping) {
        audio_play(250); 
        is_beeping = true;
    }
  }
  else {
    if (is_beeping) { 
        audio_stop();
        is_beeping = false;
    }
  }
}