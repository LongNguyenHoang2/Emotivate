EN:
	This project is an open source motivator that i've vibe coded for my school project, i've decided to make it open source to share my creative to everyone!
	Require library:
	    -Wire.h
	    -Adafruit_GFX.h
	    -Adafruit_SH110X.h
	    -vector
	    -ezButton.h
	    -EEPROM.h
	    -RTClib.h
	    -pgmspace.h: https://github.com/arduino/ArduinoCore-sam/tree/master/cores/arduino/avr
	Components List:
	    -Esp32 C3 super mini     x1
	    -1,3 inch OLED screen    x1
	    -RTC DS3231 module       x1
	    -TP4056 Charging module  x1
	    -DC-DC Boost Converter   x1 (I use HT016 miny)
	    -3.7V Lithium battery    x1
	    -Buzzer		     x1
	    -Push buttons	     x5
	    -Slide switch            x1
	Wiring:
	    -OLED Screen (connect to Esp32 C3):
		+ VCC: 3V3
		+ GND: GND	
		+ SCK: 6
		+ SDA: 5
	    -RTC module (connect to Esp32 C3, same as OLED screen):
		+ VCC: 3V3
		+ GND: GND
		+ SDA: 5
		+ SCL: 6
	    -Buttons: (Wire a pin to the Esp's GPIO pin and the other pin to GND):
	    	+ Up: 0
		+ Down: 1
		+ Left: 3
		+ Right: 4
		+ OK: 10
	    -Buzzer:
		+ (+): 7
		+ (-): GND
	    -Battery (connects to charging module):
		+ Battery's P+/ B+: B+ 
		+ Battery's P-/ B-: B-
	    -Charging module (connects to boost converter):
		+ OUT+: VIN+
		+ OUT-: VIN-
	    -Boost converter (connects to Esp32 C3):
		+ VOUT+: 5V
		+ VOUT-: GND

VN:
	Dự án này là một thiết bị truyền động lực nguồn mở mà tôi đã "vibe coded" cho bài tập ở trường. Tôi quyết định công khai mã nguồn để chia sẻ sự sáng tạo này tới mọi người!
	Thư viện yêu cầu:
	    - Wire.h
	    - Adafruit_GFX.h
	    - Adafruit_SH110X.h
	    - vector
	    - ezButton.h
	    - EEPROM.h
	    - RTClib.h
	    - pgmspace.h: https://github.com/arduino/ArduinoCore-sam/tree/master/cores/arduino/avr
	Danh sách linh kiện:
	    - Esp32 C3 super mini x1
	    - Màn hình OLED 1.3 inch x1
	    - Module RTC DS3231 x1
	    - Module sạc TP4056 x1
	    - Mạch tăng áp DC-DC Boost Converter x1 (Tôi dùng loại HT016 mini)
	    - Pin Lithium 3.7V x1
	    - Còi chip (Buzzer) x1
	    - Nút nhấn (Push buttons) x5
	    - Công tắc gạt (Slide switch) x1
	Sơ đồ đấu dây:
	    -Màn hình OLED (kết nối với Esp32 C3):
        	+ VCC: 3V3
        	+ GND: GND
        	+ SCK: 6
        	+ SDA: 5
	    -Module RTC (kết nối với Esp32 C3, chung chân với màn hình OLED):
        	+ VCC: 3V3
        	+ GND: GND
        	+ SDA: 5
        	+ SCL: 6
    	    -Nút nhấn (Nối một chân vào GPIO của Esp và chân còn lại vào GND):
        	+ Lên (Up): 0
		+ Xuống (Down): 1
        	+ Trái (Left): 3
        	+ Phải (Right): 4
        	+ OK: 10
	    -Còi chip (Buzzer):
        	+ (+): 7
        	+ (-): GND
	    -Pin (kết nối vào module sạc):
        	+ P+/ B+ của Pin: B+
        	+ P-/ B- của Pin: B-
	    -Module sạc (kết nối vào mạch tăng áp):
	        +OUT+: VIN+
	       	+OUT-: VIN-
	    -Mạch tăng áp (kết nối với Esp32 C3):
	    	+VOUT+: 5V
	    	+VOUT-: GND
