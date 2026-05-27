# LK Technik Speedbox

This project reads NMEA speed data from a GPS receiver (e.g. with RTK correction) and outputs a precise pulse signal to the **7-pin signal connector** of agricultural control units. This compensates for wheel slip and enables significantly more accurate application rate and working width control.

## 📺 YouTube Tutorial

👉 [Watch the LK Technik Speedbox Tutorial](https://www.youtube.com/watch?v=h88T-oXgUjY)

## ⚙️ How It Works

- The Arduino Nano reads NMEA data (`$GPVTG` / `$GNVTG`) via a software UART interface
- The baud rate is detected automatically (4800 / 9600 / 19200 / 38400 / 57600 / 115200)
- The speed in km/h is extracted from the VTG sentence
- **Hardware Timer 1** (pin D9 / OC1A) generates a square wave signal at exactly **130 pulses per meter**
- The signal is amplified via a **ULN2803 Darlington array** and fed to the 7-pin signal connector

## 🔌 Arduino Nano Pin Assignment

| Pin | Function |
|-----|----------|
| D3  | GPS TX (SoftwareSerial) |
| D4  | GPS RX (SoftwareSerial) |
| D9  | Pulse output (OC1A, Timer1) |

## 🛒 Bill of Materials

| Component | Link |
|-----------|------|
| Arduino Nano (ATmega328, Mini-USB) | [Reichelt](https://secure.reichelt.at/at/de/arduino_nano_v3_atmega_328_mini-usb-p142943.html) |
| IC Socket 16-pin | [Reichelt](https://secure.reichelt.at/at/de/ic-sockel_16-polig_doppelter_federkontakt-p8208.html) |
| Fuse holder 37.5x15mm 10A | [Reichelt](https://secure.reichelt.at/at/de/sicherungshalter_37_5x15mm_10a_250v-p376889.html) |
| Enclosure 80x120x59mm black | [Reichelt](https://secure.reichelt.at/at/de/universalgehaeuse_serie_1591_80_x_120_x_59_mm_schwarz-p221294.html) |
| Capacitor 100nF X7R | [Reichelt](https://secure.reichelt.at/at/de/vielschicht-kerko_100_nf_50_100_v_x7r_10_rm_5_0-p22865.html) |
| Fuse 5x20mm 2.5A medium slow-blow | [Reichelt](https://secure.reichelt.at/at/de/feinsicherung_5x20mm_mitteltraege_2_5a-p13246.html) |
| Female header 1x6 2.54mm | [Reichelt](https://secure.reichelt.at/at/de/buchsenleiste_1-reihig_1_x_6_-_2_54_mm-p407096.html) |
| Cable strain relief 3.0x7.1mm | [Reichelt](https://secure.reichelt.at/at/de/zugentlastung_3_0x7_1mm_bis_3_8x7_6mm-p25109.html) |
| ULN2803 Darlington array DIP-16 | [Reichelt](https://secure.reichelt.at/at/de/sieben_darlington-arrays_dip-16-p216685.html) |
| Arduino Nano screw terminal adapter | [Reichelt](https://www.reichelt.at/at/de/shop/produkt/arduino_shield_-_nano_screw_terminal-adapter-339155) |
| DC-DC converter LM2577 | [Reichelt](https://www.reichelt.at/at/de/shop/produkt/entwicklerboards_-_spannungswandler_dc_dc_lm2577-282580) |
| RS232 adapter (Pmod) | [Reichelt](https://www.reichelt.at/at/de/shop/produkt/pmod_rs232_serieller_konverter_und_schnittstellenadapter-243335) |
| Null modem adapter SUB-D 9-pin (for FJ Dynamics / Sveaverken) | [Reichelt](https://www.reichelt.at/at/de/shop/produkt/adapter_sub-d_9pin_buchse_buchse_nullmodem-152506) |
| 7-pin signal socket | [Granit Parts](https://www.granit-parts.com/e/product/50706066?id=49619734) |

## 💻 Software

- **Arduino IDE** or PlatformIO
- Board: **Arduino Nano (ATmega328P)**
- No external libraries required (only `SoftwareSerial.h` from the Arduino standard library)

## 📐 Pulse Calculation
Timer1 runs in **CTC mode** with toggle on OC1A. The prescaler (8 / 64 / 256 / 1024) is selected automatically to keep the OCR1A value within the 16-bit range.
