# ESP32 SSTV Transmitter with DHT22 & Custom Font 📡🌡️

Dieses Projekt verwandelt einen ESP32 (ESP-WROOM-32) in einen SSTV-Sender im Martin M1-Modus. Das Gerät erstellt ein Graustufenbild mit eingebettetem Text, der unter anderem das Rufzeichen, einen Bildzähler, Projektinformationen sowie Temperatur- und Luftfeuchtigkeitswerte vom DHT22 enthält. Das fertige Bild wird per SSTV über den Audioausgang (GPIO14) gesendet.

## Features 🚀

- **SSTV-Transmission:** Übertragung eines Graustufenbildes im Martin M1-Modus (alle Farbkanäle erhalten denselben Grauwert).
- **DHT22 Integration:** Misst Temperatur und Luftfeuchtigkeit und zeigt diese im Bild an.
- **Custom Font:** Ein minimaler 5×7-Font, der ausschließlich Großbuchstaben, Ziffern und ausgewählte Satzzeichen (inkl. Q, Y, /, !) enthält.
- **Bildzähler:** Ein Bildzähler ("PIC-NR.") wird automatisch hochgezählt und als dreistellige Zahl (000 bis 999) angezeigt.
- **Zentrierte Ausgabe:** Alle Texte werden zentriert ausgegeben auf einem dunkelblauen Hintergrund.

## Materialliste & Pinout 🔧

- **ESP32 Board (ESP-WROOM-32):**  
  - Hauptcontroller für das Projekt.

- **DHT22 Sensor:**  
  - **Daten-Pin:** GPIO4  
  - **VCC:** 3,3V (oder 5V, je nach Modul)  
  - **GND:** Masse  
  - *Hinweis:* Verwende ggf. einen 4,7kΩ–10kΩ Pull-up-Widerstand, falls dieser nicht im Modul integriert ist.

- **Audio-Ausgang:**  
  - **GPIO14:** Wird für den Audio-PWM-Ausgang (über LEDC) verwendet.  
  - Verbinde diesen Pin mit einem Lautsprecher oder Verstärker.

- **Zusätzliches Zubehör:**  
  - Steckbrett und Jumperkabel  
  - USB-Kabel zum Programmieren des ESP32  
  - Externe Stromversorgung (optional, je nach Setup)

## Software Voraussetzungen 💻

- Arduino IDE oder PlatformIO  
- [DHT Sensor Library](https://github.com/adafruit/DHT-sensor-library)  
- Sinus-Tabelle (`sin256.h`) – muss im Projekt enthalten sein  
- Passende Board-Einstellungen für den ESP32

## Installation & Verwendung 📥

1. **Repository klonen:**

   ```bash
   git clone https://github.com/ighamspirit/IGH-BpD2025-ESP-SSTV.git
   cd IGH-BpD2025-ESP-SSTV
