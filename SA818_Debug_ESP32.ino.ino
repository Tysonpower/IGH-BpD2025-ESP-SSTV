// SA818-Test über Serial1 (RS232) mit Timeout-Überprüfung

#define SA818_TX_PIN 16  // ESP32 TX -> SA818
#define SA818_RX_PIN 15  // ESP32 RX <- SA818

// Wartezeit (Timeout) in Millisekunden für Antworten
#define RESPONSE_TIMEOUT 1000

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Starte SA818-Test...");
  
  Serial1.begin(9600, SERIAL_8N1, SA818_RX_PIN, SA818_TX_PIN);
  delay(100);

  // Schritt 1: AT+VERSION
  Serial.println("Sende: AT+VERSION");
  Serial1.println("AT+VERSION");
  if(!waitForResponse(RESPONSE_TIMEOUT)) {
    Serial.println("Fehler: Keine Antwort auf AT+VERSION erhalten!");
  }
  
  // Schritt 2: AT+DMOCONNECT
  Serial.println("Sende: AT+DMOCONNECT");
  Serial1.println("AT+DMOCONNECT");
  if(!waitForResponse(RESPONSE_TIMEOUT)) {
    Serial.println("Fehler: Keine Antwort auf AT+DMOCONNECT erhalten!");
  }
  
  // Schritt 3: AT+DMOSETGROUP
  Serial.println("Sende: AT+DMOSETGROUP=0,144.5000,144.5000,0000,0,0000");
  Serial1.println("AT+DMOSETGROUP=0,144.5000,144.5000,0000,0,0000");
  if(!waitForResponse(RESPONSE_TIMEOUT)) {
    Serial.println("Fehler: Keine Antwort auf AT+DMOSETGROUP erhalten!");
  }
  
  Serial.println("SA818-Test abgeschlossen.");
  Serial1.end();
}

void loop() {
  // Keine weiteren Aktionen im Loop
}

// Hilfsfunktion: Wartet bis zu 'timeout_ms' auf eine Antwort von Serial1.
// Gibt true zurück, wenn innerhalb der Timeout-Zeit eine Antwort empfangen wurde.
bool waitForResponse(unsigned long timeout_ms) {
  unsigned long startTime = millis();
  String response = "";
  while (millis() - startTime < timeout_ms) {
    if (Serial1.available()) {
      response = Serial1.readStringUntil('\n');
      if(response.length() > 0) {
        Serial.print("Antwort: ");
        Serial.println(response);
        return true;
      }
    }
  }
  return false;
}
