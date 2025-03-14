#include <driver/ledc.h>
#include "sin256.h"
#include <DHT.h>
#include <string.h>

// ------------------------------
// Pin-Definitionen
// ------------------------------
#define DHTPIN 4         // DHT22 an GPIO4
#define DHTTYPE DHT22
#define PTT_PIN 13       // SA818 PTT an GPIO13
#define MIC_PIN 19       // Mikrofoneingang an GPIO19
#define SA818_TX_PIN 16  // Serial1 TX für SA818 an GPIO16
#define SA818_RX_PIN 15  // Serial1 RX für SA818 an GPIO15
#define AUDIO_OUT_PIN 25 // Audioausgang (LEDC) an GPIO25

// ------------------------------
// DHT22 Setup
// ------------------------------
DHT dht(DHTPIN, DHTTYPE);

// Globaler Bildzähler (dreistellig)
unsigned int picCounter = 0;

// ------------------------------
// SSTV-Konstanten
// ------------------------------
#define BELL202_BAUD 1200
#define F_SAMPLE ((BELL202_BAUD * 32) * 0.92)  // ca. 38400
#define FTOFTW (4294967295UL / F_SAMPLE)

volatile uint32_t FTW = FTOFTW * 1000;
volatile uint32_t PCW = 0;
volatile uint32_t TFLAG = 0;

#define FT_1000 (uint32_t)(1000 * FTOFTW)
#define FT_1100 (uint32_t)(1100 * FTOFTW)
#define FT_1200 (uint32_t)(1200 * FTOFTW)
#define FT_1300 (uint32_t)(1300 * FTOFTW)
#define FT_1500 (uint32_t)(1500 * FTOFTW)
#define FT_1900 (uint32_t)(1900 * FTOFTW)
#define FT_2200 (uint32_t)(2200 * FTOFTW)
#define FT_2300 (uint32_t)(2300 * FTOFTW)
#define FT_SYNC (uint32_t)(FT_1200)

#define TIME_PER_SAMPLE (1000.0 / F_SAMPLE)

// ------------------------------
// Bildauflösung (Martin M1-Modus)
// ------------------------------
#define IMG_WIDTH  320
#define IMG_HEIGHT 256

// Globaler Bildpuffer als Graustufenbild (1 Byte pro Pixel)
uint8_t imageBuffer[IMG_WIDTH * IMG_HEIGHT];

// ------------------------------
// SSTV-Konfiguration (Martin M1, vis_code 44)
// ------------------------------
class SSTV_config_t {
public:
  uint8_t vis_code;
  uint32_t width;
  uint32_t height;
  float line_time;
  float h_sync_time;
  float v_sync_time;
  float c_sync_time;
  float left_margin_time;
  float visible_pixels_time;
  float pixel_time;
  bool color;
  bool martin;
  bool robot;
  
  SSTV_config_t(uint8_t v) {
    vis_code = v;
    switch (vis_code) {
      case 44: // Martin M1 – Graustufen
        robot = false;
        martin = true;
        color = false;
        width = IMG_WIDTH;
        height = IMG_HEIGHT;
        line_time = 446.4460001;
        h_sync_time = 30.0;
        v_sync_time = 4.862;
        c_sync_time = 0.572;
        left_margin_time = 0.0;
        visible_pixels_time = line_time - v_sync_time - left_margin_time - (3 * c_sync_time);
        pixel_time = visible_pixels_time / (width * 3);
        break;
    }
  }
};

SSTV_config_t* currentSSTV = nullptr;
uint8_t* bitmap;  // Zeiger auf den Bildpuffer

// ------------------------------
// SSTV-Übertragungsvariablen
// ------------------------------
volatile uint16_t rasterX = 0;
volatile uint16_t rasterY = 0;
volatile uint8_t SSTVseq = 0;
double SSTVtime = 0;
double SSTVnext = 0;
uint8_t VISsr = 0;
uint8_t VISparity = 0;
uint8_t HEADERptr = 0;
static uint32_t SSTV_HEADER[] = {
  FT_2300, 100, FT_1500, 100, FT_2300, 100, FT_1500, 100,
  FT_1900, 300, FT_1200, 10, FT_1900, 300, FT_1200, 30, 0, 0
};
uint8_t SSTV_RUNNING = 0;
TaskHandle_t sampleHandlerHandle;

