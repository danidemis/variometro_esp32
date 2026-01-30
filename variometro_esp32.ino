#include "config.h"
#include "Sensor.h"
#include "Audio.h"
#include "Display.h"
#include "NMEA.h"

// Istanza globale della configurazione
DeviceConfig config;

// Stato attuale del sistema
DeviceMode currentMode = MODE_FLIGHT;
int selectedMenuItem = 0;
const int TOTAL_MENU_ITEMS = 5;

// Variabili di volo
bool first_reading_done = false;
float current_gain = 0.0;

// Variabili per il Burst Averaging
const int UPDATE_INTERVAL_MS = 100; // 10 Hz
static unsigned long next_update_time = 0;
static float pressure_accumulator = 0;
static int reading_count = 0;
SensorData current_data;

// Gestione Input (Debouncing)
unsigned long last_input_time = 0;
const int INPUT_DELAY = 200; // ms

void handleInputs() {
  if (millis() - last_input_time < INPUT_DELAY) return;

  bool up = digitalRead(PIN_UP) == LOW;
  bool down = digitalRead(PIN_DOWN) == LOW;
  bool select = digitalRead(PIN_SELECT) == LOW;

  if (up || down || select) {
    last_input_time = millis();

    if (currentMode == MODE_FLIGHT) {
      if (select) {
        currentMode = MODE_MENU;
        selectedMenuItem = 0;
        Serial.println("Entrato in MENU");
      }
    } 
    else if (currentMode == MODE_MENU) {
      if (up) {
        selectedMenuItem--;
        if (selectedMenuItem < 0) selectedMenuItem = TOTAL_MENU_ITEMS - 1;
      } else if (down) {
        selectedMenuItem++;
        if (selectedMenuItem >= TOTAL_MENU_ITEMS) selectedMenuItem = 0;
      } else if (select) {
        if (selectedMenuItem == 0) { // Esempio: Torna al volo
            currentMode = MODE_FLIGHT;
            Serial.println("Tornato in VOLO");
        } else {
            currentMode = MODE_EDIT;
            Serial.println("Entrato in EDIT");
        }
      }
    }
    else if (currentMode == MODE_EDIT) {
      if (select) {
        currentMode = MODE_MENU; // Salva ed esce (logica da espandere)
        Serial.println("Uscito da EDIT");
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Avvio Variometro ESP32...");

  // Inizializzazione Hardware
  sensor_init();
  audio_init();
  display_init();
  nmea_init();

  // Configurazione Pin Levetta
  pinMode(PIN_UP, INPUT_PULLUP);
  pinMode(PIN_DOWN, INPUT_PULLUP);
  pinMode(PIN_SELECT, INPUT_PULLUP);

  next_update_time = millis() + UPDATE_INTERVAL_MS;
}

void loop() {
  // Gestione costante degli input della levetta
  handleInputs();

  // Lettura sensore in background (indipendente dalla modalità display)
  if (ms5611.read(0) == 0) {
      pressure_accumulator += ms5611.getPressure();
      reading_count++;
  }

  // Elaborazione e aggiornamento display a 10Hz
  if (millis() >= next_update_time) {
    if (reading_count > 0) {
      float average_pressure = pressure_accumulator / reading_count;
      sensor_process_data(current_data, average_pressure);
      
      if (!first_reading_done) {
        config.starting_altitude = current_data.filtered_altitude;
        first_reading_done = true;
      }
      current_gain = current_data.filtered_altitude - config.starting_altitude;

      // Aggiornamento periferiche
      audio_update(current_data.vario_mps);
      nmea_send(current_data);

      // Gestione del display in base alla modalità
      if (currentMode == MODE_FLIGHT) {
        display_update(current_data, config.starting_altitude, current_gain);
      } else {
        // Qui chiameremo la funzione di disegno del menù (Step 3)
        // Per ora facciamo una stampa seriale di debug
      }
    }
    
    pressure_accumulator = 0;
    reading_count = 0;
    next_update_time += UPDATE_INTERVAL_MS;
  }
}