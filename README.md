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

# VN: Dự án Truyền Động Lực Nguồn Mở

Dự án này là một thiết bị truyền động lực nguồn mở mà tôi đã "vibe coded" cho bài tập ở trường. Tôi quyết định công khai mã nguồn để chia sẻ sự sáng tạo này tới mọi người!

## Thư viện yêu cầu
* `Wire.h`, `EEPROM.h`, `vector` (Thư viện chuẩn)
* Các thư viện Adafruit (GFX, SH110X) và RTClib.
* **MakeFont:** Hỗ trợ hiển thị Tiếng Việt.

## Danh sách linh kiện
| Linh kiện | Số lượng | Ghi chú |
| :--- | :---: | :--- |
| ESP32 C3 Super Mini | 1 | Vi điều khiển chính |
| Màn hình OLED 1.3 inch | 1 | Driver SH1106 |
| Module RTC DS3231 | 1 | Thời gian thực |
| Module sạc TP4056 | 1 | Quản lý sạc pin |
| Mạch tăng áp DC-DC | 1 | HT016 Mini |
| Pin Lithium 3.7V | 1 | |
| Còi chip (Buzzer) | 1 | |
| Nút nhấn | 5 | |
| Công tắc gạt | 1 | Công tắc nguồn |

---

## Sơ đồ đấu dây chi tiết

### Màn hình & Thời gian (I2C)
*Cả màn hình OLED và module RTC dùng chung chân I2C.*
* **VCC:** 3V3
* **GND:** GND
* **SCL / SCK:** GPIO 6
* **SDA:** GPIO 5

### Nút nhấn & Âm thanh
| Chức năng | Chân (ESP32 C3) | Cách đấu |
| :--- | :---: | :--- |
| **Lên (Up)** | 0 | Chân nút -> GND |
| **Xuống (Down)** | 1 | Chân nút -> GND |
| **Trái (Left)** | 3 | Chân nút -> GND |
| **Phải (Right)** | 4 | Chân nút -> GND |
| **OK** | 10 | Chân nút -> GND |
| **Còi (+) (Buzzer)** | 7 | Chân (-) nối GND |

### Hệ thống nguồn
1. **Pin** [P+/B+] -> **TP4056** [B+/B-]
2. **TP4056** [OUT+/OUT-] -> **Mạch tăng áp** [VIN+/VIN- ]
3. **Mạch tăng áp** [VOUT+] -> **ESP32** [5V]
4. **Mạch tăng áp** [VOUT-] -> **ESP32** [GND]