// ------------------------------
// Audio-ISR (SSTV-Modus)
// ------------------------------
void IRAM_ATTR audioISR() {
  PCW += FTW;
  TFLAG = 1;
}

// ------------------------------
// SSTV-Task
// ------------------------------
void sampleHandler(void *p) {
  disableCore0WDT();
  while (1) {
    if (TFLAG) {
      TFLAG = 0;
      int v = SinTableH[((uint8_t*)&PCW)[3]];
      ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_3, v);
      ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_3);
      SSTVtime += TIME_PER_SAMPLE;
      if (!SSTV_RUNNING || SSTVtime < SSTVnext)
        goto sstvEnd;
      switch (SSTVseq) {
        case 0:
          SSTVtime = 0;
          HEADERptr = 0;
          VISparity = 0;
          VISsr = currentSSTV->vis_code;
          FTW = SSTV_HEADER[HEADERptr++];
          SSTVnext = (float)SSTV_HEADER[HEADERptr++];
          SSTVseq++;
          break;
        case 1:
          if (SSTV_HEADER[HEADERptr + 1] == 0) {
            SSTVseq++;
            HEADERptr = 0;
          } else {
            FTW = SSTV_HEADER[HEADERptr++];
            SSTVnext += (float)SSTV_HEADER[HEADERptr++];
          }
          break;
        case 2: {
          if (HEADERptr == 7) {
            HEADERptr = 0;
            FTW = (VISparity) ? FT_1100 : FT_1300;
            SSTVnext += 30.0;
            SSTVseq++;
          } else {
            if (VISsr & 0x01) {
              VISparity ^= 0x01;
              FTW = FT_1100;
            } else {
              FTW = FT_1300;
            }
            VISsr >>= 1;
            SSTVnext += 30.0;
            HEADERptr++;
          }
        }
          break;
        case 3:
          FTW = FT_1200;
          SSTVnext += 30.0 + currentSSTV->h_sync_time;
          rasterX = 0;
          rasterY = 0;
          SSTVseq = 10;
          break;
        case 10:  // "Grüner" Kanal
          if (rasterX == currentSSTV->width) {
            rasterX = 0;
            FTW = FT_1500;
            SSTVnext += currentSSTV->c_sync_time;
            SSTVseq++;
          } else {
            int val = bitmap[rasterX + rasterY * currentSSTV->width];
            int f = map(val, 0, 255, 1500, 2300);
            FTW = FTOFTW * f;
            SSTVnext += currentSSTV->pixel_time;
            rasterX++;
          }
          break;
        case 11:  // "Blauer" Kanal
          if (rasterX == currentSSTV->width) {
            rasterX = 0;
            FTW = FT_1500;
            SSTVnext += currentSSTV->c_sync_time;
            SSTVseq++;
          } else {
            int val = bitmap[rasterX + rasterY * currentSSTV->width];
            int f = map(val, 0, 255, 1500, 2300);
            FTW = FTOFTW * f;
            SSTVnext += currentSSTV->pixel_time;
            rasterX++;
          }
          break;
        case 12:  // "Roter" Kanal
          if (rasterX == currentSSTV->width) {
            rasterX = 0;
            rasterY++;
            if (rasterY == currentSSTV->height) {
              SSTV_RUNNING = false;
              SSTVseq = 0;
              FTW = 0;
              PCW = 0;
            } else {
              FTW = FT_SYNC;
              SSTVnext += currentSSTV->v_sync_time;
              SSTVseq = 10;
            }
          } else {
            int val = bitmap[rasterX + rasterY * currentSSTV->width];
            int f = map(val, 0, 255, 1500, 2300);
            FTW = FTOFTW * f;
            SSTVnext += currentSSTV->pixel_time;
            rasterX++;
          }
          break;
      }
sstvEnd:
      ;
    }
  }
}

