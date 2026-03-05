# Open Source Motivator

[Tiếng Việt bên dưới](#vn-du-an-truyen-dong-luc-nguon-mo)

This project is an open-source motivator that I've "vibe coded" for my school project. I've decided to share my creativity with everyone!

## Required Libraries
To compile this project, you will need the following libraries:
* `Wire.h` / `EEPROM.h` / `vector` (Standard)
* [Adafruit_GFX](https://github.com/adafruit/Adafruit-GFX-Library)
* [Adafruit_SH110X](https://github.com/adafruit/Adafruit_SH110x)
* [ezButton](https://github.com/ArduinoGetStarted/button)
* [RTClib](https://github.com/adafruit/RTClib)
* [pgmspace.h](https://github.com/arduino/ArduinoCore-sam/tree/master/cores/arduino/avr)
* [MakeFont](http://arduino.vn/bai-viet/7505-hien-thi-tieng-viet-va-moi-ngon-ngu-tren-gioi-voi-thu-vien-makefont)

---

## Components List
| Component | Quantity | Note |
| :--- | :---: | :--- |
| ESP32 C3 Super Mini | 1 | Main Controller |
| 1.3 inch OLED Screen | 1 | SH1106 Driver |
| RTC DS3231 Module | 1 | Time keeping |
| TP4056 Charging Module | 1 | Battery Management |
| DC-DC Boost Converter | 1 | HT016 Mini |
| 3.7V Lithium Battery | 1 | |
| Buzzer | 1 | |
| Push Buttons | 5 | |
| Slide Switch | 1 | Power Switch |

---

## Wiring Diagram

### Display & Time (I2C)
*Both OLED and RTC share the same I2C pins.*
* **VCC:** 3V3
* **GND:** GND
* **SCL / SCK:** GPIO 6
* **SDA:** GPIO 5

### Buttons & Audio
| Function | Pin (ESP32 C3) | Connection |
| :--- | :---: | :--- |
| **Up** | 0 | Pin -> GND |
| **Down** | 1 | Pin -> GND |
| **Left** | 3 | Pin -> GND |
| **Right** | 4 | Pin -> GND |
| **OK** | 10 | Pin -> GND |
| **Buzzer (+)** | 7 | (-) to GND |

### Power System
1. **Battery** [P+/B+] -> **TP4056** [B+/B-]
2. **TP4056** [OUT+/OUT-] -> **Boost Converter** [VIN+/VIN-]
3. **Boost Converter** [VOUT+] -> **ESP32** [5V]
4. **Boost Converter** [VOUT-] -> **ESP32** [GND]

---

# VN: Dự án Truyền Động Lực Nguồn Mở

Dự án này là một thiết bị truyền động lực nguồn mở mà tôi đã "vibe coded" cho bài tập ở trường. 

## Thư viện yêu cầu
* `Wire.h`, `EEPROM.h`, `vector`
* Các thư viện Adafruit (GFX, SH110X) và RTClib.
* **MakeFont:** Hỗ trợ hiển thị Tiếng Việt.

## Danh sách linh kiện
* **ESP32 C3 Super Mini** (x1)
* **Màn hình OLED 1.3 inch** (x1)
* **Module RTC DS3231** (x1)
* **Module sạc TP4056** (x1)
* **Mạch tăng áp HT016 mini** (x1)
* **Pin Lithium 3.7V** (x1)
* **Còi chip, Nút nhấn (x5), Công tắc gạt**

## Sơ đồ đấu dây chi tiết
Vui lòng tham khảo bảng đấu nối ở phần tiếng Anh bên trên để đảm bảo độ chính xác của các chân GPIO.
