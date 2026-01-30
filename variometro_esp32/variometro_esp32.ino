#include "config.h"
#include "Sensor.h"
#include "Audio.h"
#include "Display.h"
#include "NMEA.h"

DeviceConfig config;
DeviceMode currentMode = MODE_FLIGHT;
int selectedMenuItem = 0;
int editSubIndex = 0; // Per gestire le 4 cifre dell'altitudine o sottomenu
const int TOTAL_MENU_ITEMS = 6;

bool first_reading_done = false;
float current_gain = 0.0;
const int UPDATE_INTERVAL_MS = 100; 
static unsigned long next_update_time = 0;
static float pressure_accumulator = 0;
static int reading_count = 0;
SensorData current_data;

// Variabili per la logica "On-Release"
bool lastUpState = HIGH;
bool lastDownState = HIGH;
bool lastSelectState = HIGH;

void handleInputs() {
  // Leggi gli stati attuali
  bool currUp = digitalRead(PIN_UP);
  bool currDown = digitalRead(PIN_DOWN);
  bool currSelect = digitalRead(PIN_SELECT);

  // --- LOGICA CLICK (Rileva il rilascio: era LOW, ora è HIGH) ---
  if (lastSelectState == LOW && currSelect == HIGH) {
    if (currentMode == MODE_FLIGHT) {
      currentMode = MODE_MENU;
      selectedMenuItem = 0;
    } else if (currentMode == MODE_MENU) {
      if (selectedMenuItem == 0) currentMode = MODE_FLIGHT; // ESCI
      else {
        currentMode = MODE_EDIT;
        editSubIndex = 0; // Inizia dalla prima cifra o opzione
      }
    } else if (currentMode == MODE_EDIT) {
      // Gestione specifica per l'altitudine (4 cifre)
      if (selectedMenuItem == 1) { 
        editSubIndex++;
        if (editSubIndex > 3) currentMode = MODE_MENU; // Fine modifica cifre
      } else {
        currentMode = MODE_MENU; // Per gli altri parametri esce subito
      }
    }
  }

  // --- LOGICA SU/GIÙ (Sempre al rilascio per precisione) ---
  if (lastUpState == LOW && currUp == HIGH) {
    if (currentMode == MODE_MENU) {
      selectedMenuItem = (selectedMenuItem - 1 + TOTAL_MENU_ITEMS) % TOTAL_MENU_ITEMS;
    } else if (currentMode == MODE_EDIT) {
      updateParameter(true);
    }
  }

  if (lastDownState == LOW && currDown == HIGH) {
    if (currentMode == MODE_MENU) {
      selectedMenuItem = (selectedMenuItem + 1) % TOTAL_MENU_ITEMS;
    } else if (currentMode == MODE_EDIT) {
      updateParameter(false);
    }
  }

  // Salva gli stati per il prossimo ciclo
  lastUpState = currUp;
  lastDownState = currDown;
  lastSelectState = currSelect;
}

// Funzione helper per modificare i valori nella struttura config
void updateParameter(bool increment) {
  float delta = increment ? 0.1 : -0.1;
  
  switch (selectedMenuItem) {
    case 1: { // ALTIMETRIA (Modifica 4 cifre)
      int factors[] = {1000, 100, 10, 1};
      int val = (int)config.starting_altitude;
      int digit = (val / factors[editSubIndex]) % 10;
      int newDigit = increment ? (digit + 1) % 10 : (digit - 1 + 10) % 10;
      config.starting_altitude += (newDigit - digit) * factors[editSubIndex];
      break;
    }
    case 2: // SENSIBILITÀ
      config.lift_threshold = constrain(config.lift_threshold + delta, 0.1, 2.0);
      config.sink_threshold = -config.lift_threshold; // Manteniamo simmetria
      break;
    case 3: // FILTRO
      config.filter_type = (FilterType)((config.filter_type + (increment ? 1 : 2)) % 3);
      break;
    case 4: // BATTERIA (Toggle info / Toggle Volt-%)
      if (editSubIndex == 0) config.show_battery_info = !config.show_battery_info;
      else config.show_voltage = !config.show_voltage;
      break;
    case 5: // SNIFFER
      config.thermal_sniffer = !config.thermal_sniffer;
      break;
  }
}

void setup() {
  Serial.begin(115200);
  sensor_init();
  audio_init();
  display_init();
  nmea_init();

  pinMode(PIN_UP, INPUT_PULLUP);
  pinMode(PIN_DOWN, INPUT_PULLUP);
  pinMode(PIN_SELECT, INPUT_PULLUP);

  next_update_time = millis() + UPDATE_INTERVAL_MS;
}

void loop() {
  handleInputs();

  if (ms5611.read(0) == 0) {
      pressure_accumulator += ms5611.getPressure();
      reading_count++;
  }

  if (millis() >= next_update_time) {
    if (reading_count > 0) {
      float average_pressure = pressure_accumulator / reading_count;
      sensor_process_data(current_data, average_pressure);
      
      if (!first_reading_done) {
        config.starting_altitude = current_data.filtered_altitude;
        first_reading_done = true;
      }
      current_gain = current_data.filtered_altitude - config.starting_altitude;

      audio_update(current_data.vario_mps);
      nmea_send(current_data);

      if (currentMode == MODE_FLIGHT) {
        display_update(current_data, config.starting_altitude, current_gain);
      } else if (currentMode == MODE_MENU) {
        display_menu(selectedMenuItem);
      } else if (currentMode == MODE_EDIT) {
        display_edit(selectedMenuItem, editSubIndex);
      }
    }
    pressure_accumulator = 0;
    reading_count = 0;
    next_update_time += UPDATE_INTERVAL_MS;
  }
}