// ------------------------------
// Minimaler 5x7-Font (nur Großbuchstaben, Ziffern, Satzzeichen)
// ------------------------------
struct Font5x7 {
  char c;
  uint8_t data[5];
};

const Font5x7 font[] = {
  { ' ', { 0x00, 0x00, 0x00, 0x00, 0x00 } },
  { 'A', { 0x7C, 0x12, 0x11, 0x12, 0x7C } },
  { 'B', { 0x7F, 0x49, 0x49, 0x49, 0x36 } },
  { 'C', { 0x3E, 0x41, 0x41, 0x41, 0x22 } },
  { 'D', { 0x7F, 0x41, 0x41, 0x41, 0x3E } },
  { 'E', { 0x7F, 0x49, 0x49, 0x49, 0x41 } },
  { 'G', { 0x3E, 0x41, 0x49, 0x49, 0x7A } },
  { 'H', { 0x7F, 0x08, 0x08, 0x08, 0x7F } },
  { 'I', { 0x00, 0x41, 0x7F, 0x41, 0x00 } },
  { 'J', { 0x20, 0x40, 0x41, 0x3F, 0x01 } },
  { 'K', { 0x7F, 0x08, 0x14, 0x22, 0x41 } },
  { 'L', { 0x7F, 0x40, 0x40, 0x40, 0x40 } },
  { 'M', { 0x7F, 0x02, 0x0C, 0x02, 0x7F } },
  { 'N', { 0x7F, 0x02, 0x04, 0x08, 0x7F } },
  { 'O', { 0x3E, 0x41, 0x41, 0x41, 0x3E } },
  { 'P', { 0x7F, 0x09, 0x09, 0x09, 0x06 } },
  { 'Q', { 0x3E, 0x41, 0x51, 0x21, 0x5E } },
  { 'R', { 0x7F, 0x09, 0x19, 0x29, 0x46 } },
  { 'S', { 0x46, 0x49, 0x49, 0x49, 0x31 } },
  { 'T', { 0x01, 0x01, 0x7F, 0x01, 0x01 } },
  { 'U', { 0x3F, 0x40, 0x40, 0x40, 0x3F } },
  { 'V', { 0x1F, 0x20, 0x40, 0x20, 0x1F } },
  // Angepasstes Y – Alternative Version
  { 'Y', { 0x04, 0x08, 0x70, 0x08, 0x04 } },
  
  // Ziffern
  { '0', { 0x3E, 0x45, 0x49, 0x51, 0x3E } },
  { '1', { 0x00, 0x41, 0x7F, 0x40, 0x00 } },
  { '2', { 0x42, 0x61, 0x51, 0x49, 0x46 } },
  { '3', { 0x21, 0x41, 0x45, 0x4B, 0x31 } },
  { '4', { 0x18, 0x14, 0x12, 0x7F, 0x10 } },
  { '5', { 0x27, 0x45, 0x45, 0x45, 0x39 } },
  { '6', { 0x3C, 0x4A, 0x49, 0x49, 0x30 } },
  { '7', { 0x01, 0x71, 0x09, 0x05, 0x03 } },
  { '8', { 0x36, 0x49, 0x49, 0x49, 0x36 } },
  { '9', { 0x06, 0x49, 0x49, 0x29, 0x1E } },
  
  // Satzzeichen und Sonderzeichen
  { '-', { 0x08, 0x08, 0x08, 0x08, 0x08 } },
  { ':', { 0x00, 0x36, 0x36, 0x00, 0x00 } },
  { '.', { 0x00, 0x40, 0x00, 0x00, 0x00 } },
  { '%', { 0x62, 0x64, 0x18, 0x26, 0x46 } },
  { '/', { 0x20, 0x10, 0x08, 0x04, 0x02 } },
  { '!', { 0x00, 0x00, 0x5F, 0x00, 0x00 } }
};
  
const int fontCount = sizeof(font) / sizeof(Font5x7);

