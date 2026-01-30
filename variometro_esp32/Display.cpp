#include "Display.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
unsigned long volume_display_timer = 0;

// Funzione helper da aggiungere per disegnare la barra
void draw_volume_popup() {
  // Dimensioni del box (schermo largo 64px)
  int boxX = 2;    
  int boxY = 15;   // Alzato leggermente per dare più spazio interno
  int boxW = 60;   
  int boxH = 95;   // Aumentata l'altezza totale

  // Pulisce l'area e disegna il bordo
  display.fillRect(boxX, boxY, boxW, boxH, SSD1306_BLACK);
  display.drawRect(boxX, boxY, boxW, boxH, SSD1306_WHITE);
  
  // Scritta "VOL" centrata e posizionata in alto
  display.setTextSize(2); 
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(14, boxY + 5); // Posizionata a 5 pixel dal bordo superiore del box
  display.print("VOL");

  // Configurazione barre
  int barW = 52; 
  int barH = 6;  
  int gap = 2;   
  int startX = 6; 

  // Calcolo dinamico del punto di partenza dal basso per evitare la scritta
  // Posizioniamo l'ultima barra (la più alta) a debita distanza dal testo
  // Se "VOL" finisce circa a Y=36 (15+5+16), iniziamo la pila di barre più in basso
  int topBarY = boxY + 28; // Lascia circa 28 pixel per la scritta e lo spazio vuoto
  int bottomY = topBarY + (7 * (barH + gap)); 

  // Disegna le 8 tacche dal basso verso l'alto
  for (int i = 0; i < 8; i++) {
    int y = bottomY - (i * (barH + gap));
    
    if (i < config.volume) {
      display.fillRect(startX, y, barW, barH, SSD1306_WHITE);
    } else {
      display.drawRect(startX, y, barW, barH, SSD1306_WHITE);
    }
  }
}

// Funzione per attivare la visualizzazione del volume (da chiamare nel .ino)
void display_show_volume() {
  volume_display_timer = millis() + 2000; // Mostra per 2 secondi
}

void display_init() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    Serial.println(F("Errore: Display SSD1306 non trovato"));
    return;
  }

  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(0xFF); 

  display.setRotation(3); // Orientamento verticale per il cockpit

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 50); 
  display.println(F("Vario"));
  display.display();
  delay(1500);
}

// Visualizzazione principale Modalità Volo
void display_update(const SensorData &data, float start_altitude, float relative_gain) {
  display.clearDisplay();

  // --- INFO BATTERIA ---
  if (config.show_battery_info) {
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(2, 90);
    if (config.show_voltage) {
      display.print(data.battery_voltage, 2);
      display.print("V");
    } else {
      display.print(data.battery_percent);
      display.print("%");
    }
  }

  // --- SEZIONE ALTA: GUADAGNO RELATIVO (Rispetto allo start) ---
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(2, 5);
  if (relative_gain >= 0) display.print("+");
  display.print(String(relative_gain, 0));
  display.print("m");

  // --- SEZIONE BASSA: ALTIMETRIA REALE (AMS) ---
  // CORRETTO: Ora mostra data.filtered_altitude invece di start_altitude
  display.setTextSize(2);
  display.setCursor(2, 110);
  display.print(String(data.filtered_altitude, 0));
  display.print("m");

  display.drawFastHLine(0, 100, 64, SSD1306_WHITE); 

  // --- SEZIONE CENTRALE: VARIOMETRO GRAFICO ---
  int center_y = 62; 
  display.drawFastHLine(14, center_y, 64, SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(2, center_y - 7); 
  display.print("0");

  int bar_width = 32;
  int bar_x = 24;

  if (data.vario_mps > config.lift_threshold) { 
    int num_segments = (data.vario_mps <= 0.5) ? 1 : (data.vario_mps <= 1.0) ? 2 : (data.vario_mps <= 2.0) ? 3 : 4;
    for (int i = 0; i < num_segments; i++) {
      int segment_y = center_y - ((i + 1) * 3) - (i * 2);
      display.fillRect(bar_x, segment_y, bar_width, 3, SSD1306_WHITE);
    }
  }
  else if (data.vario_mps < 0) {
    int bar_height = constrain(map(data.vario_mps * 100, 0, -500, 0, 35), 0, 35);
    if (bar_height > 0) display.fillRect(bar_x, center_y, bar_width, bar_height, SSD1306_WHITE);
  }

  // SEZIONE POP-UP VOLUME:
  if (millis() < volume_display_timer) {
    draw_volume_popup();
  }

  display.display();
}

// Rendering del MENU
void display_menu(int selectedIndex) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(5, 5);
  display.println("MENU");
  display.drawFastHLine(0, 22, 64, SSD1306_WHITE);

  const char* menuItems[] = {" ESCI", " QUOTA", " SENSIB.", " FILTRO", " BATT.", " SNIFFER"};

  display.setTextSize(1);
  for (int i = 0; i < 6; i++) {
    int y = 30 + (i * 15);
    display.setCursor(5, y);
    if (i == selectedIndex) {
      display.fillRect(0, y - 2, 64, 11, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.print(menuItems[i]);
  }
  display.display();
}

// Rendering della MODIFICA PARAMETRI
void display_edit(int selectedIndex, int subIndex) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(5, 5);
  display.println("MODIFICA:");
  display.drawFastHLine(0, 15, 64, SSD1306_WHITE);

  display.setCursor(5, 25);
  switch (selectedIndex) {
    case 1: { // Altitudine
      display.println("QUOTA PARTENZA:");
      display.setTextSize(2);
      display.setCursor(5, 45);
      char buf[10];
      sprintf(buf, "%04d", (int)config.starting_altitude);
      for(int i=0; i<4; i++) {
        if(i == subIndex) {
          display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
          display.print(buf[i]);
          display.setTextColor(SSD1306_WHITE);
        } else {
          display.print(buf[i]);
        }
      }
      display.print("m");
      break;
    }
    case 2:
      display.println("SOGLIA (m/s):");
      display.setTextSize(2); display.setCursor(5, 45);
      display.print(config.lift_threshold, 1);
      break;
    case 3:
      display.println("ALGORITMO:");
      display.setTextSize(1); display.setCursor(5, 45);
      if(config.filter_type == MEDIA_MOBILE) display.print("> MEDIA MOB.");
      else if(config.filter_type == EMA) display.print("> EMA");
      else display.print("> KALMAN");
      break;
    case 4:
      display.println("BATT. INFO:");
      display.setTextSize(1); display.setCursor(5, 45);
      if (subIndex == 0) display.print("VISIB: "), display.println(config.show_battery_info ? "[SI]" : "NO");
      else display.print("TIPO: "), display.println(config.show_voltage ? "[VOLT]" : "%");
      break;
    case 5:
      display.println("THERMAL SNIFFER:");
      display.setTextSize(2); display.setCursor(5, 45);
      display.print(config.thermal_sniffer ? "ATTIVO" : "OFF");
      break;
  }
  display.display();
}