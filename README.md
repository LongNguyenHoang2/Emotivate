# Open Source Motivator

[Tiếng Việt bên dưới](#vn-du-an-truyen-dong-luc-nguon-mo)

This project is an open-source motivator that I've "vibe coded" for my school project. I've decided to share my creativity with everyone!

## Important Note
* **pgmspace.h:** This library is already included in the source code folder. You do not need to install it manually from external links.

## Required Libraries & Credits
I would like to express my gratitude to the authors of the following libraries:
* **Adafruit GFX & SH110X:** Developed by [Adafruit](https://github.com/adafruit).
* **RTClib:** Originally by JeeLabs, maintained by [Adafruit](https://github.com/adafruit/RTClib).
* **ezButton:** Created by [ArduinoGetStarted](https://github.com/ArduinoGetStarted/button).
* **MakeFont:** Developed by [pham_duy_anh](http://arduino.vn/bai-viet/7505-hien-thi-tieng-viet-va-moi-ngon-ngu-tren-gioi-voi-thu-vien-makefont).
* **Standard Libraries:** `Wire.h`, `EEPROM.h`, `vector`, `pgmspace.h` (Arduino/ESP32 Core teams).

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

## Lưu ý quan trọng
* **pgmspace.h:** Thư viện này đã được tích hợp sẵn trong thư mục mã nguồn của dự án. Bạn không cần phải tải hay cài đặt thêm từ bên ngoài.

## Thư viện yêu cầu & Lời cảm ơn (Credits)
Trân trọng cảm ơn các tác giả và cộng đồng đã phát triển các thư viện:
* **Adafruit GFX & SH110X:** Phát triển bởi [Adafruit](https://github.com/adafruit).
* **RTClib:** Nguyên bản bởi JeeLabs, bảo trì bởi [Adafruit](https://github.com/adafruit/RTClib).
* **ezButton:** Phát triển bởi [ArduinoGetStarted](https://github.com/ArduinoGetStarted/button).
* **MakeFont:** Một thư viện tuyệt vời hỗ trợ tiếng Việt bởi tác giả [pham_duy_anh](http://arduino.vn/bai-viet/7505-hien-thi-tieng-viet-va-moi-ngon-ngu-tren-gioi-voi-thu-vien-makefont).
* **Thư viện hệ thống:** `Wire.h`, `EEPROM.h`, `vector`, `pgmspace.h` (Đội ngũ phát triển Arduino/ESP32 Core).

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
2. **TP4056** [OUT+/OUT-] -> **Mạch tăng áp** [VIN+/VIN-]
3. **Mạch tăng áp** [VOUT+] -> **ESP32** [5V]
4. **Mạch tăng áp** [VOUT-] -> **ESP32** [GND]