// ------------------------------
// Zeichnen in den Bildpuffer (Graustufen)
// ------------------------------
void setPixel(int x, int y, uint8_t val) {
  if (x < 0 || x >= IMG_WIDTH || y < 0 || y >= IMG_HEIGHT) return;
  imageBuffer[y * IMG_WIDTH + x] = val;
}
  
void drawChar(int x, int y, char c, int scale, uint8_t val) {
  const uint8_t* bitmapChar = nullptr;
  for (int i = 0; i < fontCount; i++) {
    if (font[i].c == c) {
      bitmapChar = font[i].data;
      break;
    }
  }
  if (!bitmapChar) return;
  for (int col = 0; col < 5; col++) {
    uint8_t colData = bitmapChar[col];
    for (int row = 0; row < 7; row++) {
      if (colData & (1 << row)) {
        for (int dx = 0; dx < scale; dx++) {
          for (int dy = 0; dy < scale; dy++) {
            setPixel(x + col * scale + dx, y + row * scale + dy, val);
          }
        }
      }
    }
  }
}
  
void drawText(int x, int y, const char* text, int scale, uint8_t val) {
  int cursor = x;
  for (int i = 0; text[i] != '\0'; i++) {
    drawChar(cursor, y, text[i], scale, val);
    cursor += 5 * scale + 2; // mindestens 2 Pixel Abstand
  }
}
  
// ------------------------------
// Bild erstellen: Hintergrund und Text-Overlay
// ------------------------------
void createImage() {
  // Fülle den Bildpuffer mit Grauwert 40 als "dunkelblauer" Hintergrund
  for (int y = 0; y < IMG_HEIGHT; y++) {
    for (int x = 0; x < IMG_WIDTH; x++) {
      imageBuffer[y * IMG_WIDTH + x] = 40;
    }
  }
  
  // Obere Zeile: Großschrift (Scale 5) – "DA0IGH-11"
  const char* headerText = "DA0IGH-11";
  int bigScale = 5;
  int headerWidth = 9 * (5 * bigScale + 2) - 2;
  int headerHeight = 7 * bigScale;
  int posX = (IMG_WIDTH - headerWidth) / 2;
  int posY = 10;
  drawText(posX, posY, headerText, bigScale, 255);
  
  // Direkt darunter: In halber Höhe (Scale 2) soll "VY 73!" erscheinen.
  const char* secondaryText = "VY 73!";
  int secScale = 2;
  int secWidth = 6 * (5 * secScale + 2) - 2;
  int posY_sec = posY + headerHeight + 10;
  drawText((IMG_WIDTH - secWidth) / 2, posY_sec, secondaryText, secScale, 255);
  
  // Weitere Zeilen in kleiner Schrift (Scale 2):
  int smallScale = 2;
  int lineSpacing = 4;
  int lineY = posY_sec + (7 * secScale) + 10;
  drawText(10, lineY, "BALLONPROJEKT DASSEL 2025", smallScale, 255);
  lineY += 7 * smallScale + lineSpacing;
  drawText(10, lineY, "DARC ORTSVERBAND R04", smallScale, 255);
  lineY += 7 * smallScale + lineSpacing;
  drawText(10, lineY, "IG HAMSPIRIT E.V.", smallScale, 255);
  
  // Sensordaten ausschreiben:
  lineY += 7 * smallScale + 10;
  char buf[32];
  sprintf(buf, "TEMPERATURE: %.1fC", dht.readTemperature());
  drawText(10, lineY, buf, smallScale, 255);
  lineY += 7 * smallScale + lineSpacing;
  sprintf(buf, "HUMIDITY: %.1f%%", dht.readHumidity());
  drawText(10, lineY, buf, smallScale, 255);
  
  // Bildzähler:
  lineY += 7 * smallScale + 10;
  sprintf(buf, "PIC-NR. %03d", picCounter);
  drawText(10, lineY, buf, smallScale, 255);
  picCounter = (picCounter + 1) % 1000;
  
  // Zusätzliche zwei Zeilen am unteren Rand:
  lineY += 7 * smallScale + 10;
  drawText(10, lineY, "PLS QSL VIA", smallScale, 255);
  lineY += 7 * smallScale + lineSpacing;
  drawText(10, lineY, "HTTPS://BALLON.DA0IGH.DE", smallScale, 255);
  
  bitmap = imageBuffer;
}

