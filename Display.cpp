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

  // --- NUOVA IMPOSTAZIONE: ROTAZIONE ---
  display.setRotation(3); // Ruota lo schermo di 90 gradi

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 50); // Coordinate basate sulla nuova rotazione (64x128)
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
  
  // Aggiungiamo il segno corretto
  if (relative_gain >= 0) {
    display.print("+");
    display.print(String(relative_gain, 0));
  } else {
    // Il segno meno viene aggiunto automaticamente dalla funzione String()
    display.print(String(relative_gain, 0));
  }
  display.print("m");

  // --- SEZIONE BASSA: QUOTA DI PARTENZA (invariata) ---
  display.setTextSize(2);
  display.setCursor(2, 110);
  display.print(String(start_altitude, 0));
  display.print("m");

  // --- SEPARATORE ORIZZONTALE (invariato) ---
  display.drawFastHLine(0, 100, 64, SSD1306_WHITE); 


  // --- SEZIONE CENTRALE ---
  int center_y = 62; // Il nostro punto "zero" verticale

  // --- NUOVI ELEMENTI: LINEA DELLO ZERO E ETICHETTA "0" ---
  // Disegniamo una linea orizzontale che attraversa tutto lo schermo al centro
  display.drawFastHLine(14, center_y, 64, SSD1306_WHITE);
  // Disegniamo uno "0" a sinistra della linea per renderla più chiara
  display.setTextSize(2);
  display.setCursor(2, center_y - 7); // Centrato verticalmente rispetto alla linea
  display.print("0");

  // Definiamo la geometria delle nostre tacche e della barra
  int bar_width = 32;
  int bar_x = 24;
  int segment_height = 3; // Altezza di ogni tacca in pixel
  int segment_gap = 2;    // Spazio tra una tacca e l'altra

  // --- Logica per la SALITA (a tacche) ---
  if (data.vario_mps > LIFT_THRESHOLD_MPS) { // Usiamo la soglia del buzzer per coerenza
    int num_segments = 0;
    // Determiniamo il numero di tacche da disegnare in base alla tua richiesta
    if (data.vario_mps <= 0.5) {
      num_segments = 1;
    } else if (data.vario_mps <= 1.0) {
      num_segments = 2;
    } else if (data.vario_mps <= 2.0) {
      num_segments = 3;
    } else { // > 2.0 m/s
      num_segments = 4;
    }

    // Disegniamo le tacche una per una, dal basso verso l'alto
    for (int i = 0; i < num_segments; i++) {
      // Calcoliamo la posizione Y della tacca corrente
      int segment_y = center_y - ((i + 1) * segment_height) - (i * segment_gap);
      display.fillRect(bar_x, segment_y, bar_width, segment_height, SSD1306_WHITE);
    }
  }
  // --- Logica per la DISCESA (barra continua) ---
  else if (data.vario_mps < 0) {
    // Per la discesa, usiamo una singola barra la cui altezza è proporzionale
    int bar_height = map(data.vario_mps * 100, 0, -500, 0, 35);
    bar_height = constrain(bar_height, 0, 35);
    if (bar_height > 0) {
      display.fillRect(bar_x, center_y, bar_width, bar_height, SSD1306_WHITE);
    }
  }
  // Se il vario è nella zona morta (tra 0 e LIFT_THRESHOLD_MPS), non disegna nulla.

  display.display();
}