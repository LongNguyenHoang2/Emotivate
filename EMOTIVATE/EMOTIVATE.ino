#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <vector>
#include <ezButton.h>
#include <EEPROM.h>
#include <RTClib.h>
#include <pgmspace.h>               
#include <FontMaker.h>

#define SW 128
#define SH 64
#define BZ 7
#define EEPROM_SIZE 512
Adafruit_SH1106G display = Adafruit_SH1106G(SW, SH, &Wire, -1);
RTC_DS3231 rtc;

// Ham ve diem anh cho FontMaker
void setpx(int16_t x, int16_t y, uint16_t color) {
  display.drawPixel(x, y, color);
}
MakeFont myfont(&setpx);

// Dinh nghia cac chan nut bam (Sua o day neu muon doi chan)
#define PIN_UP    0
#define PIN_DOWN  1
#define PIN_LEFT  3
#define PIN_RIGHT 4
#define PIN_OK    10

ezButton nL(PIN_UP), nX(PIN_DOWN), nTr(PIN_LEFT), nP(PIN_RIGHT), nOk(PIN_OK);

// --- PROGMEM Data for Messages ---
// This moves all the constant strings into Flash memory to save RAM.
struct Msg {
  PGM_P text1_VI; // Quote or Joke Setup (VI)
  PGM_P text1_EN; // Quote or Joke Setup (EN)
  PGM_P text2_VI; // Advice or Punchline (VI)
  PGM_P text2_EN; // Advice or Punchline (EN)
};

enum State { E_SEL, E_VIEW, ASK, G_SEL, DIFF_SEL, TETRIS, CROSSY, PINGPONG, PAUSE, GAME_OVER, SETTINGS, SUMMARY, CHECK_DANGER, DANGER_WARN,
             RESET_CONFIRM, SNAKE, DINO, FLAPPY, GADGETS, STOPWATCH, TIMER, DICE, TIMER_ALERT, SET_TIME, GOAL_MENU, ADD_PLAN, CURRENT_PLAN, GOAL_REMINDER };
State st = E_SEL;
State returnState = E_SEL; // To know where to return from Settings

// --- Animation Vars ---
int eIdx = 0;
float currentX = 0;
float targetX = 0;
unsigned long lastTick = 0;

// --- Game Vars ---
int sc = 0, mIdx = 0, highScore = 0, gameChoice = 0;
bool grid[10][20];
int px=4, py=0, pT=0, pR=0;
float ckX=64, ckY=58, carsX[4], carSpd[4];
float bX, bY, bDX, bDY; int p1Y;
State lastGame = E_SEL; // To resume after pause
int pauseOpt = 0; // 0: Resume, 1: Exit
int difficulty = 0; // 0: Easy, 1: Medium, 2: Hard
bool soundOff = false; // 0: On, 1: Off (Default 0 in EEPROM)
bool gamePlaying = false;
int language = 0; // 0: VI, 1: EN

// --- Snake Vars --
int snX[200], snY[200], snLen;
int snDirX, snDirY, apX, apY;
// --- Dino Vars ---
float dY, dV, obsX; int obsW, obsH;
// --- Flappy Bird Vars ---
float fbY, fbV;
int pipeX, pipeH;
const int pipeW = 8;
const int pipeGap = 28;

// --- Gadget Vars ---
unsigned long swStart = 0, swElapsed = 0;
bool swRun = false;
long tmDuration = 0, tmRem = 0;
unsigned long tmStart = 0;
bool tmRun = false;
int tmHour = 0, tmMin = 0, tmSec = 0;
int alertOkCount = 0;
int diceVal = 1;

// --- RTC & Log Vars ---
int setIdx = 0; // 0: Hour, 1: Min, 2: Day, 3: Month, 4: Year
DateTime adjustTime;

// --- Goal Vars ---
const char* goalsVI[] = {"Uống nước", "Tập thể dục", "Đọc sách", "Ngủ sớm", "Thiền", 
                         "Giúp đỡ người", "Học tập", "Dọn dẹp", "Nấu ăn", "Tưới cây"};
const char* goalsEN[] = {"Drink water", "Exercise", "Read book", "Sleep early", "Meditate", 
                         "Help others", "Studying", "Cleaning", "Cooking", "Water plants"};

const uint16_t shapes[7][4] = {
  {0x0F00, 0x4444, 0x0F00, 0x4444}, {0x4460, 0x0E80, 0xC440, 0x2E00},
  {0x44C0, 0x8E00, 0x6440, 0x0E20}, {0x6600, 0x6600, 0x6600, 0x6600},
  {0x06C0, 0x4620, 0x06C0, 0x4620}, {0x0E40, 0x4C40, 0x4E00, 0x4640},
  {0x0C60, 0x2640, 0x0C60, 0x2640}
};

// EEPROM Map:
// 0-1: Magic (0xAB, 0xCD)
// 2-3: Tetris HS
// 4-5: Crossy HS
// 6-7: PingPong HS
// 10: Log Head Index (0-29)
// 20-109: Logs (30 entries * 3 bytes: Day(1), Month(1), EmotionID(1))
// 120: Goal Day, 121: Goal Month, 122: Planned(LSB), 123: Done(LSB), 124: Reminder Day, 125: Planned(MSB), 126: Done(MSB)