// ------------------------------
// Morse-Funktionen
// ------------------------------
const char* getMorse(char c) {
  switch (toupper(c)) {
    case 'A': return ".-";
    case 'B': return "-...";
    case 'C': return "-.-.";
    case 'D': return "-..";
    case 'E': return ".";
    case 'F': return "..-.";
    case 'G': return "--.";
    case 'H': return "....";
    case 'I': return "..";
    case 'J': return ".---";
    case 'K': return "-.-";
    case 'L': return ".-..";
    case 'M': return "--";
    case 'N': return "-.";
    case 'O': return "---";
    case 'P': return ".--.";
    case 'Q': return "--.-";
    case 'R': return ".-.";
    case 'S': return "...";
    case 'T': return "-";
    case 'U': return "..-";
    case 'V': return "...-";
    case 'W': return ".--";
    case 'X': return "-..-";
    case 'Y': return "-.--";
    case 'Z': return "--..";
    case '0': return "-----";
    case '1': return ".----";
    case '2': return "..---";
    case '3': return "...--";
    case '4': return "....-";
    case '5': return ".....";
    case '6': return "-....";
    case '7': return "--...";
    case '8': return "---..";
    case '9': return "----.";
    default: return "";
  }
}

void sendTone(unsigned long duration_ms) {
  // Erzeuge Morse-Ton: Setze LEDC-Duty (hier 128) für die Dauer des Tons
  ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_3, 128);
  ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_3);
  delay(duration_ms);
  ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_3, 0);
  ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_3);
}

void sendMorse(const char* message, int wpm) {
  int dotDuration = 1200 / wpm; // Dot-Dauer in ms (bei 60 WPM = 20 ms)
  
  // Stelle LEDC-Frequenz für Morse-Ton auf 1000 Hz ein
  ledc_set_freq(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_1, 1900);
  
  for (int i = 0; message[i] != '\0'; i++) {
    char c = message[i];
    if (c == ' ') {
      delay(dotDuration * 7); // Wortabstand
    } else {
      const char* code = getMorse(c);
      for (int j = 0; code[j] != '\0'; j++) {
        if (code[j] == '.') {
          sendTone(dotDuration);
        } else if (code[j] == '-') {
          sendTone(dotDuration * 3);
        }
        // Zwischen den Symbolen eines Buchstabens: 1 Dot-Pause (wenn nicht letztes Symbol)
        if (code[j+1] != '\0') {
          delay(dotDuration);
        }
      }
      // Buchstabenpause: insgesamt 3 Dot-Pausen, da bereits 1 Dot-Pause erfolgt
      delay(dotDuration * 2);
    }
  }
  
  // Setze LEDC-Frequenz zurück auf 200000 Hz (für SSTV/Voice)
  ledc_set_freq(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_1, 200000);
}

// ------------------------------
// SA818 Sprachmodus: Mikrofon abtasten und Audio ausgeben
// ------------------------------
#define VOICE_TX_DURATION 10000  // Dauer in ms
#define VOICE_SAMPLE_DELAY 10    // Wartezeit zwischen ADC-Lesungen in ms

void transmitVoice() {
  // PTT einschalten
  digitalWrite(PTT_PIN, HIGH);
  unsigned long startTime = millis();
  while (millis() - startTime < VOICE_TX_DURATION) {
    int micVal = analogRead(MIC_PIN); // Mikrofon an GPIO14
    uint32_t duty = map(micVal, 0, 4095, 0, 255);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_3, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_3);
    delay(VOICE_SAMPLE_DELAY);
  }
  // PTT ausschalten
  digitalWrite(PTT_PIN, LOW);
}
  
