#include <SoftwareSerial.h>

SoftwareSerial gpsSerial(4, 3);   // RX=D4, TX=D3

const float IMPULSES_PER_METER = 130.0;

// Übliche NMEA-Baudraten
const long baudRates[] = {4800, 9600, 19200, 38400, 57600, 115200};
const int baudCount = sizeof(baudRates) / sizeof(baudRates[0]);

float speedKmh = 0.0;
float pulsesPerSecond = 0.0;

String line = "";

unsigned long lastPrintMillis = 0;
unsigned long lastValidNmeaMillis = 0;
unsigned long lastBaudSwitchMillis = 0;

int baudIndex = 0;
bool baudLocked = false;

// Nach 1500 ms ohne gültigen Satz wird neu gesucht
const unsigned long BAUD_SEARCH_TIMEOUT_MS = 1500;
// Alle 1200 ms nächste Baudrate testen, solange nichts Gültiges kommt
const unsigned long BAUD_TRY_TIME_MS = 1200;

void setup() {
  Serial.begin(115200);

  pinMode(9, OUTPUT);   // D9 = OC1A = Timer1 Ausgang
  digitalWrite(9, LOW);

  stopTimer1();

  startGpsBaud(baudRates[baudIndex]);

  Serial.println("Start...");
}

void loop() {
  readGPS();
  autoBaudTask();
  printSpeed();
}

void startGpsBaud(long baud) {
  gpsSerial.end();
  delay(20);
  gpsSerial.begin(baud);

  line = "";
  lastBaudSwitchMillis = millis();

  Serial.print("Teste Baudrate: ");
  Serial.println(baud);
}

void autoBaudTask() {
  unsigned long now = millis();

  // Wenn schon gelockt, aber lange kein gültiger VTG-Satz mehr kam -> neu suchen
  if (baudLocked) {
    if (now - lastValidNmeaMillis > BAUD_SEARCH_TIMEOUT_MS) {
      baudLocked = false;
      speedKmh = 0.0;
      pulsesPerSecond = 0.0;
      stopTimer1();

      Serial.println("Baud-Lock verloren, suche neu...");
      startGpsBaud(baudRates[baudIndex]);
    }
    return;
  }

  // Solange nicht gelockt: zyklisch nächste Baudrate testen
  if (now - lastBaudSwitchMillis > BAUD_TRY_TIME_MS) {
    baudIndex++;
    if (baudIndex >= baudCount) {
      baudIndex = 0;
    }
    startGpsBaud(baudRates[baudIndex]);
  }
}

void readGPS() {
  while (gpsSerial.available()) {
    char c = gpsSerial.read();

    if (c == '\n') {
      parseVTG(line);
      line = "";
    } else if (c != '\r') {
      line += c;
    }

    if (line.length() > 120) {
      line = "";
    }
  }
}

void parseVTG(String s) {
  if (!(s.startsWith("$GNVTG") || s.startsWith("$GPVTG"))) {
    return;
  }

  String fields[12];
  splitCSV(s, fields, 12);

  String kmhField = fields[7];
  removeChecksum(kmhField);

  if (kmhField.length() == 0) {
    return;
  }

  float parsedSpeed = kmhField.toFloat();

  // Gültiger Satz erkannt -> Baudrate ist richtig
  if (!baudLocked) {
    baudLocked = true;
    Serial.print("Baudrate erkannt: ");
    Serial.println(baudRates[baudIndex]);
  }

  lastValidNmeaMillis = millis();

  speedKmh = parsedSpeed;

  // 130 Impulse pro Meter
  // m/s = km/h / 3.6
  // Impulse/s = m/s * 130
  pulsesPerSecond = (speedKmh / 3.6) * IMPULSES_PER_METER;

  updateTimer1(pulsesPerSecond);
}

void printSpeed() {
  if (millis() - lastPrintMillis >= 200) {
    lastPrintMillis = millis();

    Serial.print("Baud: ");
    if (baudLocked) {
      Serial.print(baudRates[baudIndex]);
    } else {
      Serial.print("Suche...");
    }

    Serial.print(" | Geschwindigkeit: ");
    Serial.print(speedKmh, 2);
    Serial.print(" km/h");

    Serial.print(" | Impulse/s: ");
    Serial.println(pulsesPerSecond, 1);
  }
}

void updateTimer1(float frequencyHz) {
  if (frequencyHz < 0.1) {
    stopTimer1();
    return;
  }

  // D9 = OC1A
  // Toggle on compare match in CTC mode:
  // Ausgangsfrequenz = F_CPU / (2 * Prescaler * (1 + OCR1A))

  const uint32_t cpu = 16000000UL;

  uint16_t prescalerBits = 0;
  uint32_t prescaler = 0;
  uint32_t ocr = 0;

  // Prescaler 8
  prescaler = 8;
  ocr = (cpu / (2UL * prescaler * frequencyHz)) - 1UL;

  if (ocr <= 65535UL) {
    prescalerBits = _BV(CS11);
  } else {
    // Prescaler 64
    prescaler = 64;
    ocr = (cpu / (2UL * prescaler * frequencyHz)) - 1UL;

    if (ocr <= 65535UL) {
      prescalerBits = _BV(CS11) | _BV(CS10);
    } else {
      // Prescaler 256
      prescaler = 256;
      ocr = (cpu / (2UL * prescaler * frequencyHz)) - 1UL;

      if (ocr <= 65535UL) {
        prescalerBits = _BV(CS12);
      } else {
        // Prescaler 1024
        prescaler = 1024;
        ocr = (cpu / (2UL * prescaler * frequencyHz)) - 1UL;

        if (ocr > 65535UL) {
          ocr = 65535UL;
        }

        prescalerBits = _BV(CS12) | _BV(CS10);
      }
    }
  }

  noInterrupts();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  // Toggle OC1A on compare match
  TCCR1A |= _BV(COM1A0);

  // CTC mode, TOP = OCR1A
  TCCR1B |= _BV(WGM12);

  OCR1A = (uint16_t)ocr;

  // Start Timer
  TCCR1B |= prescalerBits;

  interrupts();
}

void stopTimer1() {
  noInterrupts();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  digitalWrite(9, LOW);

  interrupts();
}

void splitCSV(String input, String output[], int maxFields) {
  int fieldIndex = 0;
  int start = 0;

  for (int i = 0; i <= input.length(); i++) {
    if (i == input.length() || input.charAt(i) == ',') {
      if (fieldIndex < maxFields) {
        output[fieldIndex] = input.substring(start, i);
        fieldIndex++;
      }
      start = i + 1;
    }
  }
}

void removeChecksum(String &s) {
  int starPos = s.indexOf('*');
  if (starPos >= 0) {
    s = s.substring(0, starPos);
  }
}