const Msg mV[] PROGMEM = {
  // Jokes for Happy
  {PSTR("Tại sao sách toán lại buồn?"), PSTR("Why was the math book sad?"), PSTR("Vì nó có quá nhiều vấn đề."), PSTR("It had too many problems.")},
  {PSTR("Tại sao cà chua đỏ mặt?"), PSTR("Why did the tomato turn red?"), PSTR("Vì nó thấy nước sốt salad!"), PSTR("Because it saw the salad dressing!")},
  {PSTR("Cái gì có cổ mà không có đầu?"), PSTR("What has a neck but no head?"), PSTR("Cái áo."), PSTR("A shirt.")},
  {PSTR("Con gì đập thì sống, không đập thì chết?"), PSTR("What lives if you beat it, dies if you don't?"), PSTR("Con tim."), PSTR("A heart.")},
  {PSTR("Tại sao bù nhìn được giải thưởng?"), PSTR("Why did the scarecrow win an award?"), PSTR("Vì nó nổi bật trên cánh đồng!"), PSTR("He was outstanding in his field!")},
  {PSTR("Gấu không răng gọi là gì?"), PSTR("What do you call a bear with no teeth?"), PSTR("Gấu kẹo dẻo!"), PSTR("A gummy bear!")},
  {PSTR("Tại sao không nên tin nguyên tử?"), PSTR("Why don't scientists trust atoms?"), PSTR("Vì chúng bịa đặt mọi thứ!"), PSTR("Because they make up everything!")},
  {PSTR("Cái gì của bạn nhưng người khác dùng?"), PSTR("What belongs to you but others use it more?"), PSTR("Tên của bạn."), PSTR("Your name.")},
  {PSTR("Sao golf thủ mang 2 cái quần?"), PSTR("Why did the golfer bring two pairs of pants?"), PSTR("Phòng khi bị thủng một lỗ."), PSTR("In case he got a hole in one.")},
  {PSTR("Con gì càng to càng nhỏ?"), PSTR("What gets bigger the more you take away?"), PSTR("Con cua."), PSTR("A hole.")},
  {PSTR("Cái gì đen khi bạn mua nó?"), PSTR("What is black when you buy it?"), PSTR("Than."), PSTR("Charcoal.")},
  {PSTR("Hai người đi bộ vào quán bar..."), PSTR("Two guys walked into a bar..."), PSTR("Người thứ ba cúi xuống né."), PSTR("The third one ducked.")},
  {PSTR("Cái gì có nhiều chìa khóa mà không mở được cửa?"), PSTR("What has keys but can't open locks?"), PSTR("Đàn Piano."), PSTR("A piano.")},
  {PSTR("Cái gì bạn không thể giữ được lâu?"), PSTR("What can you hold but not touch?"), PSTR("Hơi thở."), PSTR("Your breath.")},
  {PSTR("Con gì có chân mà không biết đi?"), PSTR("What has legs but cannot walk?"), PSTR("Cái bàn."), PSTR("A table.")},
  {PSTR("Tại sao xe đạp lại ngã?"), PSTR("Why did the bicycle fall over?"), PSTR("Vì nó quá mệt (hai bánh)!"), PSTR("Because it was two-tired!")},
  {PSTR("Mì giả gọi là gì?"), PSTR("What do you call a fake noodle?"), PSTR("Một kẻ mạo danh!"), PSTR("An Impasta!")},
  {PSTR("Sao trứng không kể chuyện cười?"), PSTR("Why don't eggs tell jokes?"), PSTR("Chúng sẽ làm vỡ bụng nhau mất."), PSTR("They'd crack each other up.")},
  {PSTR("Khủng long đang ngủ gọi là gì?"), PSTR("What do you call a sleeping dinosaur?"), PSTR("Khủng long ngáy!"), PSTR("A dino-snore!")},
  {PSTR("Nhà máy sản xuất hàng tạm ổn gọi là gì?"), PSTR("What do you call a factory that makes okay products?"), PSTR("Một sự hài lòng."), PSTR("A satisfactory.")},
  {PSTR("Heo biết võ gọi là gì?"), PSTR("What do you call a pig that does karate?"), PSTR("Sườn cốt lết."), PSTR("A pork chop.")},
  {PSTR("Sao không thể đưa bóng bay cho Elsa?"), PSTR("Why can't you give Elsa a balloon?"), PSTR("Vì cô ấy sẽ 'Let it go'."), PSTR("Because she will let it go.")},
  {PSTR("Làm sao bắt sóc?"), PSTR("How do you catch a squirrel?"), PSTR("Leo lên cây và giả làm hạt dẻ."), PSTR("Climb a tree and act like a nut.")},
  {PSTR("Người tuyết 6 múi gọi là gì?"), PSTR("What do you call a snowman with a six-pack?"), PSTR("Người tuyết cơ bụng."), PSTR("An abdominal snowman.")},
  {PSTR("Sao máy tính đi khám bác sĩ?"), PSTR("Why did the computer go to the doctor?"), PSTR("Vì nó bị nhiễm virus."), PSTR("Because it had a virus.")},
  {PSTR("Cá không mắt gọi là gì?"), PSTR("What do you call a fish without an eye?"), PSTR("Fsh."), PSTR("Fsh.")},
  {PSTR("Tại sao bờ biển không nói gì?"), PSTR("Why did the beach say nothing?"), PSTR("Nó chỉ vẫy tay chào."), PSTR("It just waved.")},
  {PSTR("Cái gì có răng mà không cắn?"), PSTR("What has teeth but cannot bite?"), PSTR("Cái lược."), PSTR("A comb.")},
  {PSTR("Tại sao quyển lịch lại nổi tiếng?"), PSTR("Why is the calendar popular?"), PSTR("Vì nó có nhiều ngày hẹn."), PSTR("Because it has many dates.")}
};
const Msg mB[] PROGMEM = {
  // Sad - Quotes + Advice
  {PSTR("Lối thoát tốt nhất là đi xuyên qua nó."), PSTR("\"The best way out is always through.\""), PSTR("LK: Hãy tiếp tục, đừng dừng lại."), PSTR("Advice: Keep going, don't stop.")},
  {PSTR("Chuyện này rồi cũng sẽ qua."), PSTR("\"This too shall pass.\""), PSTR("LK: Kiên nhẫn là sức mạnh."), PSTR("Advice: Patience is your strength.")},
  {PSTR("Biến vết thương thành trí tuệ."), PSTR("\"Turn your wounds into wisdom.\""), PSTR("LK: Học hỏi từ nỗi đau này."), PSTR("Advice: Learn from this pain.")},
  {PSTR("Sao không thể tỏa sáng thiếu bóng tối."), PSTR("\"Stars can't shine without darkness.\""), PSTR("LK: Ánh sáng sẽ trở lại."), PSTR("Advice: Your light will return.")},
  {PSTR("Thời khắc khó khăn không kéo dài mãi."), PSTR("\"Tough times never last, but tough people do.\""), PSTR("LK: Bạn mạnh mẽ hơn bạn nghĩ."), PSTR("Advice: You are stronger than you think.")},
  {PSTR("Nỗi buồn bay đi trên đôi cánh thời gian."), PSTR("\"Sadness flies away on the wings of time.\""), PSTR("LK: Hãy cho thời gian."), PSTR("Advice: Give it time.")},
  {PSTR("Đừng khóc vì nó kết thúc, hãy cười vì nó đã xảy ra."), PSTR("\"Don't cry because it's over, smile because it happened.\""), PSTR("LK: Trân trọng kỷ niệm."), PSTR("Advice: Cherish the memories.")},
  {PSTR("Mỗi cuộc đời đều có nỗi buồn."), PSTR("\"Every life has a measure of sorrow.\""), PSTR("LK: Bạn không cô đơn."), PSTR("Advice: You are sharing the human experience.")},
  {PSTR("Hạnh phúc là hướng đi, không phải đích đến."), PSTR("Happiness is a direction, not a destination."), PSTR("LK: Tìm niềm vui trên hành trình."), PSTR("Advice: Find joy in the journey.")},
  {PSTR("Sau cơn mưa trời lại sáng."), PSTR("After the rain comes the sun."), PSTR("LK: Mọi thứ sẽ tốt đẹp hơn."), PSTR("Advice: Things will get better.")},
  {PSTR("Đừng đếm những gì bạn đã mất."), PSTR("Don't count what you have lost."), PSTR("LK: Hãy quý trọng những gì đang có."), PSTR("Advice: Cherish what you have.")},
  {PSTR("Nỗi buồn chỉ là bức tường giữa hai khu vườn."), PSTR("Sadness is but a wall between two gardens."), PSTR("LK: Hạnh phúc ở phía bên kia."), PSTR("Advice: Happiness is on the other side.")},
  {PSTR("Nước mắt đến từ tim, không phải từ não."), PSTR("Tears come from the heart, not the brain."), PSTR("LK: Để trái tim lên tiếng."), PSTR("Advice: Let your heart speak.")},
  {PSTR("Ai cũng có nỗi buồn thầm kín."), PSTR("Every man has his secret sorrows."), PSTR("LK: Hãy tử tế với mọi người."), PSTR("Advice: Be kind to everyone.")},
  {PSTR("Thời gian chữa lành tất cả."), PSTR("Time heals all wounds."), PSTR("LK: Hãy kiên nhẫn với chính mình."), PSTR("Advice: Be patient with yourself.")},
  {PSTR("Đau buồn là giá phải trả cho tình yêu."), PSTR("Grief is the price we pay for love."), PSTR("LK: Tình yêu luôn xứng đáng."), PSTR("Advice: Love is worth it.")},
  {PSTR("Trái tim nặng trĩu sẽ nhẹ hơn khi mưa xuống."), PSTR("Heavy hearts are relieved by letting water."), PSTR("LK: Hãy khóc nếu cần."), PSTR("Advice: Cry if you need to.")},
  {PSTR("Không ổn cũng không sao cả."), PSTR("It's okay not to be okay."), PSTR("LK: Chấp nhận cảm xúc của bạn."), PSTR("Advice: Accept your feelings.")},
  {PSTR("Chỉ là một ngày tệ, không phải cả đời."), PSTR("Just a bad day, not a bad life."), PSTR("LK: Ngày mai là khởi đầu mới."), PSTR("Advice: Tomorrow is new.")},
  {PSTR("Bạn không đơn độc."), PSTR("You are not alone in this."), PSTR("LK: Hãy chia sẻ với ai đó."), PSTR("Advice: Reach out to someone.")},
  {PSTR("Để nó đau, rồi để nó đi."), PSTR("Let it hurt, then let it go."), PSTR("LK: Buông bỏ nỗi đau."), PSTR("Advice: Release the pain.")},
  {PSTR("Đừng sợ bóng tối, đó là nơi sao sáng nhất."), PSTR("Don't fear dark, stars shine there."), PSTR("LK: Tìm ánh sáng của bạn."), PSTR("Advice: Find your light.")},
  {PSTR("Mọi kết thúc là một khởi đầu mới."), PSTR("Every end is a new beginning."), PSTR("LK: Hãy bắt đầu lại."), PSTR("Advice: Start again.")},
  {PSTR("Bạn đủ mạnh mẽ để vượt qua."), PSTR("You are strong enough to get through."), PSTR("LK: Tin vào sức mạnh của bạn."), PSTR("Advice: Trust your strength.")}
};
const Msg mK[] PROGMEM = {
  // Crying - Quotes + Advice
  {PSTR("Nước mắt là những lời cần được viết ra."), PSTR("\"Tears are words that need to be written.\""), PSTR("LK: Hãy bày tỏ cảm xúc của bạn."), PSTR("Advice: Express your feelings.")},
  {PSTR("Đừng xin lỗi vì đã khóc."), PSTR("\"Do not apologize for crying.\""), PSTR("LK: Khóc là điều rất con người."), PSTR("Advice: It's okay to be human.")},
  {PSTR("Khóc là sự rửa sạch tâm hồn."), PSTR("\"Crying is cleansing.\""), PSTR("LK: Hãy xả hết nỗi đau."), PSTR("Advice: Let it all out.")},
  {PSTR("Nước mắt đến từ trái tim."), PSTR("\"Tears come from the heart.\""), PSTR("LK: Hãy cảm nhận cảm xúc."), PSTR("Advice: Feel your emotions fully.")},
  {PSTR("Khóc không phải là yếu đuối."), PSTR("Crying is not weakness."), PSTR("LK: Đó là dấu hiệu bạn đã mạnh mẽ quá lâu."), PSTR("Advice: Sign you've been strong too long.")},
  {PSTR("Nước mắt là ngôn ngữ câm lặng của nỗi đau."), PSTR("\"Tears are the silent language of grief.\""), PSTR("LK: Hãy để chúng lên tiếng."), PSTR("Advice: Let them speak.")},
  {PSTR("Để trái tim nhẹ nhõm hơn."), PSTR("To make the heart lighter."), PSTR("LK: Khóc sẽ giúp bạn thấy khá hơn."), PSTR("Advice: You will feel lighter.")},
  {PSTR("Nước mắt vô hình là khó lau nhất."), PSTR("Invisible tears are the hardest to wipe."), PSTR("LK: Hãy chia sẻ với ai đó."), PSTR("Advice: Share your pain with someone.")},
  {PSTR("Sau cơn mưa, cầu vồng sẽ xuất hiện."), PSTR("Rainbows appear after the rain."), PSTR("LK: Niềm vui sẽ trở lại."), PSTR("Advice: Joy will return.")},
  {PSTR("Nước mắt là lời trái tim không thể nói."), PSTR("Tears are words the heart can't say."), PSTR("LK: Để chúng lên tiếng."), PSTR("Advice: Let them speak.")},
  {PSTR("Khóc là cách mắt nói khi miệng không thể."), PSTR("Crying is how eyes speak when mouth can't."), PSTR("LK: Yếu đuối cũng không sao."), PSTR("Advice: It's okay to break.")},
  {PSTR("Ai không biết khóc sẽ không biết cười."), PSTR("Who doesn't weep, cannot laugh."), PSTR("LK: Cảm nhận sâu sắc."), PSTR("Advice: Feel deeply.")},
  {PSTR("Nước mắt là van an toàn của trái tim."), PSTR("Tears are the safety valve of the heart."), PSTR("LK: Giải tỏa áp lực."), PSTR("Advice: Release the pressure.")},
  {PSTR("Xà phòng cho cơ thể, nước mắt cho tâm hồn."), PSTR("Soap for body, tears for soul."), PSTR("LK: Rửa sạch tâm hồn."), PSTR("Advice: Cleanse your soul.")},
  {PSTR("Khóc hết nước mắt để nhường chỗ cho nụ cười."), PSTR("Cry out tears to make room for smiles."), PSTR("LK: Nụ cười sẽ đến."), PSTR("Advice: Smiles will come.")},
  {PSTR("Thuốc chữa lành là nước mặn: mồ hôi, nước mắt."), PSTR("Cure is salt water: sweat, tears, sea."), PSTR("LK: Để nó chữa lành bạn."), PSTR("Advice: Let it heal you.")},
  {PSTR("Khóc. Tha thứ. Học hỏi. Tiếp tục."), PSTR("Cry. Forgive. Learn. Move on."), PSTR("LK: Con đường phía trước."), PSTR("Advice: The path forward.")},
  {PSTR("Nước mắt chỉ là sự giải tỏa tạm thời."), PSTR("Your tears are just a temporary release."), PSTR("LK: Bình yên sẽ theo sau."), PSTR("Advice: Peace will follow.")},
  {PSTR("Ngay cả bầu trời cũng có lúc khóc."), PSTR("Even the sky cries sometimes."), PSTR("LK: Đó là lẽ tự nhiên."), PSTR("Advice: It's natural.")},
  {PSTR("Để nước mắt tưới tẩm hạt giống hạnh phúc."), PSTR("Let tears water seeds of happiness."), PSTR("LK: Lớn lên từ nỗi buồn."), PSTR("Advice: Grow from sadness.")},
  {PSTR("Một trận khóc làm nhẹ lòng."), PSTR("A good cry lightens the heart."), PSTR("LK: Cảm nhận sự nhẹ nhõm."), PSTR("Advice: Feel the relief.")},
  {PSTR("Đừng giấu nước mắt."), PSTR("Don't hide your tears."), PSTR("LK: Sống thật với cảm xúc."), PSTR("Advice: Be true to feelings.")}
};
const Msg mTv[] PROGMEM = {
  // Despair - Quotes + Advice
  {PSTR("Trời tối nhất là trước khi bình minh."), PSTR("\"It is always darkest just before dawn.\""), PSTR("LK: Hy vọng đang ở rất gần."), PSTR("Advice: Hope is near.")},
  {PSTR("Ngã 7 lần, đứng dậy 8 lần."), PSTR("\"Fall seven times, stand up eight.\""), PSTR("LK: Kiên trì là chìa khóa."), PSTR("Advice: Persistence is key.")},
  {PSTR("Khi muốn bỏ cuộc, nhớ lý do bắt đầu."), PSTR("When you want to quit, remember why you started."), PSTR("LK: Đừng bỏ cuộc ngay lúc này."), PSTR("Advice: Don't give up yet.")},
  {PSTR("Vinh quang là đứng dậy sau mỗi lần ngã."), PSTR("Glory is rising every time we fall."), PSTR("LK: Hãy đứng dậy lần nữa."), PSTR("Advice: Rise again.")},
  {PSTR("Tin rằng bạn có thể là bạn đã thành công một nửa."), PSTR("\"Believe you can and you're halfway there.\""), PSTR("LK: Hãy tin vào chính mình."), PSTR("Advice: Trust yourself.")},
  {PSTR("Hy vọng là ánh sáng trong bóng tối."), PSTR("Hope is the light in darkness."), PSTR("LK: Hãy tìm kiếm ánh sáng đó."), PSTR("Advice: Look for the light.")},
  {PSTR("Đáy vực là nền tảng để xây dựng lại."), PSTR("Rock bottom is the foundation to rebuild."), PSTR("LK: Xây dựng lại từ đây."), PSTR("Advice: Build up from here.")},
  {PSTR("Chậm cũng được, miễn là đừng dừng lại."), PSTR("Slow is fine, as long as you don't stop."), PSTR("LK: Tiếp tục tiến bước."), PSTR("Advice: Keep moving forward.")},
  {PSTR("Không bao giờ là quá muộn."), PSTR("It is never too late."), PSTR("LK: Bạn luôn có thể bắt đầu lại."), PSTR("Advice: You can always restart.")},
  {PSTR("Trong cái khó ló cái khôn."), PSTR("Necessity is the mother of invention."), PSTR("LK: Tìm giải pháp mới."), PSTR("Advice: Find a new solution.")},
  {PSTR("Khi ở cuối sợi dây, hãy thắt nút và bám chặt."), PSTR("When at end of rope, tie a knot and hang on."), PSTR("LK: Đừng buông tay."), PSTR("Advice: Don't let go.")},
  {PSTR("Chấp nhận thất vọng, đừng mất hy vọng."), PSTR("Accept disappointment, never lose hope."), PSTR("LK: Giữ vững hy vọng."), PSTR("Advice: Keep hoping.")},
  {PSTR("Chỉ trong bóng tối mới thấy được sao."), PSTR("Only in darkness can you see stars."), PSTR("LK: Hãy nhìn lên."), PSTR("Advice: Look up.")},
  {PSTR("Không có tuyệt vọng nào là mãi mãi."), PSTR("No despair is forever."), PSTR("LK: Bạn sẽ vượt qua."), PSTR("Advice: You will survive.")},
  {PSTR("Hành động là thuốc giải cho tuyệt vọng."), PSTR("Action is the antidote to despair."), PSTR("LK: Làm việc nhỏ gì đó."), PSTR("Advice: Do something small.")},
  {PSTR("Từ khó khăn phép màu sẽ đến."), PSTR("Out of difficulties grow miracles."), PSTR("LK: Chờ đợi phép màu."), PSTR("Advice: Wait for it.")},
  {PSTR("Can đảm là đi tiếp khi không còn sức."), PSTR("Courage is going on without strength."), PSTR("LK: Tiếp tục đi."), PSTR("Advice: Keep going.")},
  {PSTR("Giờ đen tối nhất cũng chỉ có 60 phút."), PSTR("Darkest hour has only 60 minutes."), PSTR("LK: Nó sẽ qua."), PSTR("Advice: It will pass.")},
  {PSTR("Khi mặt trời lặn, sao sẽ mọc."), PSTR("When sun goes down, stars come out."), PSTR("LK: Tìm những vì sao."), PSTR("Advice: Look for stars.")},
  {PSTR("Mỗi bức tường đều là một cánh cửa."), PSTR("Every wall is a door."), PSTR("LK: Tìm tay nắm cửa."), PSTR("Advice: Find the handle.")},
  {PSTR("Bạn mạnh mẽ hơn bạn biết."), PSTR("You are stronger than you know."), PSTR("LK: Tin vào chính mình."), PSTR("Advice: Trust yourself.")},
  {PSTR("Đừng để hôm qua chiếm quá nhiều hôm nay."), PSTR("Don't let yesterday take up too much of today."), PSTR("LK: Sống cho hiện tại."), PSTR("Advice: Live for today.")},
  {PSTR("Khó khăn sinh ra người mạnh mẽ."), PSTR("Hard times create strong men."), PSTR("LK: Bạn đang mạnh mẽ lên."), PSTR("Advice: You are growing strong.")},
  {PSTR("Con đường dài nhất bắt đầu bằng một bước."), PSTR("Longest journey starts with a step."), PSTR("LK: Bước bước đầu tiên."), PSTR("Advice: Take the first step.")}
};
const Msg mTg[] PROGMEM = {
  // Angry - Quotes + Advice
  {PSTR("Mỗi phút tức giận mất 60s hạnh phúc."), PSTR("Every minute angry loses 60s of happiness."), PSTR("LK: Chọn hạnh phúc thay vì giận dữ."), PSTR("Advice: Choose happiness instead.")},
  {PSTR("Giận quá mất khôn."), PSTR("Anger is one letter short of danger."), PSTR("LK: Hạ hỏa trước khi hành động."), PSTR("Advice: Cool down before you act.")},
  {PSTR("Lời nói khi giận là lời hối tiếc nhất."), PSTR("Angry words are the most regretted."), PSTR("LK: Im lặng là vàng."), PSTR("Advice: Silence is golden now.")},
  {PSTR("Giữ giận dữ như uống thuốc độc."), PSTR("Holding anger is like drinking poison."), PSTR("LK: Hãy buông bỏ nó."), PSTR("Advice: Let it go.")},
  {PSTR("Người chiến binh giỏi nhất không bao giờ giận."), PSTR("\"The best fighter is never angry.\""), PSTR("LK: Bình tĩnh để chiến thắng."), PSTR("Advice: Stay calm to win.")},
  {PSTR("Giận dữ làm hại bạn nhiều hơn."), PSTR("Anger hurts you more than others."), PSTR("LK: Đừng tự làm đau mình."), PSTR("Advice: Don't hurt yourself.")},
  {PSTR("Khi giận dữ, hãy im lặng."), PSTR("When you are angry, be silent."), PSTR("LK: Hít thở thật sâu."), PSTR("Advice: Take a deep breath.")},
  {PSTR("Ai làm bạn giận, kẻ đó điều khiển bạn."), PSTR("He who angers you conquers you."), PSTR("LK: Lấy lại quyền kiểm soát."), PSTR("Advice: Reclaim your power.")},
  {PSTR("Một phút kiên nhẫn tránh trăm ngày hối tiếc."), PSTR("Patience saves 100 days of regret."), PSTR("LK: Hãy kiên nhẫn."), PSTR("Advice: Be patient now.")},
  {PSTR("Tha thứ là vindicta tốt nhất."), PSTR("Forgiveness is the best revenge."), PSTR("LK: Tha thứ để bình yên."), PSTR("Advice: Forgive to find peace.")},
  {PSTR("Giận dữ là axit ăn mòn vật chứa nó."), PSTR("Anger is acid that harms the vessel."), PSTR("LK: Đừng giữ nó."), PSTR("Advice: Don't hold it.")},
  {PSTR("Giữ cục than hồng để ném người khác, bạn sẽ bỏng."), PSTR("Holding hot coal burns you."), PSTR("LK: Buông cục than xuống."), PSTR("Advice: Drop the coal.")},
  {PSTR("Giận dữ chỉ ở trong lòng kẻ khờ."), PSTR("Anger dwells in the bosom of fools."), PSTR("LK: Hãy khôn ngoan."), PSTR("Advice: Be wise.")},
  {PSTR("Khi giận, đếm đến 10."), PSTR("When angry, count to ten."), PSTR("LK: Đếm ngay đi."), PSTR("Advice: Count now.")},
  {PSTR("Giận dữ thổi tắt ngọn đèn trí tuệ."), PSTR("Anger blows out the lamp of the mind."), PSTR("LK: Bảo vệ ánh sáng."), PSTR("Advice: Protect your light.")},
  {PSTR("Không ai làm bạn giận nếu bạn không cho phép."), PSTR("No one angers you without consent."), PSTR("LK: Đừng cho phép."), PSTR("Advice: Don't consent.")},
  {PSTR("Cay đắng như ung thư, nó ăn mòn vật chủ."), PSTR("Bitterness eats upon the host."), PSTR("LK: Buông bỏ đi."), PSTR("Advice: Let it go.")},
  {PSTR("Thuốc chữa giận dữ là sự trì hoãn."), PSTR("Remedy for anger is delay."), PSTR("LK: Đợi một chút."), PSTR("Advice: Wait a while.")},
  {PSTR("Đừng đi ngủ khi đang giận."), PSTR("Don't go to bed angry."), PSTR("LK: Giải quyết nó."), PSTR("Advice: Resolve it.")},
  {PSTR("Tha thứ giúp bạn lớn lên."), PSTR("Forgiveness forces you to grow."), PSTR("LK: Tha thứ."), PSTR("Advice: Forgive.")},
  {PSTR("Trả thù là thú nhận nỗi đau."), PSTR("Revenge is admitting pain."), PSTR("LK: Chữa lành nỗi đau."), PSTR("Advice: Heal the pain.")},
  {PSTR("Giận dữ là kẻ thù của hòa bình."), PSTR("Anger is the enemy of peace."), PSTR("LK: Tìm sự bình yên."), PSTR("Advice: Find peace.")},
  {PSTR("Đừng để cảm xúc chi phối hành động."), PSTR("Don't let emotions rule actions."), PSTR("LK: Suy nghĩ trước."), PSTR("Advice: Think first.")}
};
const Msg mS[] PROGMEM = {
  // Fear - Quotes + Advice
  {PSTR("Thứ duy nhất đáng sợ là nỗi sợ."), PSTR("\"The only thing to fear is fear itself.\""), PSTR("LK: Sợ hãi chỉ là cảm giác."), PSTR("Advice: Fear is just a feeling.")},
  {PSTR("Sợ hãi là phản xạ. Can đảm là lựa chọn."), PSTR("Fear is reaction. Courage is decision."), PSTR("LK: Hãy chọn sự dũng cảm."), PSTR("Advice: Choose to be brave.")},
  {PSTR("Làm một việc khiến bạn sợ mỗi ngày."), PSTR("Do one thing every day that scares you."), PSTR("LK: Bước ra khỏi vùng an toàn."), PSTR("Advice: Step out of comfort zone.")},
  {PSTR("Mọi thứ bạn muốn ở bên kia nỗi sợ."), PSTR("Everything you want is on the other side of fear."), PSTR("LK: Hãy tiến tới và lấy nó."), PSTR("Advice: Go get it.")},
  {PSTR("Can đảm là chiến thắng nỗi sợ."), PSTR("Courage is triumph over fear."), PSTR("LK: Bạn có thể chiến thắng."), PSTR("Advice: You can triumph.")},
  {PSTR("Đối mặt với mọi thứ và vươn lên."), PSTR("Face Everything And Rise."), PSTR("LK: Sự lựa chọn là của bạn."), PSTR("Advice: The choice is yours.")},
  {PSTR("Sợ hãi là cảm giác. Dũng cảm là hành động."), PSTR("Scared is feeling. Brave is doing."), PSTR("LK: Tiếp tục hành động."), PSTR("Advice: Keep doing it.")},
  {PSTR("Đừng sợ thất bại, hãy sợ không thử."), PSTR("Don't fear failure, fear not trying."), PSTR("LK: Hãy thử sức mình."), PSTR("Advice: Take the chance.")},
  {PSTR("Sợ hãi là tạm thời. Hối tiếc là mãi mãi."), PSTR("Fear is temporary. Regret is forever."), PSTR("LK: Đừng để nỗi sợ ngăn cản."), PSTR("Advice: Don't let fear stop you.")},
  {PSTR("Bạn mạnh mẽ hơn nỗi sợ của mình."), PSTR("You are stronger than your fear."), PSTR("LK: Tin vào sức mạnh của bạn."), PSTR("Advice: Trust your strength.")},
  {PSTR("Can đảm là kháng cự, làm chủ nỗi sợ."), PSTR("Courage is mastery of fear."), PSTR("LK: Làm chủ nó."), PSTR("Advice: Master it.")},
  {PSTR("Đừng để nỗi sợ thất bại ngăn bạn chơi."), PSTR("Don't let fear keep you from playing."), PSTR("LK: Chơi hết mình."), PSTR("Advice: Play the game.")},
  {PSTR("Nhiều người không sống với ước mơ vì sợ."), PSTR("Many don't live dreams because of fear."), PSTR("LK: Sống với ước mơ."), PSTR("Advice: Live your dream.")},
  {PSTR("Nỗi sợ cắt sâu hơn gươm giáo."), PSTR("Fear cuts deeper than swords."), PSTR("LK: Đừng để nó làm tổn thương."), PSTR("Advice: Don't let it cut.")},
  {PSTR("Đối diện nỗi sợ, nó sẽ mất quyền lực."), PSTR("Expose fear, it loses power."), PSTR("LK: Vạch trần nó."), PSTR("Advice: Expose it.")},
  {PSTR("Chinh phục nỗi sợ mỗi ngày."), PSTR("Conquer some fear every day."), PSTR("LK: Chinh phục hôm nay."), PSTR("Advice: Conquer today.")},
  {PSTR("Nỗi sợ chỉ sâu đến mức tâm trí cho phép."), PSTR("Fear is only as deep as mind allows."), PSTR("LK: Kiểm soát tâm trí."), PSTR("Advice: Control your mind.")},
  {PSTR("Cảm thấy sợ nhưng vẫn cứ làm."), PSTR("Feel the fear and do it anyway."), PSTR("LK: Cứ làm đi."), PSTR("Advice: Do it.")},
  {PSTR("Đừng sợ bóng tối, hãy thắp nến."), PSTR("Don't fear dark, light a candle."), PSTR("LK: Thắp sáng hy vọng."), PSTR("Advice: Light hope.")},
  {PSTR("Sợ hãi giết chết nhiều ước mơ hơn thất bại."), PSTR("Fear kills more dreams than failure."), PSTR("LK: Đừng để nó giết ước mơ."), PSTR("Advice: Save your dreams.")},
  {PSTR("Hành động chữa lành nỗi sợ."), PSTR("Action cures fear."), PSTR("LK: Hành động ngay."), PSTR("Advice: Act now.")},
  {PSTR("Bạn an toàn, hãy tin tưởng."), PSTR("You are safe, trust the process."), PSTR("LK: Mọi thứ sẽ ổn."), PSTR("Advice: Everything will be ok.")}
};

