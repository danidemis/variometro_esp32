#include "config.h"
#include "Sensor.h"
#include "Audio.h"
#include "Display.h"
#include "NMEA.h"
#include "Storage.h"

DeviceConfig config;
DeviceMode currentMode = MODE_FLIGHT;

// Sessione
FlightRecord currentSession;
FlightRecord currentHistoryView;
bool isSessionActive = false;
unsigned long sessionStartTime = 0;

// Menu
int selectedMenuItem = 0;
int editSubIndex = 0; 
int historyIndex = 0;
const int TOTAL_MENU_ITEMS = 8; 

// Dati
bool first_reading_done = false;
float current_gain = 0.0;
const int UPDATE_INTERVAL_MS = 100; // 10 Hz
static unsigned long next_update_time = 0;
static float pressure_accumulator = 0;
static int reading_count = 0;
SensorData current_data;

bool lastUpState = HIGH, lastDownState = HIGH, lastSelectState = HIGH;

// --- FUNZIONI DI SUPPORTO ---
void updateHistoryView() {
    storage_get_flight(historyIndex, currentHistoryView);
}

void startNewSession() {
    if (isnan(current_data.filtered_altitude)) return;
    currentSession.max_alt = current_data.filtered_altitude;
    currentSession.min_alt = current_data.filtered_altitude;
    currentSession.max_climb = 0;
    currentSession.max_sink = 0;
    currentSession.used_filter = config.filter_type;
    currentSession.duration_secs = 0;
    currentSession.valid = true;
    sessionStartTime = millis(); 
    isSessionActive = true;
    audio_beep_feedback();
}

void saveCurrentSession() {
    if (isSessionActive) {
        currentSession.duration_secs = (millis() - sessionStartTime) / 1000;
        storage_save_flight(currentSession);
        isSessionActive = false;
        audio_beep_feedback(); delay(100); audio_beep_feedback();
    }
}

void calibrate_altitude() {
    if (isnan(current_data.altitude)) return;
    float raw_sensor_altitude = current_data.altitude - config.altitude_offset;
    config.altitude_offset = config.starting_altitude - raw_sensor_altitude;
}

void updateParameter(bool increment) {
  float delta = increment ? 0.1 : -0.1;
  switch (selectedMenuItem) {
    case 1: { 
      int factors[] = {1000, 100, 10, 1};
      int val = (int)config.starting_altitude;
      int digit = (val / factors[editSubIndex]) % 10;
      int newDigit = increment ? (digit + 1) % 10 : (digit - 1 + 10) % 10;
      config.starting_altitude += (newDigit - digit) * factors[editSubIndex];
      break;
    }
    case 2: config.lift_threshold = constrain(config.lift_threshold + delta, 0.1, 2.0); config.sink_threshold = -config.lift_threshold - 0.2; break;
    case 3: config.filter_type = (FilterType)((config.filter_type + (increment ? 1 : 2)) % 3); break;
    case 4: if (editSubIndex == 0) config.show_battery_info = !config.show_battery_info; else config.show_voltage = !config.show_voltage; break;
    case 5: config.thermal_sniffer = !config.thermal_sniffer; break;
  }
}

// --- GESTIONE INPUT (PURA LOGICA, NESSUN DISEGNO) ---
void handleInputs() {
  bool currUp = digitalRead(PIN_UP);
  bool currDown = digitalRead(PIN_DOWN);
  bool currSelect = digitalRead(PIN_SELECT);

  if (lastSelectState == LOW && currSelect == HIGH) {
    if (currentMode == MODE_FLIGHT) { currentMode = MODE_MENU; selectedMenuItem = 0; } 
    else if (currentMode == MODE_MENU) {
      if (selectedMenuItem == 0) currentMode = MODE_FLIGHT;
      else if (selectedMenuItem == 6) { if (!isSessionActive) startNewSession(); else saveCurrentSession(); currentMode = MODE_FLIGHT; }
      else if (selectedMenuItem == 7) { currentMode = MODE_HISTORY; historyIndex = 0; updateHistoryView(); }
      else { currentMode = MODE_EDIT; editSubIndex = 0; }
    }
    else if (currentMode == MODE_EDIT) {
      if (selectedMenuItem == 1) { editSubIndex++; if (editSubIndex > 3) { calibrate_altitude(); currentMode = MODE_CONFIRM_START; } }
      else { currentMode = MODE_MENU; }
    }
    else if (currentMode == MODE_CONFIRM_START) { startNewSession(); currentMode = MODE_FLIGHT; }
    else if (currentMode == MODE_HISTORY) currentMode = MODE_MENU;
  }

  if (lastUpState == LOW && currUp == HIGH) {
    if (currentMode == MODE_FLIGHT) { if (config.volume < 8) { config.volume++; audio_beep_feedback(); display_show_volume(); } }
    else if (currentMode == MODE_MENU) selectedMenuItem = (selectedMenuItem - 1 + TOTAL_MENU_ITEMS) % TOTAL_MENU_ITEMS;
    else if (currentMode == MODE_EDIT) updateParameter(true);
    else if (currentMode == MODE_HISTORY) { historyIndex = (historyIndex - 1 + 10) % 10; updateHistoryView(); }
    else if (currentMode == MODE_CONFIRM_START) currentMode = MODE_MENU;
  }

  if (lastDownState == LOW && currDown == HIGH) {
    if (currentMode == MODE_FLIGHT) { if (config.volume > 1) { config.volume--; audio_beep_feedback(); display_show_volume(); } }
    else if (currentMode == MODE_MENU) selectedMenuItem = (selectedMenuItem + 1) % TOTAL_MENU_ITEMS;
    else if (currentMode == MODE_EDIT) updateParameter(false);
    else if (currentMode == MODE_HISTORY) { historyIndex = (historyIndex + 1) % 10; updateHistoryView(); }
    else if (currentMode == MODE_CONFIRM_START) currentMode = MODE_MENU;
  }

  lastUpState = currUp; lastDownState = currDown; lastSelectState = currSelect;
}

