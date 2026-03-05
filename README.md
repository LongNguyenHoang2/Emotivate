EN:
	This project is an open source motivator that i've vibe coded for my school project, i've decided to make it open source to share my creative to everyone!
	_**Require library:**_
	    -Wire.h
	    -Adafruit_GFX.h
	    -Adafruit_SH110X.h
	    -vector
	    -ezButton.h
	    -EEPROM.h
	    -RTClib.h
	    -pgmspace.h: https://github.com/arduino/ArduinoCore-sam/tree/master/cores/arduino/avr
		-MakeFont: http://arduino.vn/bai-viet/7505-hien-thi-tieng-viet-va-moi-ngon-ngu-tren-gioi-voi-thu-vien-makefont
_**Components List:**_
	    -Esp32 C3 super mini     x1
	    -1,3 inch OLED screen    x1
	    -RTC DS3231 module       x1
	    -TP4056 Charging module  x1
	    -DC-DC Boost Converter   x1 (I use HT016 miny)
	    -3.7V Lithium battery    x1
	    -Buzzer		     x1
	    -Push buttons	     x5
	    -Slide switch            x1
_**Wiring:**_
	    **-OLED Screen** (connect to Esp32 C3):
		+ VCC: 3V3
		+ GND: GND	
		+ SCK: 6
		+ SDA: 5
	    **-RTC modul**e (connect to Esp32 C3, same as OLED screen):
		+ VCC: 3V3
		+ GND: GND
		+ SDA: 5
		+ SCL: 6
	 _**-Buttons:_** (Wire a pin to the Esp's GPIO pin and the other pin to GND):
	    	+ Up: 0
		+ Down: 1
		+ Left: 3
		+ Right: 4
		+ OK: 10
	    **-Buzzer:**
		+ (+): 7
		+ (-): GND
**_-Battery_**(connects to charging module):
		+ Battery's P+/ B+: B+ 
		+ Battery's P-/ B-: B-
	    -Charging module (connects to boost converter):
		+ OUT+: VIN+
		+ OUT-: VIN-
_**	    -Boost converter **_(connects to Esp32 C3):
		+ VOUT+: 5V
		+ VOUT-: GND



VN:
	Dự án này là một thiết bị truyền động lực nguồn mở mà tôi đã "vibe coded" cho bài tập ở trường. Tôi quyết định công khai mã nguồn để chia sẻ sự sáng tạo này tới mọi người!
	_**Thư viện yêu cầu:**_
	    - Wire.h
	    - Adafruit_GFX.h
	    - Adafruit_SH110X.h
	    - vector
	    - ezButton.h
	    - EEPROM.h
	    - RTClib.h
	    - pgmspace.h: https://github.com/arduino/ArduinoCore-sam/tree/master/cores/arduino/avr
		-MakeFont: http://arduino.vn/bai-viet/7505-hien-thi-tieng-viet-va-moi-ngon-ngu-tren-gioi-voi-thu-vien-makefont
_**Danh sách linh kiện:**_
	    - Esp32 C3 super mini x1
	    - Màn hình OLED 1.3 inch x1
	    - Module RTC DS3231 x1
	    - Module sạc TP4056 x1
	    - Mạch tăng áp DC-DC Boost Converter x1 (Tôi dùng loại HT016 mini)
	    - Pin Lithium 3.7V x1
	    - Còi chip (Buzzer) x1
	    - Nút nhấn (Push buttons) x5
	    - Công tắc gạt (Slide switch) x1
	**Sơ đồ đấu dây:**
	   _**-Màn hình OLED**_ (kết nối với Esp32 C3):
        	+ VCC: 3V3
        	+ GND: GND
        	+ SCK: 6
        	+ SDA: 5
	    _**-Module RTC**_(kết nối với Esp32 C3, chung chân với màn hình OLED):
        	+ VCC: 3V3
        	+ GND: GND
        	+ SDA: 5
        	+ SCL: 6
    	    _-**Nút nhấn**_ (Nối một chân vào GPIO của Esp và chân còn lại vào GND):
        	+ Lên (Up): 0
			+ Xuống (Down): 1
        	+ Trái (Left): 3
        	+ Phải (Right): 4
        	+ OK: 10
	  _ **-Còi chip **_(Buzzer):
        	+ (+): 7
        	+ (-): GND
	    _**-Pin **_(kết nối vào module sạc):
        	+ P+/ B+ của Pin: B+
        	+ P-/ B- của Pin: B-
	   _ **-Module sạc**_(kết nối vào mạch tăng áp):
	        +OUT+: VIN+
	       	+OUT-: VIN-
	   ** -Mạch tăng áp **(kết nối với Esp32 C3):
	    	+VOUT+: 5V
	    	+VOUT-: GND