const Msg mLA[] PROGMEM = {
  // Anxious - Quotes + Advice
  {PSTR("Lo lắng không làm hết muộn phiền ngày mai."), PSTR("Worrying doesn't empty tomorrow's sorrow."), PSTR("LK: Nó làm hết sức lực hôm nay."), PSTR("Advice: It empties today's strength.")},
  {PSTR("Hít thở sâu. Mọi chuyện sẽ ổn thôi."), PSTR("Take a deep breath. It will be okay."), PSTR("LK: Bạn đang làm tốt mà."), PSTR("Advice: You are doing fine.")},
  {PSTR("Bạn không cần phải kiểm soát mọi thứ."), PSTR("You don't have to control everything."), PSTR("LK: Hãy buông bỏ những thứ không thể."), PSTR("Advice: Let go of what you can't.")},
  {PSTR("Suy nghĩ quá nhiều là kẻ thù của hạnh phúc."), PSTR("Overthinking is the enemy of happiness."), PSTR("LK: Đừng suy nghĩ nữa, hãy làm đi."), PSTR("Advice: Stop thinking, just do.")},
  {PSTR("Đừng lo lắng về những điều chưa xảy ra."), PSTR("Don't worry about what hasn't happened."), PSTR("LK: Sống cho hiện tại."), PSTR("Advice: Live in the moment.")},
  {PSTR("Bình tĩnh là một siêu năng lực."), PSTR("Calm is a super power."), PSTR("LK: Hãy giữ cái đầu lạnh."), PSTR("Advice: Keep a cool head.")},
  {PSTR("Cảm giác này chỉ là tạm thời."), PSTR("This feeling is temporary."), PSTR("LK: Nó sẽ qua đi."), PSTR("Advice: It will pass.")},
  {PSTR("Bạn đã đủ tốt rồi."), PSTR("You are enough."), PSTR("LK: Đừng tự gây áp lực."), PSTR("Advice: Don't pressure yourself.")},
  {PSTR("Tập trung vào điều bạn kiểm soát được."), PSTR("Focus on what you can control."), PSTR("LK: Buông bỏ phần còn lại."), PSTR("Advice: Let go of the rest.")},
  {PSTR("Mỗi lần một bước thôi."), PSTR("One step at a time."), PSTR("LK: Đừng vội vàng."), PSTR("Advice: Don't rush.")},
  {PSTR("Hít vào bình yên, thở ra áp lực."), PSTR("Breathe in peace, breathe out stress."), PSTR("LK: Thư giãn cơ thể."), PSTR("Advice: Relax your body.")},
  {PSTR("Bạn đã vượt qua 100% ngày tồi tệ."), PSTR("You survived 100% of bad days."), PSTR("LK: Bạn rất mạnh mẽ."), PSTR("Advice: You are strong.")},
  {PSTR("Đừng tin mọi điều bạn nghĩ."), PSTR("Don't believe everything you think."), PSTR("LK: Suy nghĩ không phải sự thật."), PSTR("Advice: Thoughts are not facts.")},
  {PSTR("Hiện tại là tất cả bạn có."), PSTR("The present is all you have."), PSTR("LK: Sống cho hiện tại."), PSTR("Advice: Live in the now.")},
  {PSTR("Lo âu là kẻ nói dối."), PSTR("Anxiety is a liar."), PSTR("LK: Đừng tin nó."), PSTR("Advice: Don't believe it.")},
  {PSTR("Bước nhỏ vẫn là tiến bộ."), PSTR("Small steps are still progress."), PSTR("LK: Cứ tiếp tục đi."), PSTR("Advice: Keep going.")},
  {PSTR("Đây không phải tận thế."), PSTR("It's not the end of the world."), PSTR("LK: Mọi thứ sẽ ổn."), PSTR("Advice: Everything will be ok.")},
  {PSTR("Tâm trí bạn là một khu vườn."), PSTR("Your mind is a garden."), PSTR("LK: Gieo hạt giống tích cực."), PSTR("Advice: Plant positive seeds.")},
  {PSTR("Buông bỏ nhu cầu phải chắc chắn."), PSTR("Let go of need for certainty."), PSTR("LK: Chấp nhận sự bất định."), PSTR("Advice: Accept uncertainty.")},
  {PSTR("Bạn đang an toàn ngay lúc này."), PSTR("You are safe right now."), PSTR("LK: Nhìn xung quanh xem."), PSTR("Advice: Look around you.")},
  {PSTR("Lỗi lầm là bằng chứng nỗ lực."), PSTR("Mistakes are proof of trying."), PSTR("LK: Đừng sợ sai."), PSTR("Advice: Don't fear mistakes.")},
  {PSTR("Hãy tử tế với chính mình."), PSTR("Be kind to yourself."), PSTR("LK: Bạn xứng đáng được yêu thương."), PSTR("Advice: You deserve love.")},
  {PSTR("Bão tố không kéo dài mãi."), PSTR("Storms don't last forever."), PSTR("LK: Mặt trời sẽ lên."), PSTR("Advice: The sun will rise.")},
  {PSTR("Tin vào thời điểm của cuộc đời."), PSTR("Trust the timing of your life."), PSTR("LK: Mọi thứ xảy ra đúng lúc."), PSTR("Advice: Things happen on time.")},
  {PSTR("Nhỡ đâu mọi thứ tốt đẹp thì sao?"), PSTR("What if it all works out?"), PSTR("LK: Nghĩ về điều tích cực."), PSTR("Advice: Think positive.")}
};

