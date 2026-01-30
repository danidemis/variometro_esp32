#include "config.h"
#include "Sensor.h"
#include "Audio.h"
#include "Display.h"
#include "NMEA.h"

DeviceConfig config;
DeviceMode currentMode = MODE_FLIGHT;
int selectedMenuItem = 0;
int editSubIndex = 0; 
const int TOTAL_MENU_ITEMS = 6;

bool first_reading_done = false;
float current_gain = 0.0;
const int UPDATE_INTERVAL_MS = 100; 
static unsigned long next_update_time = 0;
static float pressure_accumulator = 0;
static int reading_count = 0;
SensorData current_data;

// Variabili per monitorare il rilascio dei tasti
bool lastUpState = HIGH;
bool lastDownState = HIGH;
bool lastSelectState = HIGH;

// Calcola l'offset per fare in modo che la quota letta diventi quella impostata
void calibrate_altitude() {
    // Offset = Quota Desiderata - Quota Grezza Sensore
    // Sottraiamo l'offset vecchio per avere la quota pura del sensore
    float raw_sensor_altitude = current_data.altitude - config.altitude_offset;
    config.altitude_offset = config.starting_altitude - raw_sensor_altitude;
}

void updateParameter(bool increment) {
  float delta = increment ? 0.1 : -0.1;
  switch (selectedMenuItem) {
    case 1: { // ALTIMETRIA
      int factors[] = {1000, 100, 10, 1};
      int val = (int)config.starting_altitude;
      int digit = (val / factors[editSubIndex]) % 10;
      int newDigit = increment ? (digit + 1) % 10 : (digit - 1 + 10) % 10;
      config.starting_altitude += (newDigit - digit) * factors[editSubIndex];
      break;
    }
    case 2: 
      config.lift_threshold = constrain(config.lift_threshold + delta, 0.1, 2.0);
      config.sink_threshold = -config.lift_threshold - 0.2; // Soglia sink leggermente più bassa
      break;
    case 3: 
      config.filter_type = (FilterType)((config.filter_type + (increment ? 1 : 2)) % 3);
      break;
    case 4: 
      if (editSubIndex == 0) config.show_battery_info = !config.show_battery_info;
      else config.show_voltage = !config.show_voltage;
      break;
    case 5: 
      config.thermal_sniffer = !config.thermal_sniffer;
      break;
  }
}

void handleInputs() {
  bool currUp = digitalRead(PIN_UP);
  bool currDown = digitalRead(PIN_DOWN);
  bool currSelect = digitalRead(PIN_SELECT);

  // CLICK SELECT (Al rilascio)
  if (lastSelectState == LOW && currSelect == HIGH) {
    if (currentMode == MODE_FLIGHT) {
      currentMode = MODE_MENU;
    } else if (currentMode == MODE_MENU) {
      if (selectedMenuItem == 0) currentMode = MODE_FLIGHT;
      else { currentMode = MODE_EDIT; editSubIndex = 0; }
    } else if (currentMode == MODE_EDIT) {
      if (selectedMenuItem == 1) { 
        editSubIndex++;
        if (editSubIndex > 3) { calibrate_altitude(); currentMode = MODE_MENU; }
      } else if (selectedMenuItem == 4) { 
        editSubIndex++;
        if (editSubIndex > 1) currentMode = MODE_MENU;
      } else { currentMode = MODE_MENU; }
    }
  }

  // UP (Al rilascio)
  if (lastUpState == LOW && currUp == HIGH) {
    if (currentMode == MODE_FLIGHT) {
      // REGOLA VOLUME SU NELLA SCHERMATA PRINCIPALE
      config.volume = constrain(config.volume + 10, 0, 100);
      Serial.print("Volume: "); Serial.println(config.volume);
    } 
    else if (currentMode == MODE_MENU) {
      selectedMenuItem = (selectedMenuItem - 1 + TOTAL_MENU_ITEMS) % TOTAL_MENU_ITEMS;
    } 
    else if (currentMode == MODE_EDIT) {
      updateParameter(true);
    }
  }

  // DOWN (Al rilascio)
  if (lastDownState == LOW && currDown == HIGH) {
    if (currentMode == MODE_FLIGHT) {
      // REGOLA VOLUME GIÙ NELLA SCHERMATA PRINCIPALE
      config.volume = constrain(config.volume - 10, 0, 100);
      Serial.print("Volume: "); Serial.println(config.volume);
    } 
    else if (currentMode == MODE_MENU) {
      selectedMenuItem = (selectedMenuItem + 1) % TOTAL_MENU_ITEMS;
    } 
    else if (currentMode == MODE_EDIT) {
      updateParameter(false);
    }
  }

  lastUpState = currUp;
  lastDownState = currDown;
  lastSelectState = currSelect;
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
        calibrate_altitude(); // Calibra il sensore all'avvio
        first_reading_done = true;
      }
      
      // Il guadagno ora sarà coerente con la quota impostata
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