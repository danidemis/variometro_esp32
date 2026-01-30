#include "Display.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void display_init() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    Serial.println(F("Errore: Display SSD1306 non trovato"));
    return;
  }

  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(0xFF); 

  display.setRotation(3); 

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 50); 
  display.println(F("Vario"));
  display.display();
  delay(1500);
}

void display_update(const SensorData &data, float start_altitude, float relative_gain) {
  display.clearDisplay();

  // --- SEZIONE ALTA: GUADAGNO RELATIVO ATTUALE ---
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(2, 5);
  
  if (relative_gain >= 0) {
    display.print("+");
    display.print(String(relative_gain, 0));
  } else {
    display.print(String(relative_gain, 0));
  }
  display.print("m");

  // --- SEZIONE BASSA: QUOTA DI PARTENZA ---
  display.setTextSize(2);
  display.setCursor(2, 110);
  display.print(String(start_altitude, 0));
  display.print("m");

  display.drawFastHLine(0, 100, 64, SSD1306_WHITE); 

  // --- SEZIONE CENTRALE ---
  int center_y = 62; 
  display.drawFastHLine(14, center_y, 64, SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(2, center_y - 7); 
  display.print("0");

  int bar_width = 32;
  int bar_x = 24;
  int segment_height = 3; 
  int segment_gap = 2;    

  // --- Logica per la SALITA (a tacche) ---
  // Ora usa la soglia dinamica definita nel menu
  if (data.vario_mps > config.lift_threshold) { 
    int num_segments = 0;
    if (data.vario_mps <= 0.5) {
      num_segments = 1;
    } else if (data.vario_mps <= 1.0) {
      num_segments = 2;
    } else if (data.vario_mps <= 2.0) {
      num_segments = 3;
    } else { 
      num_segments = 4;
    }

    for (int i = 0; i < num_segments; i++) {
      int segment_y = center_y - ((i + 1) * segment_height) - (i * segment_gap);
      display.fillRect(bar_x, segment_y, bar_width, segment_height, SSD1306_WHITE);
    }
  }
  // --- Logica per la DISCESA (barra continua) ---
  else if (data.vario_mps < 0) {
    int bar_height = map(data.vario_mps * 100, 0, -500, 0, 35);
    bar_height = constrain(bar_height, 0, 35);
    if (bar_height > 0) {
      display.fillRect(bar_x, center_y, bar_width, bar_height, SSD1306_WHITE);
    }
  }

  display.display();
}