const PROGMEM void* const messageGroups[] = { mV, mB, mK, mTv, mTg, mS, mLA };
const int messageGroupCounts[] = {
  sizeof(mV) / sizeof(Msg), sizeof(mB) / sizeof(Msg), sizeof(mK) / sizeof(Msg),
  sizeof(mTv) / sizeof(Msg), sizeof(mTg) / sizeof(Msg), sizeof(mS) / sizeof(Msg), sizeof(mLA) / sizeof(Msg)
};

void beep(int f, int d) { if (!soundOff) tone(BZ, f, d); }
void playGameOver() {
  // 8-bit style descending "lose" sound
  for (int f = 350; f > 150; f -= 30) {
    beep(f, 40);
    delay(40);
  }
  mIdx = 0;
}

void initGame() {
  gamePlaying = false;
  if(gameChoice == 0) { st=TETRIS; sc=0; py=0; px=4; pT=random(7); memset(grid, 0, sizeof(grid)); highScore=getHighScore(0); }
  else if(gameChoice == 1) { st=CROSSY; sc=0; ckX=64; ckY=58; carsX[0]=0; carsX[1]=40; carsX[2]=80; carsX[3]=20; carSpd[0]=1.0; carSpd[1]=-1.2; carSpd[2]=1.5; carSpd[3]=-1.0; highScore=getHighScore(1); }
  else if(gameChoice == 2) { st=PINGPONG; sc=0; bX=64; bY=32; bDX=1.8; bDY=1.8; p1Y=22; highScore=getHighScore(2); }
  else if(gameChoice == 3) { st=SNAKE; sc=0; snLen=3; snX[0]=5; snY[0]=5; snX[1]=4; snY[1]=5; snX[2]=3; snY[2]=5; snDirX=1; snDirY=0; apX=10; apY=8; highScore=getHighScore(3); }
  else if(gameChoice == 4) { st=DINO; sc=0; dY=55; dV=0; obsX=128; obsW=6; obsH=10; highScore=getHighScore(4); }
  else if(gameChoice == 5) { st=FLAPPY; sc=0; fbY=32; fbV=0; pipeX=SW; pipeH=random(10, SH-pipeGap-10); highScore=getHighScore(5); }
  lastGame = st;
}

int getHSOffset(int gameIdx) {
  if (gameIdx == 4) return 110; // Dino HS
  if (gameIdx == 5) return 112; // Flappy Bird HS
  return 2 + (gameIdx * 2);
}

void saveHighScore(int gameIdx, int score) {
  int addr = getHSOffset(gameIdx);
  int currentHS = (EEPROM.read(addr) << 8) | EEPROM.read(addr + 1);
  if (score > currentHS) {
    EEPROM.write(addr, (score >> 8) & 0xFF);
    EEPROM.write(addr + 1, score & 0xFF);
    EEPROM.commit();
  }
}

int getHighScore(int gameIdx) {
  int addr = getHSOffset(gameIdx);
  return (EEPROM.read(addr) << 8) | EEPROM.read(addr + 1);
}

void logEmotion(int emoIdx) {
  DateTime now = rtc.now();
  int head = EEPROM.read(10);
  if (head >= 30) head = 0;
  
  // Removed overwrite logic to record every instance

  int addr = 20 + (head * 3);
  EEPROM.write(addr, now.day());
  EEPROM.write(addr + 1, now.month());
  EEPROM.write(addr + 2, emoIdx);
  
  head = (head + 1) % 30;
  EEPROM.write(10, head);
  EEPROM.commit();
}

void setup() {
  Wire.begin(5, 6);
  pinMode(PIN_UP, INPUT_PULLUP); pinMode(PIN_DOWN, INPUT_PULLUP);
  pinMode(PIN_LEFT, INPUT_PULLUP); pinMode(PIN_RIGHT, INPUT_PULLUP); pinMode(PIN_OK, INPUT_PULLUP);
  EEPROM.begin(EEPROM_SIZE);
  if (!rtc.begin()) { /* Handle error if needed */ }
  
  // Init EEPROM if new
  if (EEPROM.read(0) != 0xAB || EEPROM.read(1) != 0xCD) {
    for (int i = 0; i < EEPROM_SIZE; i++) EEPROM.write(i, 0);
    EEPROM.write(0, 0xAB); EEPROM.write(1, 0xCD);
    EEPROM.commit();
  }
  soundOff = EEPROM.read(8);
  language = EEPROM.read(9); // Load language
  // Sanitize Log Head Index to prevent crashes
  if (EEPROM.read(10) >= 30) {
    EEPROM.write(10, 0);
    EEPROM.commit();
  }

  display.begin(0x3C, true);
  display.setTextColor(SH110X_WHITE);
  display.setTextWrap(false);
  targetX = 0; currentX = 0;

  // Thiet lap chong doi (debounce) cho tat ca cac nut
  nL.setDebounceTime(0);
  nX.setDebounceTime(0);
  nTr.setDebounceTime(0);
  nP.setDebounceTime(0);
  nOk.setDebounceTime(0);
  myfont.set_font(VietFont);

  // Splash Screen
  display.clearDisplay();
  display.setTextSize(1);
  int16_t x1, y1; uint16_t w, h;
  const char* line1 = "EMOTIVATE";
  const char* line2 = "Made by Hoang Logg";
  
  display.getTextBounds(line1, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SW - w) / 2, 20);
  display.print(line1);
  
  display.getTextBounds(line2, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SW - w) / 2, 35);
  display.print(line2);
  
  display.display();
  delay(1000); // Doi 1 giay (Fake loading)

  if (language == 0) {
     const char* msg = "Nhấn OK để tiếp";
     myfont.print((SW - myfont.getLength((char*)msg)) / 2, 52, (char*)msg, SH110X_WHITE);
  } else {
     const char* msg = "PRESS OK TO START";
     display.getTextBounds(msg, 0, 0, &x1, &y1, &w, &h);
     display.setCursor((SW - w) / 2, 52);
     display.print(msg);
  }
  display.display();

  while (true) {
    nOk.loop();
    if (nOk.isPressed()) break;
    delay(10);
  }
  beep(1000, 50);
}

void printWrappedVI(int x, int y, const char* text) {
  char buf[256];
  strncpy(buf, text, 255);
  buf[255] = 0;
  
  int cursorX = x;
  int cursorY = y;
  int spaceW = myfont.getLength((char*)" ");
  if (spaceW == 0) spaceW = 4; // Đảm bảo khoảng cách là 4px nếu font không định nghĩa
  int lineH = 10;

  char* start = buf;
  char* end = buf;
  
  while (*start) {
    end = strchr(start, ' ');
    bool lastWord = (end == NULL);
    if (lastWord) end = start + strlen(start);
    
    char saved = *end;
    *end = 0;
    int w = myfont.getLength(start);
    
    if (cursorX + w > SW) {
      cursorX = 0;
      cursorY += lineH;
    }
    
    if (cursorY > 45) break; // Ngăn tràn màn hình xuống dòng thông báo
    
    myfont.print(cursorX, cursorY, start, SH110X_WHITE);
    cursorX += w + spaceW;
    
    *end = saved;
    if (lastWord) break;
    start = end + 1;
  }
}

void drawPauseMenu() {
  display.clearDisplay();
  // Canh giữa hộp thoại và nội dung bên trong
  const int boxW = 100, boxH = 56;
  const int boxX = (SW - boxW) / 2;
  const int boxY = (SH - boxH) / 2;
  display.drawRect(boxX, boxY, boxW, boxH, SH110X_WHITE);
  display.fillRect(boxX + 1, boxY + 1, boxW - 2, boxH - 2, SH110X_BLACK);
  
  const char* title = (language == 0) ? "TẠM DỪNG" : "PAUSE";
  const char* opt0 = (language == 0) ? "TIẾP TỤC" : "RESUME";
  const char* opt1 = (language == 0) ? "CÀI ĐẶT" : "SETTINGS";
  const char* opt2 = (language == 0) ? "THOÁT" : "EXIT";

  if (language == 0) {
    myfont.print(boxX + (boxW - myfont.getLength((char*)title)) / 2, boxY + 6, (char*)title, SH110X_WHITE);
    
    int optX = boxX + 10;
    int optY = boxY + 16;
    
    myfont.print(optX, optY, (char*)(pauseOpt == 0 ? "> " : "  "), SH110X_WHITE);
    myfont.print(optX + myfont.getLength((char*)(pauseOpt == 0 ? "> " : "  ")), optY, (char*)opt0, SH110X_WHITE);
    
    myfont.print(optX, optY + 12, (char*)(pauseOpt == 1 ? "> " : "  "), SH110X_WHITE);
    myfont.print(optX + myfont.getLength((char*)(pauseOpt == 1 ? "> " : "  ")), optY + 12, (char*)opt1, SH110X_WHITE);
    
    myfont.print(optX, optY + 24, (char*)(pauseOpt == 2 ? "> " : "  "), SH110X_WHITE);
    myfont.print(optX + myfont.getLength((char*)(pauseOpt == 2 ? "> " : "  ")), optY + 24, (char*)opt2, SH110X_WHITE);
  } else {
    display.setCursor(boxX + (boxW - strlen(title) * 6) / 2, boxY + 6); display.print(title);
    display.setCursor(boxX + 12, boxY + 16); display.print(pauseOpt == 0 ? "> " : "  "); display.print(opt0);
    display.setCursor(boxX + 12, boxY + 28); display.print(pauseOpt == 1 ? "> " : "  "); display.print(opt1);
    display.setCursor(boxX + 12, boxY + 40); display.print(pauseOpt == 2 ? "> " : "  "); display.print(opt2);
  }
  display.display();
}

bool checkC(int nx, int ny, int nr) {
  for (int i = 0; i < 16; i++) if (shapes[pT][nr] & (1 << (15 - i))) {
    int tx = nx + (i % 4), ty = ny + (i / 4);
    if (tx < 0 || tx >= 10 || ty >= 20) return true;
    if (ty >= 0 && grid[tx][ty]) return true;
  }
  return false;
}