void setup() {
  Serial.begin(115200);
  current_data.altitude = 0; current_data.filtered_altitude = 0; current_data.vario_mps = 0;
  sensor_init(); audio_init(); display_init(); nmea_init();
  pinMode(PIN_UP, INPUT_PULLUP); pinMode(PIN_DOWN, INPUT_PULLUP); pinMode(PIN_SELECT, INPUT_PULLUP);
  next_update_time = millis() + UPDATE_INTERVAL_MS;
}

void loop() {
  // 1. Input e Sensori (VELOCE)
  handleInputs();
  if (ms5611.read(0) == 0) { pressure_accumulator += ms5611.getPressure(); reading_count++; }

  // 2. Logica Principale a 10Hz (MEDIO)
  if (millis() >= next_update_time) {
    if (reading_count > 0) {
      float avg_p = pressure_accumulator / reading_count;
      sensor_process_data(current_data, avg_p);

      if (isnan(current_data.filtered_altitude)) { 
          pressure_accumulator = 0; reading_count = 0; next_update_time += UPDATE_INTERVAL_MS; return; 
      }

      if (!first_reading_done) { config.starting_altitude = current_data.filtered_altitude; calibrate_altitude(); first_reading_done = true; }
      current_gain = current_data.filtered_altitude - config.starting_altitude;

      if (isSessionActive) {
          if (current_data.filtered_altitude > currentSession.max_alt) currentSession.max_alt = current_data.filtered_altitude;
          if (current_data.filtered_altitude < currentSession.min_alt) currentSession.min_alt = current_data.filtered_altitude;
          if (current_data.vario_mps > currentSession.max_climb) currentSession.max_climb = current_data.vario_mps;
          if (current_data.vario_mps < currentSession.max_sink) currentSession.max_sink = current_data.vario_mps;
      }

      audio_update(current_data.vario_mps);
      nmea_send(current_data);

      // --- 3. GESTIONE DISPLAY CENTRALIZZATA (LENTO) ---
      // Questa è la parte cruciale per evitare artefatti e blocchi.
      // Il display viene aggiornato una sola volta per ciclo.
      
      static DeviceMode lastModeProcessed = MODE_FLIGHT; // Ricorda l'ultima modalità disegnata

      // Se la modalità è cambiata rispetto all'ultimo disegno, pulisci lo schermo
      if (currentMode != lastModeProcessed) {
          display.clearDisplay();
          lastModeProcessed = currentMode;
      }

      // Disegna il contenuto della modalità attuale
      switch(currentMode) {
        case MODE_FLIGHT: display_update(current_data, config.starting_altitude, current_gain); break;
        case MODE_MENU: display_menu(selectedMenuItem); break;
        case MODE_EDIT: display_edit(selectedMenuItem, editSubIndex); break;
        case MODE_CONFIRM_START: display_confirm_start(); break;
        case MODE_HISTORY: display_history(historyIndex, currentHistoryView); break;
      }
      // NOTA: display.display() è chiamato dentro le singole funzioni (es. display_menu), 
      // quindi qui non serve chiamarlo di nuovo. Questo va bene.
    }
    pressure_accumulator = 0; reading_count = 0;
    next_update_time += UPDATE_INTERVAL_MS;
  }
}