// ------------------------------
// SSTV-Übertragung (Bildmodus)
// ------------------------------
void doImage() {
  createImage();
  if (currentSSTV) { delete currentSSTV; currentSSTV = nullptr; }
  currentSSTV = new SSTV_config_t(44);
  Serial.println(TIME_PER_SAMPLE, 10);
  Serial.println("Sending image via SSTV...");
  SSTVtime = 0;
  SSTVnext = 0;
  SSTVseq = 0;
  SSTV_RUNNING = true;
  vTaskResume(sampleHandlerHandle);
  while (SSTV_RUNNING) {
    Serial.print(".");
    delay(1000);
  }
  vTaskSuspend(sampleHandlerHandle);
  Serial.println("Image transmission finished.");
  delay(5000);
}
  
// ------------------------------
// Setup & Loop
// ------------------------------
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Start..");
  delay(500);
  
  dht.begin();
  
  // PTT-Pin konfigurieren
  pinMode(PTT_PIN, OUTPUT);
  digitalWrite(PTT_PIN, HIGH);
  
  // RS232 für SA818 konfigurieren (Serial1)
  Serial1.begin(9600, SERIAL_8N1, SA818_RX_PIN, SA818_TX_PIN);
  delay(100);
  Serial1.println("AT+VERSION");
  delay(500);
  while (Serial1.available()) { Serial.println(Serial1.readStringUntil('\n')); }
  Serial1.println("AT+DMOCONNECT");
  delay(500);
  while (Serial1.available()) { Serial.println(Serial1.readStringUntil('\n')); }
  Serial1.println("AT+DMOSETGROUP=0,144.5000,144.5000,0000,0,0000");
  delay(500);
  while (Serial1.available()) { Serial.println(Serial1.readStringUntil('\n')); }
  
  // Teste DHT22:
  float testTemp = dht.readTemperature();
  if (isnan(testTemp)) {
    Serial.println("DHT22 NICHT vorhanden oder Fehler beim Lesen!");
  } else {
    Serial.print("DHT22 vorhanden, Temp: ");
    Serial.print(testTemp);
    Serial.println(" C");
  }
  
  // Konfiguriere Hardware-Timer & LEDC für Audio-PWM (SSTV & Voice/Morse)
  hw_timer_t* timer = timerBegin(2, 10, true);
  timerAttachInterrupt(timer, &audioISR, true);
  timerAlarmWrite(timer, 8000000 / F_SAMPLE, true);
  timerAlarmEnable(timer);
  
  ledc_timer_config_t ledc_timer;
  ledc_timer.speed_mode = LEDC_HIGH_SPEED_MODE;
  ledc_timer.duty_resolution = LEDC_TIMER_8_BIT;
  ledc_timer.timer_num = LEDC_TIMER_1;
  ledc_timer.freq_hz = 200000;
  ledc_timer_config(&ledc_timer);
  
  ledc_channel_config_t ledc_channel;
  ledc_channel.channel    = LEDC_CHANNEL_3;
  ledc_channel.gpio_num   = AUDIO_OUT_PIN;  // Audioausgang an GPIO25
  ledc_channel.speed_mode = LEDC_HIGH_SPEED_MODE;
  ledc_channel.timer_sel  = LEDC_TIMER_1;
  ledc_channel.duty       = 2;
  ledc_channel.hpoint     = 0;
  ledc_channel_config(&ledc_channel);
  
  // Erstelle den SSTV-Task und pausiere ihn initial
  xTaskCreatePinnedToCore(sampleHandler, "SSTV", 4096, NULL, 1, &sampleHandlerHandle, 0);
  vTaskSuspend(sampleHandlerHandle);
}
  
void loop() {
  digitalWrite(PTT_PIN, LOW);
  // Zuerst SSTV-Bildmodus:
  doImage();
  
  // Dann Morseübertragung des Rufzeichens "DA0IGH" bei 60 WPM:
  Serial.println("Sending Morse: DA0IGH");
  sendMorse("DA0IGH", 15);
  Serial.println("Morse transmission finished.");
  
  // Danach Sprachmodus (optional):
  //Serial.println("Starting voice transmission...");
  //transmitVoice();
  //Serial.println("Voice transmission finished.");
  
  digitalWrite(PTT_PIN, HIGH);
  delay(45000);
  Serial.println(millis());
}