void loop() {
  nL.loop(); nX.loop(); nTr.loop(); nP.loop(); nOk.loop();
  DateTime now = rtc.now();
  
  display.setTextSize(1); // Ensure text size is reset every loop

  // Global Timer Check (Chay ngam o moi man hinh)
  if (tmRun && (millis() - tmStart >= tmDuration)) {
    tmRun = false;
    st = TIMER_ALERT;
    alertOkCount = 0;
  }

  // Daily Goal Reset Check
  if (now.day() != EEPROM.read(120) || now.month() != EEPROM.read(121)) {
    EEPROM.write(120, now.day());
    EEPROM.write(121, now.month());
    EEPROM.write(122, 0); // Reset planned LSB
    EEPROM.write(123, 0); // Reset done LSB
    EEPROM.write(125, 0); // Reset planned MSB
    EEPROM.write(126, 0); // Reset done MSB
    EEPROM.commit();
  }

  if (st == E_SEL) {
    static unsigned long holdStart = 0; // For hold-to-select

    // At 8 PM, ask user to review goals if not already asked today.
    if (now.hour() >= 20 && EEPROM.read(124) != now.day()) {
        st = GOAL_REMINDER;
        EEPROM.write(124, now.day());
        EEPROM.commit();
        // Skip the rest of E_SEL drawing for this frame
        return; 
    }

    // Animation: Truot muot ma
    if (abs(targetX - currentX) > 0.1) currentX += (targetX - currentX) * 0.3;
    else currentX = targetX;

    display.clearDisplay();
    display.setTextSize(1); display.setCursor(0, 0); 
    display.print(now.hour()); display.print(":"); if(now.minute()<10) display.print("0"); display.print(now.minute());
    display.setCursor(35, 0); display.print(" EMOTIVATE");
    
    // Added GAME and SET to the menu
    String ic[] = {":)", ":(", "T_T", "X_X", ">:<", "O_O", "O_o", "GAME", "TOOL", "GOAL", "LIST", "SET", "RESET"};
    String nmVI[] = {"Vui", "Buồn", "Khóc", "Tuyệt vọng", "Giận", "Sợ hãi", "Lo âu", "Giải trí", "Tiện ích", "Mục tiêu", "Tổng kết", "Cài đặt", "Reset"};
    String nmEN[] = {"HAPPY", "SAD", "CRYING", "DESPAIR", "ANGRY", "FEAR", "ANXIOUS", "GAMES", "TOOLS", "GOAL", "SUMMARY", "SETTINGS", "RESET"};

    for (int i = 0; i < 13; i++) {
      int xP = 45 + (i * 100) - (int)currentX;
      if (xP > -60 && xP < 130) {
        display.setTextSize(3); display.setCursor(xP, 20); display.print(ic[i]);
        String name = (language == 0) ? nmVI[i] : nmEN[i];
        if (language == 0) {
           myfont.print(xP + (ic[i].length() * 9) - (myfont.getLength((char*)name.c_str()) / 2), 52, (char*)name.c_str(), SH110X_WHITE);
        } else {
           display.setTextSize(1); display.setCursor(xP + (ic[i].length() * 9) - (name.length() * 3), 52); display.print(name);
        }
      }
    }

    if (nTr.isPressed() && eIdx > 0) { eIdx--; targetX -= 100; beep(800, 20); }
    if (nP.isPressed() && eIdx < 12) { eIdx++; targetX += 100; beep(800, 20); }
    
    if (nOk.getState() == LOW) { // Button is being held
        if (holdStart == 0) holdStart = millis();
        unsigned long dur = millis() - holdStart;

        if (dur >= 1000) { // Held for 1 second
            if (eIdx == 12) { st = RESET_CONFIRM; mIdx = 0; }
            else if (eIdx == 11) { st = SETTINGS; adjustTime = now; setIdx = 0; returnState = E_SEL; }
            else if (eIdx == 10) { st = SUMMARY; } 
            else if (eIdx == 9) { st = GOAL_MENU; mIdx = 0; }
            else if (eIdx == 8) { st = GADGETS; mIdx = 0; }
            else if (eIdx == 7) { st = G_SEL; mIdx = 0; }
            else if (eIdx == 5) { st = CHECK_DANGER; mIdx = 0; }
            else { st = E_VIEW; }
            beep(1200, 50);
            holdStart = 0; // Reset for next time
        } else {
            // Display "Hold to select" message
            display.fillRect(0, 50, SW, 14, SH110X_BLACK); // Clear bottom area
            display.setTextSize(1);
            const char* msg = (language == 0) ? "Giữ 1s để chọn" : "HOLD 1s TO PICK";
            if (language == 0) {
               myfont.print((SW - myfont.getLength((char*)msg)) / 2, 52, (char*)msg, SH110X_WHITE);
            } else {
               display.setCursor((SW - (strlen(msg) * 6)) / 2, 52);
               display.print(msg);
            }
        }
    } else { // Button is not held
        holdStart = 0;
    }
    display.display();
  }

  else if (st == E_VIEW) {
    static int phase = 0; // 0: Quote/Joke, 1: Advice/Punchline
    display.clearDisplay();
    const void* group_pgm_ptr = pgm_read_ptr(&messageGroups[eIdx]);
    int count = messageGroupCounts[eIdx];
    static int r = -1; if(r == -1) { r = random(count); phase = 0; }

    Msg selectedMsg;
    memcpy_P(&selectedMsg, (PGM_P)group_pgm_ptr + r * sizeof(Msg), sizeof(Msg));
    char buffer[200]; // Buffer to hold strings from PROGMEM

    display.setTextWrap(true);
    display.setTextSize(1);
    
    if (phase == 0) {
      strcpy_P(buffer, (language == 0) ? selectedMsg.text1_VI : selectedMsg.text1_EN);
      if (language == 0) {
        printWrappedVI(0, 5, buffer);
        myfont.print((SW - myfont.getLength((char*)"Nhấn OK để tiếp"))/2, 52, (char*)"Nhấn OK để tiếp", SH110X_WHITE);
      } else {
        display.setCursor(0, 5); display.print(buffer);
        display.setCursor(0, 55); display.print("PRESS OK FOR MORE");
      }
    } else {
      strcpy_P(buffer, (language == 0) ? selectedMsg.text2_VI : selectedMsg.text2_EN);
      if (language == 0) {
        printWrappedVI(0, 5, buffer);
        myfont.print((SW - myfont.getLength((char*)"Nhấn OK để xong"))/2, 52, (char*)"Nhấn OK để xong", SH110X_WHITE);
      } else {
        display.setCursor(0, 5); display.print(buffer);
        display.setCursor(0, 55); display.print("PRESS OK TO FINISH");
      }
    }
    
    display.setTextWrap(false); // Reset to default for other screens
    display.display();
    if (nOk.isPressed()) { 
      if (phase == 0) { phase = 1; beep(800, 50); }
      else { st = ASK; mIdx = 0; r = -1; phase = 0; beep(1000, 50); }
    }
  }

  else if (st == ASK) {
    display.clearDisplay();
    if (language == 0) {
      myfont.print(25, 15, "Đã đỡ hơn chưa?", SH110X_WHITE);
      if (mIdx == 0) myfont.print(20, 45, "> Có    Không", SH110X_WHITE);
      else myfont.print(20, 45, "  Có  > Không", SH110X_WHITE);
    } else {
      display.setCursor(25, 15); display.println("Feeling better?");
      display.setCursor(31, 45); display.println(mIdx==0 ? "> YES    NO" : "  YES  > NO");
    }
    display.display();
    if(nTr.isPressed() || nP.isPressed() || nL.isPressed() || nX.isPressed()) { mIdx = 1 - mIdx; beep(600, 30); }
    if(nOk.isPressed()) {
      logEmotion(eIdx); // Ghi lai cam xuc ke ca khi chon KHONG
      if(mIdx == 0) { 
        st = G_SEL; 
      } else { 
        st = E_SEL; 
      }
      beep(1500, 100);
    }
  }

  else if (st == G_SEL) {
    display.clearDisplay(); 
    if (language == 0) myfont.print((SW - myfont.getLength("Chọn game:"))/2, 0, "Chọn game:", SH110X_WHITE);
    else { display.setCursor(28, 0); display.println("SELECT GAME:"); }
    
    const char* gNames[] = {"TETRIS", "CROSSY", "PINGPONG", "SNAKE", "DINO", "FLAPPY"};
    const char* exitName = (language==0) ? "Thoát" : "EXIT";
    int top = (mIdx > 3) ? mIdx - 3 : 0;
    for(int i=0; i<7; i++) {
      int y = 12 + (i - top) * 9;
      if(y < 12) continue;
      if(y < 64) {
        if (language == 0) {
           myfont.print(10, y, (mIdx == i) ? "> " : "  ", SH110X_WHITE);
           myfont.print(22, y, (char*)((i < 6) ? gNames[i] : exitName), SH110X_WHITE);
        } else {
           display.setCursor(10, y);
           if(mIdx == i) display.print("> "); else display.print("  ");
           if (i < 6) display.println(gNames[i]); else display.println(exitName);
        }
      }
    }
    display.display();
    if(nL.isPressed()) { mIdx=(mIdx-1+7)%7; beep(600, 20); }
    if(nX.isPressed()) { mIdx=(mIdx+1)%7; beep(600, 20); }
    if(nOk.isPressed()) {
      if (mIdx == 6) { // THOAT
        st = E_SEL;
        beep(1500, 50);
      } else {
        gameChoice = mIdx;
        st = DIFF_SEL;
        mIdx = difficulty; // Mac dinh chon do kho hien tai
        beep(1000, 50);
      }
    }
  }

  else if (st == DIFF_SEL) {
    display.clearDisplay();
    if (language == 0) myfont.print(10, 5, "Chọn độ khó:", SH110X_WHITE);
    else { display.setCursor(10, 5); display.println("DIFFICULTY:"); }
    
    const char* dNamesVI[] = {"Dễ", "Trung bình", "Khó"};
    const char* dNamesEN[] = {"EASY", "MEDIUM", "HARD"};
    for(int i=0; i<3; i++) {
      if (language == 0) {
        myfont.print(10, 20 + i*12, (mIdx == i) ? "> " : "  ", SH110X_WHITE);
        myfont.print(22, 20 + i*12, (char*)dNamesVI[i], SH110X_WHITE);
      } else {
        display.setCursor(10, 20 + i*12);
        if(mIdx == i) display.print("> "); else display.print("  ");
        display.println(dNamesEN[i]);
      }
    }
    display.display();
    
    if(nL.isPressed()) { mIdx = (mIdx - 1 + 3) % 3; beep(600, 20); }
    if(nX.isPressed()) { mIdx = (mIdx + 1) % 3; beep(600, 20); }
    if(nTr.isPressed()) { st = G_SEL; mIdx = gameChoice; beep(1000, 50); } // Quay lai
    
    if(nOk.isPressed()) {
      difficulty = mIdx;
      initGame();
      beep(1500, 50);
    }
  }

  else if (st == TETRIS) {
    static unsigned long lF = 0;
    // Objective: Speed up every 500 points
    int startSpd = (difficulty == 0) ? 600 : ((difficulty == 1) ? 500 : 400);
    int minSpd = (difficulty == 0) ? 150 : ((difficulty == 1) ? 100 : 50);
    int baseSpeed = max(minSpd, startSpd - (sc / 500) * 30);
    int fS = (nX.isPressed()) ? 50 : baseSpeed;
    if (millis() - lF > fS) {
      if (!checkC(px, py + 1, pR)) py++;
      else {
        for (int i = 0; i < 16; i++) if (shapes[pT][pR] & (1 << (15 - i))) {
          int tx = px + (i % 4), ty = py + (i / 4);
          if (ty >= 0 && tx >= 0 && tx < 10) grid[tx][ty] = true;
        }
        for (int y = 20-1; y >= 0; y--) {
          bool f = true; for (int x = 0; x < 10; x++) if (!grid[x][y]) f = false;
          if (f) { sc += 100; beep(1000, 50); for (int ty = y; ty > 0; ty--) for (int tx = 0; tx < 10; tx++) grid[tx][ty] = grid[tx][ty-1]; y++; }
        }
        py = 0; px = 4; pT = random(7); pR = 0; 
        if (checkC(px, py, pR)) { saveHighScore(0, sc); playGameOver(); st = G_SEL; }
        if (checkC(px, py, pR)) { saveHighScore(0, sc); playGameOver(); st = GAME_OVER; }
      }
      lF = millis();
    }
    if (nTr.isPressed() && !checkC(px - 1, py, pR)) px--;
    if (nP.isPressed() && !checkC(px + 1, py, pR)) px++;
    if (nL.isPressed()) { int nr = (pR + 1) % 4; if (!checkC(px, py, nr)) pR = nr; }
    
    display.clearDisplay();
    display.drawRect(39, 0, 42, 62, SH110X_WHITE);
    for (int x=0; x<10; x++) for (int y=0; y<20; y++) if (grid[x][y]) display.fillRect(40+x*4, y*3, 3, 2, SH110X_WHITE);
    uint16_t s = shapes[pT][pR];
    for (int i=0; i<16; i++) if (s & (1 << (15-i))) {
      int tx = px + (i % 4), ty = py + (i / 4);
      if (ty >= 0) display.fillRect(40+tx*4, ty*3, 3, 2, SH110X_WHITE);
    }
    display.setCursor(85, 5); display.print(sc); 
    display.setCursor(85, 15); display.print("HI:"); display.print(highScore);
    display.display();
    if (nOk.isPressed()) { st = PAUSE; pauseOpt = 0; beep(1000, 50); }
  }

  else if (st == CROSSY) {
    display.clearDisplay();
    int lanesY[] = {12, 24, 36, 48};
    float baseMult = (difficulty == 0) ? 1.0 : ((difficulty == 1) ? 1.25 : 1.5);
    float spdMult = baseMult + (sc / 100.0) * 0.1; // Objective: Speed up
    for(int i=0; i<4; i++) {
      display.drawFastHLine(0, lanesY[i]+8, 128, SH110X_WHITE);
      display.fillRect((int)carsX[i], lanesY[i], 16, 7, SH110X_WHITE);
      carsX[i] += carSpd[i] * spdMult;
      if (carsX[i] > 140) carsX[i] = -20; if (carsX[i] < -30) carsX[i] = 130;
      if (ckY > lanesY[i]-4 && ckY < lanesY[i]+7 && ckX > carsX[i]-4 && ckX < carsX[i]+16) {
        saveHighScore(1, sc); playGameOver(); st = G_SEL;
        saveHighScore(1, sc); playGameOver(); st = GAME_OVER;
      }
    }
    display.setCursor(0, 0); display.print(sc);
    display.setCursor(60, 0); display.print("HI:"); display.print(highScore);
    display.fillCircle((int)ckX, (int)ckY, 3, SH110X_WHITE);
    display.display();
    if (nL.isPressed()) { ckY -= 6; sc += 10; beep(1000, 10); }
    if (nX.isPressed() && ckY < 58) ckY += 6;
    if (nTr.isPressed() && ckX > 5) ckX -= 3;
    if (nP.isPressed() && ckX < 120) ckX += 3;
    if (ckY < 8) { beep(1800, 30); ckY = 58; sc += 50; }
    if (nOk.isPressed()) { st = PAUSE; pauseOpt = 0; beep(1000, 50); }
  }

  else if (st == PINGPONG) {
    display.clearDisplay();
    display.fillRect(2, p1Y, 3, 20, SH110X_WHITE);
    display.fillCircle((int)bX, (int)bY, 2, SH110X_WHITE);
    display.setCursor(60, 0); display.print(sc); 
    display.setCursor(60, 10); display.print("HI:"); display.print(highScore);
    display.display();
    bX += bDX; bY += bDY;
    if(bY <= 0 || bY >= 62) bDY *= -1;
    if(bX <= 5 && bY >= p1Y && bY <= p1Y+20) { 
      float mult = (difficulty == 0) ? 1.05 : ((difficulty == 1) ? 1.08 : 1.1);
      bDX = -bDX * mult; // Objective: Ball gets faster
      if(abs(bDX) > 6) bDX = (bDX > 0 ? 6 : -6); // Cap speed
      sc++; beep(1000, 20); 
    }
    if(bX >= 125) bDX *= -1;
    if(bX < 0) { saveHighScore(2, sc); playGameOver(); st=G_SEL; }
    if(nL.isPressed() && p1Y > 0) p1Y -= 4;
    if(nX.isPressed() && p1Y < 44) p1Y += 4;
    if(bX < 0) { saveHighScore(2, sc); playGameOver(); st=GAME_OVER; }
    if(nL.getState() == LOW && p1Y > 0) p1Y -= 2;
    if(nX.getState() == LOW && p1Y < 44) p1Y += 2;
    if(nOk.isPressed()) { st = PAUSE; pauseOpt = 0; beep(1000, 50); }
  }

  else if (st == SNAKE) {
    static unsigned long lF = 0;
    int startSpd = (difficulty == 0) ? 300 : ((difficulty == 1) ? 250 : 200);
    int minSpd = (difficulty == 0) ? 100 : ((difficulty == 1) ? 75 : 50);
    int speed = max(minSpd, startSpd - (sc / 5) * 5);
    if (millis() - lF > speed) {
      for (int i = snLen; i > 0; i--) { snX[i] = snX[i-1]; snY[i] = snY[i-1]; }
      snX[0] += snDirX; snY[0] += snDirY;
      if (snX[0] < 0 || snX[0] >= 32 || snY[0] < 0 || snY[0] >= 16) { saveHighScore(3, sc); playGameOver(); st = G_SEL; }
      for (int i = 1; i < snLen; i++) if (snX[0] == snX[i] && snY[0] == snY[i]) { saveHighScore(3, sc); playGameOver(); st = G_SEL; }
      if (snX[0] < 0 || snX[0] >= 32 || snY[0] < 0 || snY[0] >= 16) { saveHighScore(3, sc); playGameOver(); st = GAME_OVER; }
      for (int i = 1; i < snLen; i++) if (snX[0] == snX[i] && snY[0] == snY[i]) { saveHighScore(3, sc); playGameOver(); st = GAME_OVER; }
      if (snX[0] == apX && snY[0] == apY) {
        sc++; snLen++; beep(1000, 20);
        apX = random(32); apY = random(16);
      }
      lF = millis();
    }
    if (nL.isPressed() && snDirY == 0) { snDirX = 0; snDirY = -1; }
    if (nX.isPressed() && snDirY == 0) { snDirX = 0; snDirY = 1; }
    if (nTr.isPressed() && snDirX == 0) { snDirX = -1; snDirY = 0; }
    if (nP.isPressed() && snDirX == 0) { snDirX = 1; snDirY = 0; }
    
    display.clearDisplay();
    for (int i = 0; i < snLen; i++) display.fillRect(snX[i]*4, snY[i]*4, 3, 3, SH110X_WHITE);
    display.fillRect(apX*4, apY*4, 3, 3, SH110X_WHITE);
    display.setCursor(0,0); display.print(sc);
    display.display();
    if (nOk.isPressed()) { st = PAUSE; pauseOpt = 0; beep(1000, 50); }
  }

  else if (st == DINO) {
    // Variable jump: cut velocity if button released
    if (nL.getState() == HIGH && dV < -3.0) dV = -3.0;

    dY += dV; dV += 0.6;
    if (dY >= 55) { dY = 55; dV = 0; }
    float baseSpd = (difficulty == 0) ? 2.0 : ((difficulty == 1) ? 3.0 : 4.0);
    float currentSpd = baseSpd + (sc * 0.06);
    obsX -= currentSpd;
    if (obsX < -10) { obsX = 128 + random(20); obsW = random(5, 9); obsH = random(6, 12); sc++; beep(500, 20); }
    
    if (10 + 8 > obsX && 10 < obsX + obsW && dY > 55 - obsH) { saveHighScore(4, sc); playGameOver(); st = GAME_OVER; }
    
    if (nL.isPressed() && dY >= 55) { dV = -6.0; beep(200, 20); }
    
    display.clearDisplay();
    display.drawFastHLine(0, 55, 128, SH110X_WHITE);
    display.fillRect(10, (int)dY - 10, 8, 10, SH110X_WHITE);
    display.fillRect((int)obsX, 55 - obsH, obsW, obsH, SH110X_WHITE);
    // Cooler Dino
    display.fillRect(10, (int)dY - 10, 6, 7, SH110X_WHITE); // Body
    display.fillRect(12, (int)dY - 13, 6, 5, SH110X_WHITE); // Head
    display.drawPixel(13, (int)dY - 12, SH110X_BLACK); // Eye
    display.drawFastVLine(11, (int)dY - 3, 3, SH110X_WHITE); // Leg
    display.drawFastVLine(14, (int)dY - 3, 3, SH110X_WHITE); // Leg
    // Cooler Cactus
    display.drawFastVLine((int)obsX + obsW/2, 55 - obsH, obsH, SH110X_WHITE);
    display.drawFastHLine((int)obsX, 55 - obsH + 3, obsW, SH110X_WHITE);
    display.drawFastVLine((int)obsX, 55 - obsH + 1, 3, SH110X_WHITE);
    display.drawFastVLine((int)obsX + obsW, 55 - obsH + 1, 3, SH110X_WHITE);
    
    display.setCursor(60, 0); display.print(sc);
    display.setCursor(60, 10); display.print("HI:"); display.print(highScore);
    display.display();
    if (nOk.isPressed()) { st = PAUSE; pauseOpt = 0; beep(1000, 50); }
  }

  else if (st == FLAPPY) {
    static bool scored = false;
    if (!gamePlaying) {
      display.clearDisplay();
      display.fillRect(SW/2 - 4, (int)fbY, 8, 8, SH110X_WHITE);
      if (language == 0) myfont.print(20, 20, "NHẤN ĐỂ BẮT ĐẦU", SH110X_WHITE);
      else { display.setCursor(20, 20); display.print("PRESS TO START"); }
      display.display();
      if (nL.isPressed()) { gamePlaying = true; fbV = -3.0; beep(200, 20); }
      return;
    }

    // --- Physics ---
    if (nL.getState() == HIGH && fbV < -1.5) fbV = -1.5;
    fbY += fbV;
    fbV += 0.4; // Gravity

    // --- Input ---
    if (nL.isPressed()) {
        fbV = -3.0; // Jump
        beep(200, 20);
    }

    // --- Pipes ---
    float pipeSpeed = (difficulty == 0) ? 1.5 : ((difficulty == 1) ? 2.0 : 2.5);
    pipeX -= pipeSpeed;
    if (pipeX < -pipeW) {
        pipeX = SW;
        pipeH = random(10, SH - pipeGap - 10);
        scored = false; // Reset score flag for new pipe
    }

    // --- Scoring ---
    if (!scored && (pipeX + pipeW) < (SW/2 - 4)) {
         sc++;
         scored = true;
         beep(500, 20);
    }

    // --- Collision ---
    bool collision = (fbY < 0 || fbY > SH - 8); // Top/Bottom
    if (pipeX < (SW/2 + 4) && pipeX + pipeW > (SW/2 - 4)) { // Pipe X collision
        if (fbY < pipeH || fbY + 8 > pipeH + pipeGap) collision = true; // Pipe Y
    }

    if (collision) { saveHighScore(5, sc); playGameOver(); st = GAME_OVER; }

    // --- Drawing ---
    display.clearDisplay();
    display.fillRect(SW/2 - 4, (int)fbY, 8, 8, SH110X_WHITE); // Bird
    display.drawPixel(SW/2 + 2, (int)fbY + 2, SH110X_BLACK); // Eye
    display.fillRect(pipeX, 0, pipeW, pipeH, SH110X_WHITE); // Top pipe
    display.fillRect(pipeX, pipeH + pipeGap, pipeW, SH - (pipeH + pipeGap), SH110X_WHITE); // Bottom pipe
    display.setCursor(5, 5); display.print("SC:"); display.print(sc);
    display.setCursor(SW - 50, 5); display.print("HI:"); display.print(highScore);
    display.display();
    if (nOk.isPressed()) { st = PAUSE; pauseOpt = 0; beep(1000, 50); }
  }

  else if (st == PAUSE) {
    drawPauseMenu();
    if (nL.isPressed()) { pauseOpt = (pauseOpt - 1 + 3) % 3; beep(600, 20); }
    if (nX.isPressed()) { pauseOpt = (pauseOpt + 1) % 3; beep(600, 20); }
    if (nOk.isPressed()) {
      if (pauseOpt == 0) st = lastGame;
      else if (pauseOpt == 1) { st = SETTINGS; adjustTime = now; setIdx = 0; returnState = PAUSE; }
      else { 
        // Save score before exit
        int gIdx = -1;
        if (lastGame == TETRIS) gIdx = 0;
        else if (lastGame == CROSSY) gIdx = 1;
        else if (lastGame == PINGPONG) gIdx = 2;
        else if (lastGame == SNAKE) gIdx = 3;
        else if (lastGame == DINO) gIdx = 4;
        else if (lastGame == FLAPPY) gIdx = 5;
        if (gIdx != -1) saveHighScore(gIdx, sc);
        st = G_SEL; 
      }
      beep(1000, 50);
    }
  }

  else if (st == GAME_OVER) {
    display.clearDisplay();
    display.fillScreen(SH110X_WHITE);
    display.setTextColor(SH110X_BLACK);
    display.setCursor(37, 10); display.print("GAME OVER");
    display.setCursor(35, 30); display.print("SCORE: "); display.print(sc);
    display.setCursor(15, 45);
    if (language == 0) {
      if(mIdx == 0) myfont.print(15, 45, "> CHƠI LẠI  THOÁT", SH110X_BLACK);
      else myfont.print(15, 45, "  CHƠI LẠI  > THOÁT", SH110X_BLACK);
    } else {
      if(mIdx == 0) display.print("> REPLAY   EXIT");
      else display.print("  REPLAY   > EXIT");
    }
    display.display();
    display.setTextColor(SH110X_WHITE);
    if(nL.isPressed() || nX.isPressed() || nTr.isPressed() || nP.isPressed()) { mIdx = 1 - mIdx; beep(600, 20); }
    if (nOk.isPressed()) {
      if(mIdx == 0) initGame();
      else { st = G_SEL; mIdx = gameChoice; }
      beep(1000, 50);
    }
  }

  else if (st == SETTINGS) {
    // New Settings Menu
    display.clearDisplay();
    if (language == 0) myfont.print((SW - myfont.getLength("Cài đặt:"))/2, 0, "Cài đặt:", SH110X_WHITE);
    else { display.setCursor(37, 0); display.println("SETTINGS:"); }
    
    const char* itemsVI[] = {"Thời gian", "Âm thanh", "Ngôn ngữ", "Thoát"};
    const char* itemsEN[] = {"TIME", "SOUND", "LANGUAGE", "EXIT"};
    
    for(int i=0; i<4; i++) {
      if (language == 0) {
        myfont.print(10, 20 + i*12, (mIdx == i) ? "> " : "  ", SH110X_WHITE);
        myfont.print(22, 20 + i*12, (char*)itemsVI[i], SH110X_WHITE);
        
        // Cap nhat vi tri con tro GFX de hien thi gia tri (ON/OFF, VI/EN) dung cho
        if (i == 1 || i == 2) {
             int w = myfont.getLength((char*)itemsVI[i]);
             display.setCursor(22 + w, 20 + i*12);
        }
      } else {
        display.setCursor(10, 20 + i*12);
        if(mIdx == i) display.print("> "); else display.print("  ");
        display.print(itemsEN[i]);
      }
      
      if (i == 1) { display.print(": "); display.print(soundOff ? "OFF" : "ON"); }
      if (i == 2) { display.print(": "); display.print(language==0 ? "VI" : "EN"); }
    }
    display.display();
    
    if(nL.isPressed()) { mIdx=(mIdx-1+4)%4; beep(600, 20); }
    if(nX.isPressed()) { mIdx=(mIdx+1)%4; beep(600, 20); }
    if(nOk.isPressed()) {
      if(mIdx == 0) { st = SET_TIME; setIdx = 0; } // Go to Time Adjust
      else if(mIdx == 1) { soundOff = !soundOff; EEPROM.write(8, soundOff); EEPROM.commit(); }
      else if(mIdx == 2) { language = 1 - language; EEPROM.write(9, language); EEPROM.commit(); }
      else if(mIdx == 3) { st = returnState; }
      beep(1000, 50);
    }
  }

  else if (st == SET_TIME) {
    display.clearDisplay();
    if (language == 0) myfont.print((SW - myfont.getLength("Chỉnh giờ"))/2, 0, "Chỉnh giờ", SH110X_WHITE);
    else { display.setCursor(40, 0); display.print("SET TIME"); }
    
    char timeBuf[10];
    sprintf(timeBuf, "%02d:%02d", adjustTime.hour(), adjustTime.minute());
    display.setCursor(49, 20);
    display.print(timeBuf);

    char dateBuf[15];
    sprintf(dateBuf, "%02d/%02d/%04d", adjustTime.day(), adjustTime.month(), adjustTime.year());
    display.setCursor(34, 35);
    display.print(dateBuf);
    
    // Draw an underline cursor for the selected item
    int cursorX, cursorY;
    int cursorW = (setIdx == 4) ? 4*6 : 2*6;
    
    if (setIdx < 2) { cursorY = 28; cursorX = 49 + (setIdx * 18); }
    else { cursorY = 43; cursorX = 34 + ((setIdx - 2) * 18); }
    
    display.drawFastHLine(cursorX, cursorY, cursorW, SH110X_WHITE);

    if (language == 0) myfont.print((SW - myfont.getLength("OK:Lưu Tr:Về"))/2, 52, "OK:Lưu Tr:Về", SH110X_WHITE);
    else { display.setCursor(19, 50); display.print("OK:Save Tr:Back"); }
    display.display();

    if (nTr.isPressed()) { st = SETTINGS; mIdx = 0; beep(1000, 50); } // Back to Menu
    if (nP.isPressed()) { setIdx = (setIdx + 1) % 5; beep(600, 20); } // Cycle fields
    if (nL.isPressed()) { // Increase (nL is UP button)
      TimeSpan ts(setIdx==2?1:0, setIdx==0?1:0, setIdx==1?1:0, 0);
      if(setIdx < 3) adjustTime = adjustTime + ts;
      else if(setIdx == 3) { int m = adjustTime.month()+1; if(m>12) m=1; adjustTime = DateTime(adjustTime.year(), m, adjustTime.day(), adjustTime.hour(), adjustTime.minute()); }
      else { adjustTime = DateTime(adjustTime.year()+1, adjustTime.month(), adjustTime.day(), adjustTime.hour(), adjustTime.minute()); }
      beep(800, 20);
    }
    if (nX.isPressed()) { // Decrease (nX is DOWN button)
      TimeSpan ts(setIdx==2?1:0, setIdx==0?1:0, setIdx==1?1:0, 0);
      if(setIdx < 3) adjustTime = adjustTime - ts;
      else if(setIdx == 3) { int m = adjustTime.month()-1; if(m<1) m=12; adjustTime = DateTime(adjustTime.year(), m, adjustTime.day(), adjustTime.hour(), adjustTime.minute()); }
      else { adjustTime = DateTime(adjustTime.year()-1, adjustTime.month(), adjustTime.day(), adjustTime.hour(), adjustTime.minute()); }
      beep(800, 20);
    }
    if (nOk.isPressed()) { rtc.adjust(adjustTime); st = SETTINGS; mIdx = 0; beep(1500, 100); }
  }

  else if (st == SUMMARY) {
    display.clearDisplay();
    if (language == 0) myfont.print((SW - myfont.getLength("Tổng kết 30 ngày:"))/2, 0, "Tổng kết 30 ngày:", SH110X_WHITE);
    else { display.setCursor(16, 0); display.println("30 DAYS SUMMARY:"); }
    int counts[7] = {0,0,0,0,0,0,0};
    for(int i=0; i<30; i++) {
      if (EEPROM.read(20 + i*3) != 0) { // Chi dem khi co du lieu ngay (khac 0)
        int emo = EEPROM.read(20 + i*3 + 2);
        if(emo >= 0 && emo < 7) counts[emo]++; // Safety check
      }
    }
    String namesVI[] = {"Vui", "Buồn", "Khóc", "Tuyệt", "Giận", "Sợ", "Lo"};
    String namesEN[] = {"Hap", "Sad", "Cry", "Desp", "Ang", "Fear", "Anx"};
    for(int i=0; i<7; i++) {
      if (language == 0) {
         char line_buf[20];
         sprintf(line_buf, "%s:%d", namesVI[i].c_str(), counts[i]);
         myfont.print((i%2)*62, 12 + (i/2)*11, line_buf, SH110X_WHITE);
      } else {
         display.setCursor((i%2)*62, 12 + (i/2)*11);
         display.print(namesEN[i]);
         display.print(":"); display.print(counts[i]);
      }
    }
    if (language == 0) myfont.print((SW - myfont.getLength("Nhấn OK để thoát"))/2, 52, "Nhấn OK để thoát", SH110X_WHITE);
    else { display.setCursor(16, 55); display.print("Press OK to exit"); }
    display.display();
    if (nOk.isPressed()) { st = E_SEL; beep(1000, 50); }
  }

  else if (st == CHECK_DANGER) {
    display.clearDisplay();
    if (language == 0) {
      myfont.print(5, 10, "Bạn có gặp", SH110X_WHITE);
      myfont.print(5, 25, "nguy hiểm không?", SH110X_WHITE);
      if (mIdx == 0) myfont.print(20, 45, "> Có    Không", SH110X_WHITE);
      else myfont.print(20, 45, "  Có  > Không", SH110X_WHITE);
    } else {
      display.setCursor(5, 10); display.println("Are you in");
      display.setCursor(5, 25); display.println("danger?");
      display.setCursor(31, 45); display.println(mIdx==0 ? "> YES    NO" : "  YES  > NO");
    }
    display.display();
    if(nTr.isPressed() || nP.isPressed() || nL.isPressed() || nX.isPressed()) { mIdx = 1 - mIdx; beep(600, 30); }
    if(nOk.isPressed()) { 
      if (mIdx == 0) {
        logEmotion(eIdx);
        st = DANGER_WARN;
      } else { st = E_VIEW; }
      beep(1000, 50); 
    }
  }

  else if (st == DANGER_WARN) {
    display.clearDisplay();
    if (language == 0) {
       // myfont doesn't support setTextSize(2), so we might just use size 1 or draw twice?
       // For simplicity, use size 1 but centered or just standard myfont
       myfont.print((SW - myfont.getLength("Tìm nơi"))/2, 10, "Tìm nơi", SH110X_WHITE);
       myfont.print((SW - myfont.getLength("An toàn!"))/2, 35, "An toàn!", SH110X_WHITE);
    } else {
       display.setCursor(37, 10); display.println("FIND SAFE");
       display.setCursor(46, 35); display.println("PLACE!");
    }
    display.display();
    if(nOk.isPressed()) { st = E_SEL; beep(1000, 50); }
  }

  else if (st == GADGETS) {
    display.clearDisplay(); 
    if (language == 0) myfont.print((SW - myfont.getLength("Tiện ích:"))/2, 0, "Tiện ích:", SH110X_WHITE); else { display.setCursor(40, 0); display.println("GADGETS:"); }
    const char* tNamesVI[] = {"Bấm giờ", "Hẹn giờ", "Xúc xắc", "Thoát"};
    const char* tNamesEN[] = {"STOPWATCH", "TIMER", "DICE", "EXIT"};
    for(int i=0; i<4; i++) {
      if (language == 0) {
        myfont.print(10, 20 + i*12, (mIdx == i) ? "> " : "  ", SH110X_WHITE);
        myfont.print(22, 20 + i*12, (char*)tNamesVI[i], SH110X_WHITE);
      } else {
        display.setCursor(10, 20 + i*12);
        if(mIdx == i) display.print("> "); else display.print("  ");
        display.println(tNamesEN[i]);
      }
    }
    display.display();
    if(nL.isPressed()) { mIdx=(mIdx-1+4)%4; beep(600, 20); }
    if(nX.isPressed()) { mIdx=(mIdx+1)%4; beep(600, 20); }
    if(nOk.isPressed()) {
      if(mIdx == 0) { st = STOPWATCH; swRun = false; swElapsed = 0; }
      else if(mIdx == 1) { st = TIMER; if(!tmRun) { tmDuration = 0; tmRem = 0; mIdx = 0; tmHour=0; tmMin=0; tmSec=0; } }
      else if(mIdx == 2) { st = DICE; diceVal = 1; }
      else st = E_SEL;
      beep(1000, 50);
    }
  }

  else if (st == STOPWATCH) {
    display.clearDisplay();
    if (language == 0) myfont.print((SW - myfont.getLength("Bấm giờ"))/2, 0, "Bấm giờ", SH110X_WHITE);
    else { display.setCursor(37, 0); display.print("STOPWATCH"); }
    unsigned long current = swElapsed + (swRun ? (millis() - swStart) : 0);
    int m = (current / 60000);
    int s = (current % 60000) / 1000;
    int ms = (current % 1000) / 10;
    display.setCursor(40, 25);
    if(m<10) display.print("0"); display.print(m); display.print(":"); if(s<10) display.print("0"); display.print(s); display.print("."); if(ms<10) display.print("0"); display.print(ms);
    if (language == 0) myfont.print((SW - myfont.getLength("OK:Chạy/Dừng,L:Xóa"))/2, 52, "OK:Chạy/Dừng,L:Xóa", SH110X_WHITE);
    else { display.setCursor(7, 55); display.print("OK:Run/Stop,L:Reset"); }
    display.display();
    
    if(nOk.isPressed()) {
      if(swRun) { swElapsed += millis() - swStart; swRun = false; }
      else { swStart = millis(); swRun = true; }
      beep(1000, 50);
    }
    if(nL.isPressed() && !swRun) { swElapsed = 0; beep(600, 50); }
    if(nTr.isPressed()) { st = GADGETS; mIdx = 0; beep(1000, 50); }
  }

  else if (st == TIMER) {
    display.clearDisplay();
    if (language == 0) myfont.print((SW - myfont.getLength("Hẹn giờ"))/2, 0, "Hẹn giờ", SH110X_WHITE);
    else { display.setCursor(49, 0); display.print("TIMER"); }
    
    if(tmRun) {
      long passed = millis() - tmStart;
      tmRem = tmDuration - passed;
      if(tmRem < 0) tmRem = 0;
    } else {
      tmRem = (tmHour * 3600L + tmMin * 60 + tmSec) * 1000L;
    }
    
    int h = tmRem / 3600000;
    int m = (tmRem % 3600000) / 60000;
    int s = (tmRem % 60000) / 1000;
    
    display.setCursor(40, 25);
    if(h<10) display.print("0"); display.print(h); display.print(":");
    if(m<10) display.print("0"); display.print(m); display.print(":");
    if(s<10) display.print("0"); display.print(s);
    
    if(!tmRun) {
      int lineX = (mIdx==0) ? 40 : ((mIdx==1) ? 58 : 76);
      display.drawFastHLine(lineX, 35, 12, SH110X_WHITE);
      if (language == 0) myfont.print((SW - myfont.getLength("L/X:+- Tr:Về P:Chọn"))/2, 52, "L/X:+- Tr:Về P:Chọn", SH110X_WHITE);
      else { display.setCursor(4, 55); display.print("L/X:+- Tr:Back P:Sel"); }
      
      if(nP.isPressed()) { mIdx = (mIdx + 1) % 3; beep(600, 20); }
      if(nTr.isPressed()) {
        st = GADGETS; mIdx = 1; beep(1000, 50);
      }
      
      if(nL.isPressed()) { 
        if(mIdx==0) tmHour++; 
        else if(mIdx==1) { tmMin++; if(tmMin>=60) tmMin=0; }
        else { tmSec++; if(tmSec>=60) tmSec=0; } 
        beep(600, 20); 
      }
      if(nX.isPressed()) { 
        if(mIdx==0) { if(tmHour>0) tmHour--; }
        else if(mIdx==1) { tmMin--; if(tmMin<0) tmMin=59; }
        else { tmSec--; if(tmSec<0) tmSec=59; }
        beep(600, 20); 
      }
    } else {
      if (language == 0) myfont.print((SW - myfont.getLength("OK:Dừng Tr:Thoát"))/2, 52, "OK:Dừng Tr:Thoát", SH110X_WHITE);
      else { display.setCursor(19, 55); display.print("OK:Stop Tr:Exit"); }
      if(nTr.isPressed()) { st = GADGETS; mIdx = 1; beep(1000, 50); }
    }
    display.display();
    
    if(nOk.isPressed()) {
      if(tmRun) { tmRun = false; tmDuration = tmRem; tmHour = tmRem/3600000; tmMin = (tmRem%3600000)/60000; tmSec=(tmRem%60000)/1000; }
      else if(tmRem > 0) { tmDuration = tmRem; tmStart = millis(); tmRun = true; }
      beep(1000, 50);
    }
  }

  else if (st == TIMER_ALERT) {
    display.clearDisplay();
    if ((millis() / 200) % 2 == 0) display.fillScreen(SH110X_WHITE);
    display.setTextColor(((millis() / 200) % 2 == 0) ? SH110X_BLACK : SH110X_WHITE);
    if (language == 0) {
       // myfont doesn't support text color inversion easily with fillScreen logic unless we handle it manually
       // For simplicity, just draw text
       myfont.print((SW - myfont.getLength("Hết giờ!"))/2, 20, "Hết giờ!", ((millis() / 200) % 2 == 0) ? SH110X_BLACK : SH110X_WHITE);
       myfont.print((SW - myfont.getLength("Nhấn OK để tắt"))/2, 52, "Nhấn OK để tắt", ((millis() / 200) % 2 == 0) ? SH110X_BLACK : SH110X_WHITE);
    } else {
       display.setCursor(34, 20); display.print("TIME'S UP!");
       display.setCursor(16, 45); display.print("PRESS OK TO STOP");
    }
    display.display();
    display.setTextColor(SH110X_WHITE);
    
    if ((millis() / 100) % 2 == 0) beep(2000, 50); else beep(1500, 50);
    
    if (nOk.isPressed()) {
      st = GADGETS; mIdx = 1;
      noTone(BZ);
      beep(1000, 100);
    }
  }

  else if (st == DICE) {
    display.clearDisplay();
    if (language == 0) myfont.print((SW - myfont.getLength("Xúc xắc"))/2, 0, "Xúc xắc", SH110X_WHITE);
    else { display.setCursor(52, 0); display.print("DICE"); }
    display.drawRect(44, 14, 40, 40, SH110X_WHITE);
    display.setCursor(61, 30); display.print(diceVal);
    if (language == 0) myfont.print((SW - myfont.getLength("OK:Tung Tr:Thoát"))/2, 52, "OK:Tung Tr:Thoát", SH110X_WHITE);
    else { display.setTextSize(1); display.setCursor(19, 55); display.print("OK:Roll Tr:Exit"); }
    display.display();
    if(nOk.isPressed()) { 
      for(int i=0; i<15; i++) { 
        diceVal = random(1, 7); 
        display.fillRect(45,15,38,38,SH110X_BLACK); 
        display.setCursor(61, 30); 
        display.print(diceVal); 
        display.display(); 
        delay(50 + i*10); 
        if (i < 14) beep(800, 20); // Only beep while the number is changing
      } 
    }
    if(nTr.isPressed()) { st = GADGETS; mIdx = 2; beep(1000, 50); }
  }

  else if (st == RESET_CONFIRM) {
    display.clearDisplay();
    if (language == 0) {
      myfont.print((SW - myfont.getLength("Xóa dữ liệu &")) / 2, 10, "Xóa dữ liệu &", SH110X_WHITE);
      myfont.print((SW - myfont.getLength("Khởi động lại?")) / 2, 22, "Khởi động lại?", SH110X_WHITE);
    } else {
      display.setCursor((SW - (strlen("WIPE DATA &") * 6)) / 2, 10); display.println("WIPE DATA &");
      display.setCursor((SW - (strlen("RESTART?") * 6)) / 2, 22); display.println("RESTART?");
    }
    
    if(nTr.isPressed() || nP.isPressed() || nL.isPressed() || nX.isPressed()) { mIdx = 1 - mIdx; beep(600, 30); }
    
    static unsigned long holdStart = 0;
    int optionX = (SW - (strlen(language==0 ? "> CÓ    KHÔNG" : "> YES    NO") * 6)) / 2;

    if (mIdx == 1) { // KHONG
      if (language == 0) myfont.print(20, 45, "  Có  > Không", SH110X_WHITE);
      else { display.setCursor(optionX, 45); display.println("  YES  > NO"); }
      if(nOk.isPressed()) { st = E_SEL; beep(1000, 50); }
      holdStart = 0;
    } else { // CO
      if (nOk.getState() == LOW) { // Nut dang duoc giu
        if (holdStart == 0) holdStart = millis();
        unsigned long dur = millis() - holdStart;
        if (dur >= 3000) { // Rut ngan thoi gian giu nut con 3 giay
           display.clearDisplay();
           if (language == 0) myfont.print(30, 30, "Đang xóa...", SH110X_WHITE);
           else { display.setCursor(30, 30); display.print("WIPING..."); }
           display.display();
           for (int i = 0; i < EEPROM_SIZE; i++) EEPROM.write(i, 0);
           EEPROM.commit();
           delay(500);
           ESP.restart();
        } else {
           if (language == 0) {
             myfont.print(30, 45, "Giữ 3s...", SH110X_WHITE);
           } else {
             const char* holdMsg = "HOLD 3s...";
             display.setCursor((SW - (strlen(holdMsg) * 6)) / 2, 45);
             display.print("HOLD "); display.print(3 - dur/1000); display.print("s...");
           }
        }
      } else {
        holdStart = 0;
        if (language == 0) myfont.print(20, 45, "> Có    Không", SH110X_WHITE);
        else { display.setCursor(optionX, 45); display.println("> YES    NO"); }
      }
    }
    display.display(); 
  }

  else if (st == GOAL_MENU) {
    display.clearDisplay(); 
    if (language == 0) myfont.print((SW - myfont.getLength("Mục tiêu:"))/2, 0, "Mục tiêu:", SH110X_WHITE);
    else { display.setCursor(46, 0); display.println("GOALS:"); }
    
    const char* gNamesVI[] = {"Thêm mục tiêu", "Mục tiêu hiện tại", "Thoát"};
    const char* gNamesEN[] = {"ADD PLAN", "CURRENT PLAN", "EXIT"};
    for(int i=0; i<3; i++) {
      if (language == 0) {
        myfont.print(10, 20 + i*12, (mIdx == i) ? "> " : "  ", SH110X_WHITE);
        myfont.print(22, 20 + i*12, (char*)gNamesVI[i], SH110X_WHITE);
      } else {
        display.setCursor(10, 20 + i*12);
        if(mIdx == i) display.print("> "); else display.print("  ");
        display.println(gNamesEN[i]);
      }
    }
    display.display();
    if(nL.isPressed()) { mIdx=(mIdx-1+3)%3; beep(600, 20); }
    if(nX.isPressed()) { mIdx=(mIdx+1)%3; beep(600, 20); }
    if(nOk.isPressed()) {
      if(mIdx == 0) { st = ADD_PLAN; mIdx = 0; }
      else if(mIdx == 1) { st = CURRENT_PLAN; mIdx = 0; }
      else st = E_SEL;
      beep(1000, 50);
    }
  }

  else if (st == ADD_PLAN) {
    display.clearDisplay(); 
    if (language == 0) myfont.print((SW - myfont.getLength("Chọn mục tiêu:"))/2, 0, "Chọn mục tiêu:", SH110X_WHITE);
    else { display.setCursor(25, 0); display.println("SELECT GOALS:"); }
    
    uint16_t planned = (EEPROM.read(125) << 8) | EEPROM.read(122);
    int top = (mIdx > 4) ? mIdx - 4 : 0;
    for(int i=0; i<10; i++) {
      int y = 15 + (i - top) * 10;
      if (y < 15 || y > 55) continue;
      if (language == 0) {
        myfont.print(0, y, (mIdx == i) ? "> " : "  ", SH110X_WHITE);
        myfont.print(12, y, (planned & (1 << i)) ? "[x] " : "[ ] ", SH110X_WHITE);
        myfont.print(36, y, (char*)goalsVI[i], SH110X_WHITE);
      } else {
        display.setCursor(0, y);
        if(mIdx == i) display.print("> "); else display.print("  ");
        display.print((planned & (1 << i)) ? "[x] " : "[ ] ");
        display.println(goalsEN[i]);
      }
    }
    display.display();
    if(nL.isPressed()) { mIdx=(mIdx-1+10)%10; beep(600, 20); }
    if(nX.isPressed()) { mIdx=(mIdx+1)%10; beep(600, 20); }
    if(nOk.isPressed()) {
      planned ^= (1 << mIdx);
      EEPROM.write(122, planned & 0xFF);
      EEPROM.write(125, (planned >> 8) & 0xFF);
      EEPROM.commit();
      beep(800, 20);
    }
    if(nTr.isPressed()) { st = GOAL_MENU; mIdx = 0; beep(1000, 50); }
  }

  else if (st == CURRENT_PLAN) {
    display.clearDisplay(); 
    if (language == 0) myfont.print((SW - myfont.getLength("Danh sách:"))/2, 0, "Danh sách:", SH110X_WHITE);
    else { display.setCursor(34, 0); display.println("YOUR PLAN:"); }
    
    uint16_t planned = (EEPROM.read(125) << 8) | EEPROM.read(122);
    uint16_t done = (EEPROM.read(126) << 8) | EEPROM.read(123);
    std::vector<int> activeGoals;
    for(int i=0; i<10; i++) if(planned & (1 << i)) activeGoals.push_back(i);
    
    if(activeGoals.empty()) {
      if (language == 0) myfont.print(10, 30, "(Trống)", SH110X_WHITE);
      else { display.setCursor(10, 30); display.print("(Empty)"); }
    } else {
      if(mIdx >= activeGoals.size()) mIdx = 0;
      int top = (mIdx > 4) ? mIdx - 4 : 0;
      for(int i=0; i<activeGoals.size(); i++) {
        int y = 15 + (i - top) * 10;
        if (y < 15 || y > 55) continue;
        int gIdx = activeGoals[i];
        if (language == 0) {
          myfont.print(0, y, (mIdx == i) ? "> " : "  ", SH110X_WHITE);
          myfont.print(12, y, (done & (1 << gIdx)) ? "[v] " : "[ ] ", SH110X_WHITE);
          myfont.print(36, y, (char*)goalsVI[gIdx], SH110X_WHITE);
        } else {
          display.setCursor(0, y);
          if(mIdx == i) display.print("> "); else display.print("  ");
          display.print((done & (1 << gIdx)) ? "[v] " : "[ ] ");
          display.println(goalsEN[gIdx]);
        }
      }
    }
    display.display();
    if(!activeGoals.empty()) {
      if(nL.isPressed()) { mIdx=(mIdx-1+activeGoals.size())%activeGoals.size(); beep(600, 20); }
      if(nX.isPressed()) { mIdx=(mIdx+1)%activeGoals.size(); beep(600, 20); }
      if(nOk.isPressed()) {
        done ^= (1 << activeGoals[mIdx]);
        EEPROM.write(123, done & 0xFF);
        EEPROM.write(126, (done >> 8) & 0xFF);
        EEPROM.commit();
        beep(800, 20);
      }
    }
    if(nTr.isPressed()) { st = GOAL_MENU; mIdx = 1; beep(1000, 50); }
  }

  else if (st == GOAL_REMINDER) {
    display.clearDisplay();
    if (language == 0) {
      myfont.print((SW - myfont.getLength("Xem lại mục tiêu?"))/2, 10, "Xem lại mục tiêu?", SH110X_WHITE);
      myfont.print((SW - myfont.getLength("Có(L) Không(X)"))/2, 45, "Có(L) Không(X)", SH110X_WHITE);
    } else {
      display.setTextSize(1);
      display.setTextWrap(true);
      display.setCursor(5, 10);
      display.print("Review today's goals?");
      display.setTextWrap(false);
      display.setCursor(16, 45); display.print("YES(UP) NO(DOWN)");
    }
    display.display();

    if (nL.isPressed()) { // YES
        st = CURRENT_PLAN;
        mIdx = 0;
        beep(1000, 50);
    }
    if (nX.isPressed()) { // NO
        st = E_SEL;
        beep(1000, 50);
    }
  }
}