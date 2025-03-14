# ESP32 SSTV Transmitter with DHT22 & Custom Font 📡🌡️

Dieses Projekt verwandelt einen ESP32 (ohne PSRAM) in einen SSTV-Sender im Martin M1-Modus. Dabei werden:

- Ein statischer Graustufen-Bildpuffer (320×256) verwendet, der als "dunkelblauer" Hintergrund dient 🎨.
- Ein benutzerdefinierter 5×7-Font (nur Großbuchstaben, Ziffern und ausgewählte Satzzeichen) verwendet, um Text in unterschiedlichen Skalierungen anzuzeigen 🅰️🔠.
- Sensorwerte (Temperatur und Luftfeuchte) vom DHT22 (an GPIO4) ausgelesen und im Bild eingeblendet 🌡️💧.
- Diverse Texte, wie z.B. das Rufzeichen "DA0IGH-11", "VY 73!", Projektinformationen und einen Bildzähler, zentriert ausgegeben 🖼️.
- Das fertige Bild per SSTV über den Audioausgang (GPIO14, mittels LEDC) gesendet 🔊.

## Features 🚀

- **SSTV-Transmission:** Übertragung im Martin M1-Modus, wobei alle Farbkanäle denselben Grauwert verwenden.
- **DHT22 Integration:** Messung von Temperatur und Luftfeuchtigkeit.
- **Custom Font:** Minimaler 5×7-Font, der auch Zeichen wie Q, Y, / und ! enthält.
- **Bildzähler:** Ein Bildzähler ("PIC-NR.") wird automatisch hochgezählt und als dreistellige Zahl angezeigt.
- **Modernes Design:** Alle Texte werden zentriert und mit anpassbaren Abständen ausgegeben.

## Hardware Voraussetzungen 🔧

- **ESP32** (ESP-WROOM-32, ohne PSRAM)
- **DHT22 Sensor** (angeschlossen an GPIO4)
- **Audio-Ausgang** (über GPIO14, entsprechend Ihrer Audio-Hardware)

## Software Voraussetzungen 💻

- Arduino IDE (oder PlatformIO)
- [DHT Sensor Library](https://github.com/adafruit/DHT-sensor-library)
- Sinus-Tabelle (`sin256.h`) – Diese muss im Projekt enthalten sein.
- Passende Board-Einstellungen für den ESP32

## Installation & Verwendung 📥

1. **Repository klonen:**

   ```bash
   git clone https://github.com/deinBenutzername/ESP32_SSTV_DHT22.git
   cd ESP32_SSTV_DHT22
