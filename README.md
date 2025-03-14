# ESP32 SSTV Transmitter mit DHT22 und Textoverlay
Dieses Projekt implementiert einen SSTV-Transmitter (Martin M1-Modus) auf Basis eines ESP32, der ein Graustufenbild mit eingeblendetem Text überträgt. Der Bildpuffer wird statisch im internen RAM angelegt und enthält ein Textoverlay, das u.a. ein Hauptrufzeichen, Sensorwerte vom DHT22 (Temperatur und Humidity) sowie weitere Informationen (Projektname, Bildzähler, QSL-Hinweis) enthält.

## Features

- **SSTV-Übertragung im Martin M1-Modus:**  
  Das Graustufenbild wird zeilenweise in drei Farbkanälen (Grün, Blau, Rot) übertragen, wobei alle Kanäle denselben Wert verwenden.

- **DHT22-Sensor:**  
  Der ESP32 liest Temperatur- und Feuchtigkeitswerte über den DHT22-Sensor (an GPIO4) aus.

- **Textoverlay:**  
  - Großschrift (Scale 5): "DA0IGH-11" (zentriert)
  - Direkt darunter in halber Höhe (Scale 2): "VY 73!"
  - Kleinere Texte (Scale 2), zentriert:
    - "BALLONPROJEKT DASSEL 2025"
    - "DARC ORTSVERBAND R04"
    - "IG HAMSPIRIT E.V."
    - "TEMPERATURE: ..." und "HUMIDITY: ..." (voll ausgeschrieben)
    - "PIC-NR. XXX" (Bildzähler, dreistellig)
    - "PLS QSL VIA"
    - "HTTPS://BALLON.DA0IGH.DE"

- **Audioausgang:**  
  Der Audioausgang wird über LEDC auf GPIO14 realisiert, um den SSTV-Audiosignal zu erzeugen.

## Hardware-Anforderungen
- ESP32 (ESP-WROOM-32) ohne externes PSRAM (das Projekt arbeitet vollständig im internen RAM)
- DHT22-Sensor (verbunden mit GPIO4)
- Audioausgang: Lautsprecher bzw. Verstärker an GPIO14
- Passende Schaltung für SSTV und Audio-PWM

## Software-Anforderungen
- [Arduino IDE](https://www.arduino.cc/en/software) oder ein kompatibles ESP32-Entwicklungssystem
- Installierte Bibliotheken:
  - DHT sensor library (z.B. [Adafruit DHT sensor library](https://github.com/adafruit/DHT-sensor-library))
  - sin256.h (benutzerdefinierte DDS-Sinus-Tabelle, muss vorhanden sein)
  - LEDC (Standard ESP32 Tools)
  
## Installation
1. Klone das Repository oder lade den Quellcode als ZIP herunter.
2. Öffne den Sketch in der Arduino IDE.
3. Stelle in den Board-Einstellungen sicher, dass das richtige ESP32-Board ausgewählt ist.
4. Verbinde den DHT22-Sensor mit GPIO4 und schließe den Lautsprecher/Verstärker an GPIO14 an.
5. Lade den Sketch auf deinen ESP32 hoch.

## Funktionsweise

Der Sketch erstellt einen statischen Graustufenbildpuffer (320×256 Pixel), in den verschiedene Texte eingeblendet werden. Dabei wird der Hintergrund mit einem niedrigen Grauwert gefüllt, um einen "dunkelblauen" Effekt zu simulieren. Der DHT22 liefert Temperatur- und Feuchtigkeitswerte, die ebenfalls in den Bildpuffer geschrieben werden. Ein Bildzähler wird hochgezählt und formatiert als dreistellige Zahl ausgegeben.

Anschließend wird das Bild im Martin M1-Modus per SSTV übertragen, wobei der Audioausgang über den LEDC-Kanal auf GPIO14 realisiert wird.

## Anpassungen
- **Font:**  
  Eine einfache 5x7-Font wird verwendet, die nur Großbuchstaben, Ziffern und einige Sonderzeichen (z.B. Q, Y, /, !) enthält.

## Hinweise

- Falls Probleme bei der Sensor-Abfrage auftreten, prüfe die Verkabelung des DHT22.
- Passe ggf. die Abstände und Skalierungen im Code an, um die Darstellung zu optimieren.

