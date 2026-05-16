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

// --- Note Frequencies for Music Player ---
#define REST      0
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_B5  988

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
#define PIN_VCC   2
#define PIN_SLP   8
#define PIN_BZ    7
ezButton nL(PIN_UP), nX(PIN_DOWN), nTr(PIN_LEFT), nP(PIN_RIGHT), nOk(PIN_OK), nSlp(PIN_SLP);

// --- PROGMEM Data for Messages ---
// This moves all the constant strings into Flash memory to save RAM.
struct Msg {
  PGM_P text1_VI; // Quote or Joke Setup (VI)
  PGM_P text1_EN; // Quote or Joke Setup (EN)
  PGM_P text2_VI; // Advice or Punchline (VI)
  PGM_P text2_EN; // Advice or Punchline (EN)
};

enum State { E_SEL, E_VIEW, ASK, G_SEL, DIFF_SEL, TETRIS, CROSSY, PINGPONG, PAUSE, GAME_OVER, SETTINGS, SUMMARY, CHECK_DANGER, DANGER_WARN,
             RESET_CONFIRM, SNAKE, DINO, FLAPPY, GADGETS, STOPWATCH, TIMER, DICE, TIMER_ALERT, SET_TIME, GOAL_MENU, ADD_PLAN, CURRENT_PLAN, 
             GOAL_REMINDER, TANK, DASH, BREAKOUT, MAZE, INVADERS, CATCH, ZEN_MODE, SET_SCREEN_TIMEOUT, MUSIC_PLAYER, 
             MORSE_MENU, TEXT_TO_MORSE, MORSE_TO_TEXT, DOOM };

State st = E_SEL;
State returnState = E_SEL; // To know where to return from Settings

// --- Animation Vars ---
int eIdx = 0;
float currentX = 0;
float targetX = 0;
unsigned long lastTick = 0;

// --- Screen Saver Vars ---
unsigned long lastActivityTime = 0;
bool screenOn = true;
bool manualSleep = false;
bool ignoreUntilAllReleased = false;
unsigned long screenTimeoutValue = 30000; // Default 30 seconds (30000 ms)

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
const int pipeW = 10;
const int pipeGap = 28;

// --- New Game Vars ---
float tX, tY, tA, bX_t, bY_t, bDX_t, bDY_t; bool bActive; // Player Tank
float eTX, eTY, eTA, ebX, ebY, ebDX, ebDY; bool eAlive, ebActive; // Enemy Tank
float dashY, dashV; int dashObsX, dashObsType, dashMode; // Mini Dash: Mode 0:Cube, 1:Ship, 2:Wave
float brkX, brkY, brkDX, brkDY, pdX; bool bricks[8][4]; // Breakout
int mzX, mzY; bool maze[16][8]; // Maze
float invX, invBX, invBY; bool invBAct; float invsX[10], invsY[10]; bool invsA[10]; // Invaders
float ctX, ctOX, ctOY; // Catch game
float px_doom, py_doom, pa_doom; // Doom Raycaster

// --- Morse Vars ---
String morseBuffer = "";
String morseInputBuffer = "";
char morsePicker = 'A';
unsigned long morseTimer = 0;

int worldMap[12][12] = {
  {1,1,1,1,1,1,1,1,1,1,1,1},{1,0,0,0,0,0,0,0,0,0,0,1},{1,0,1,1,0,1,1,0,1,1,0,1},{1,0,1,0,0,0,0,0,0,1,0,1},{1,0,0,0,1,0,0,1,0,0,0,1},
  {1,0,1,0,1,0,0,1,0,1,0,1},{1,0,1,0,0,0,0,0,0,1,0,1},{1,0,0,0,1,1,1,1,0,0,0,1},{1,0,1,0,0,0,0,0,0,1,0,1},{1,0,1,1,0,0,0,0,1,1,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,1},{1,1,1,1,1,1,1,1,1,1,1,1}
};

// --- Gadget Vars ---
bool swRun = false;
unsigned long swStart = 0, swElapsed = 0;
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

// --- Music Data ---
const uint16_t rickRollNotes[] PROGMEM = { NOTE_G4, NOTE_A4, NOTE_C5, NOTE_A4, NOTE_E5, NOTE_E5, NOTE_D5, NOTE_G4, NOTE_A4, NOTE_C5, NOTE_A4, NOTE_D5, NOTE_D5, NOTE_C5, NOTE_B4, NOTE_A4, NOTE_G4, NOTE_A4, NOTE_C5, NOTE_A4, NOTE_C5, NOTE_D5, NOTE_B4, NOTE_A4, NOTE_G4, NOTE_G4, NOTE_D5, NOTE_C5 };
const uint16_t rickRollDurs[] PROGMEM = { 125, 125, 125, 125, 250, 250, 500, 125, 125, 125, 125, 250, 250, 500, 125, 125, 125, 125, 250, 250, 250, 250, 250, 250, 500, 250, 250, 500 };

const uint16_t coffinNotes[] PROGMEM = { NOTE_A4, NOTE_A4, NOTE_A4, NOTE_A4, NOTE_C5, NOTE_B4, NOTE_A4, NOTE_G4, NOTE_G4, NOTE_G4, NOTE_G4, NOTE_B4, NOTE_A4, NOTE_G4, NOTE_F4, NOTE_F4, NOTE_F4, NOTE_F4, NOTE_A4, NOTE_G4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_E4, NOTE_E4, NOTE_E4, NOTE_E4, NOTE_E4 };
const uint16_t coffinDurs[] PROGMEM = { 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125 };

const uint16_t nyanNotes[] PROGMEM = { NOTE_FS5, NOTE_GS5, NOTE_DS5, NOTE_DS5, NOTE_B4, NOTE_D5, NOTE_CS5, NOTE_B4, NOTE_AS4, NOTE_B4, NOTE_CS5, NOTE_DS5, NOTE_FS5, NOTE_GS5, NOTE_DS5, NOTE_FS5, NOTE_CS5, NOTE_AS4, NOTE_B4, NOTE_CS5, NOTE_AS4, NOTE_B4, NOTE_CS5, NOTE_DS5 };
const uint16_t nyanDurs[] PROGMEM = { 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 };

struct Song {
  const char* name;
  const uint16_t* notes;
  const uint16_t* durs;
  int length;
};

Song playlist[] = {
  {"RickRoll", rickRollNotes, rickRollDurs, 28},
  {"CoffinDance", coffinNotes, coffinDurs, 28},
  {"Nyan Cat", nyanNotes, nyanDurs, 24}
};

// --- Morse Table ---
const char* const morseAlpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
const char* const morseTable[] = {
  ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--..", "/"
};

String getMorse(char c) {
  if (c >= 'a' && c <= 'z') c -= 32;
  if (c >= 'A' && c <= 'Z') return String(morseTable[c - 'A']);
  if (c == ' ') return "/";
  return "";
}

char fromMorse(String s) {
  for (int i = 0; i < 27; i++) {
    if (s == String(morseTable[i])) return morseAlpha[i];
  }
  return '?';
}

void drawRotRect(int cx, int cy, int size, float a) {
  float s = sin(a), c = cos(a);
  int x[4], y[4];
  int vx[] = {-size/2, size/2, size/2, -size/2}, vy[] = {-size/2, -size/2, size/2, size/2};
  for(int i=0; i<4; i++) {
    x[i] = cx + (vx[i] * c - vy[i] * s);
    y[i] = cy + (vx[i] * s + vy[i] * c);
  }
  for(int i=0; i<4; i++) display.drawLine(x[i], y[i], x[(i+1)%4], y[(i+1)%4], SH110X_WHITE);
}

int currentSongIdx = -1;
int currentNoteIdx = 0;
unsigned long nextNoteTime = 0;
bool isMusicPlaying = false;

// EEPROM Map:
// 0-1: Magic (0xAB, 0xCD)
// 2-3: Tetris HS
// 4-5: Crossy HS
// 6-7: PingPong HS
// 10: Log Head Index (0-29)
// 20-109: Logs (30 entries * 3 bytes: Day(1), Month(1), EmotionID(1))
// 120: Goal Day, 121: Goal Month, 122: Planned(LSB), 123: Done(LSB), 124: Reminder Day, 125: Planned(MSB), 126: Done(MSB)
// 11: Games Only Mode (0/1)
// 114-121: High Scores for CrineMaft, Tank, Dash, Breakout, Maze (Note: Maze HS moved to 128). 130-133: Screen Timeout (unsigned long).

const Msg mV[] PROGMEM = {
  // Universal Jokes & Riddles
  {PSTR("Tại sao sách toán lại buồn?"), PSTR("Why was the math book sad?"), PSTR("Vì nó có quá nhiều vấn đề."), PSTR("Because it had too many problems.")},
  {PSTR("Cái gì có cổ mà không có đầu?"), PSTR("What has a neck but no head?"), PSTR("Đó chính là chiếc áo sơ mi."), PSTR("It is a shirt.")},
  {PSTR("Cái gì càng lau lại càng bẩn?"), PSTR("What gets dirtier the more you wipe?"), PSTR("Đó là chiếc khăn lau bụi."), PSTR("A duster or a rag.")},
  {PSTR("Cái gì có nhiều răng nhưng không cắn?"), PSTR("What has many teeth but never bites?"), PSTR("Đó là chiếc lược của bạn."), PSTR("It is your comb.")},
  {PSTR("Cái gì luôn đi đến mà không bao giờ tới?"), PSTR("What is always coming but never arrives?"), PSTR("Đó là ngày mai."), PSTR("Tomorrow.")},
  {PSTR("Thứ gì luôn ở trước mặt nhưng ta không nhìn thấy?"), PSTR("What is always in front of you but invisible?"), PSTR("Đó chính là tương lai."), PSTR("The future.")},
  {PSTR("Cái gì có thể đi khắp thế giới mà chỉ ở một góc?"), PSTR("What can travel the world while staying in a corner?"), PSTR("Một con tem bưu điện."), PSTR("A postage stamp.")},
  {PSTR("Tại sao xe đạp không tự đứng được?"), PSTR("Why can't a bicycle stand on its own?"), PSTR("Vì nó quá 'mệt' (two-tired)."), PSTR("Because it's two-tired.")},
  {PSTR("Cái gì có cánh mà không biết bay?"), PSTR("What has wings but cannot fly?"), PSTR("Đó là một tòa nhà."), PSTR("A building (wings of a building).")},
  {PSTR("Cái gì càng tối lại càng thấy rõ?"), PSTR("What is seen more clearly the darker it gets?"), PSTR("Đó là những vì sao."), PSTR("The stars.")},
  {PSTR("Con gì đập thì sống, không đập thì chết?"), PSTR("What lives if you beat it, and dies if you stop?"), PSTR("Đó là trái tim."), PSTR("A heart.")},
  {PSTR("Cái gì của bạn nhưng người khác dùng nhiều hơn?"), PSTR("What belongs to you but others use it more?"), PSTR("Chính là tên của bạn."), PSTR("Your name.")},
  {PSTR("Cái gì đi lên mà không bao giờ đi xuống?"), PSTR("What goes up but never comes down?"), PSTR("Đó là số tuổi của bạn."), PSTR("Your age.")},
  {PSTR("Gấu không răng gọi là gì?"), PSTR("What do you call a bear with no teeth?"), PSTR("Gấu kẹo dẻo (Gummy bear)!"), PSTR("A gummy bear!")},
  {PSTR("Cái gì có một mắt nhưng không thể nhìn thấy?"), PSTR("What has one eye but cannot see?"), PSTR("Đó chính là cây kim khâu."), PSTR("It is a sewing needle.")},
  {PSTR("Cái gì bắt đầu bằng chữ T, kết thúc bằng chữ T và bên trong chứa đầy T?"), PSTR("What starts with T, ends with T, and has T in it?"), PSTR("Đó là một chiếc ấm trà (Teapot)."), PSTR("It is a teapot.")},
  {PSTR("Cái gì có thể đi lên ống khói khi đóng, nhưng không thể xuống ống khói khi mở?"), PSTR("What can go up a chimney down but not down a chimney up?"), PSTR("Đó chính là một chiếc ô."), PSTR("An umbrella.")},
  {PSTR("Tôi có lỗ ở trên, ở dưới, ở trái và ở phải. Tôi là ai?"), PSTR("I have holes in my top and bottom, my left and right. What am I?"), PSTR("Tôi là một miếng bọt biển."), PSTR("A sponge.")},
  {PSTR("Cái gì của bạn nhưng bạn lại không bao giờ dùng tới?"), PSTR("What is yours but you never use it?"), PSTR("Đó là cái bóng của chính bạn."), PSTR("Your shadow.")}
};
const Msg mB[] PROGMEM = {
  // Sad - Deep & Comforting
  {PSTR("Dù thế giới có vội vã, bạn vẫn có quyền bước chậm lại."), PSTR("Even if the world is rushing, you have the right to walk slowly."), PSTR("Hãy tử tế với chính mình hôm nay."), PSTR("Be kind to yourself today.")},
  {PSTR("Nỗi buồn chỉ là một cơn mưa giúp tâm hồn nảy mầm."), PSTR("Sadness is just rain that helps the soul bloom."), PSTR("Đừng sợ ướt át, hạt mầm đang lớn lên."), PSTR("Don't fear the damp; the seeds are growing.")},
  {PSTR("Đừng đếm những gì đã mất, hãy trân trọng những gì còn lại."), PSTR("Don't count what you lost; cherish what's still here."), PSTR("Bạn vẫn còn nhiều điều tuyệt vời."), PSTR("You still have many wonderful things.")},
  {PSTR("Những ngôi sao tỏa sáng nhất trong đêm tối nhất."), PSTR("The stars shine brightest in the darkest nights."), PSTR("Đừng ghét bóng tối, nó làm hiện rõ ánh sáng."), PSTR("Don't hate the dark; it reveals the light.")},
  {PSTR("Lối thoát duy nhất là đi xuyên qua nó."), PSTR("The only way out is through."), PSTR("Đừng sợ hãi, hãy cứ bước tiếp."), PSTR("Don't fear, just keep stepping.")},
  {PSTR("Thời gian không chữa lành, nó dạy ta cách sống chung."), PSTR("Time doesn't heal; it teaches us how to live with it."), PSTR("Bạn đang mạnh mẽ hơn mỗi ngày."), PSTR("You are growing stronger every day.")},
  {PSTR("Đôi khi bạn phải buông tay để thấy mình được tự do."), PSTR("Sometimes you have to let go to see that you are free."), PSTR("Buông bỏ không phải là mất mát."), PSTR("Letting go is not losing.")},
  {PSTR("Hoa nở không phải để khoe sắc, mà để hoàn thiện chính mình."), PSTR("Flowers bloom not to show off, but to fulfill themselves."), PSTR("Hãy cứ là chính bạn, dù âm thầm."), PSTR("Just be yourself, even in silence.")},
  {PSTR("Nỗi buồn là cái giá ta trả cho những gì ta từng yêu."), PSTR("Sadness is the price we pay for what we once loved."), PSTR("Tình yêu đó vẫn luôn xứng đáng."), PSTR("That love was always worth it.")},
  {PSTR("Đừng để nỗi buồn của hôm qua làm mờ đi nắng hôm nay."), PSTR("Don't let yesterday's sadness blur today's sun."), PSTR("Mở cửa sổ tâm hồn ra nhé."), PSTR("Open the window of your soul.")},
  {PSTR("Bình yên không phải là không có bão, mà là lặng lẽ giữa cơn bão."), PSTR("Peace is not the absence of storms, but being calm within them."), PSTR("Tìm một khoảng lặng bên trong bạn."), PSTR("Find a quiet space inside you.")},
  {PSTR("Mỗi vết sẹo đều kể một câu chuyện về sự sống sót."), PSTR("Every scar tells a story of survival."), PSTR("Bạn là một chiến binh dũng cảm."), PSTR("You are a brave survivor.")},
  {PSTR("Có những ngày ta chỉ cần tồn tại thôi đã là một kỳ tích."), PSTR("Some days, just existing is a miracle."), PSTR("Cảm ơn bạn vì đã không bỏ cuộc."), PSTR("Thank you for not giving up.")},
  {PSTR("Đừng để một chương tồi tệ làm bạn muốn kết thúc cuốn sách."), PSTR("Don't let a bad chapter make you want to end the book."), PSTR("Vẫn còn rất nhiều trang đẹp phía trước."), PSTR("There are still many beautiful pages ahead.")},
  {PSTR("Nỗi buồn là một phần của hành trình trưởng thành."), PSTR("Sadness is a part of the growth journey."), PSTR("Hãy cứ bước đi, dù là từng bước nhỏ."), PSTR("Just keep walking, even in small steps.")},
  {PSTR("Bạn không cần phải lúc nào cũng tỏ ra mạnh mẽ."), PSTR("You don't always have to be strong."), PSTR("Yếu lòng một chút cũng không sao."), PSTR("It's okay to be weak sometimes.")},
  {PSTR("Nỗi đau hôm nay sẽ trở thành sức mạnh cho ngày mai."), PSTR("Today's pain will become tomorrow's strength."), PSTR("Hãy tin vào khả năng phục hồi của mình."), PSTR("Believe in your resilience.")},
  {PSTR("Mọi vết thương rồi sẽ được chữa lành bởi thời gian."), PSTR("Every wound will be healed by time."), PSTR("Hãy kiên nhẫn với bản thân mình."), PSTR("Be patient with yourself.")}
};
const Msg mK[] PROGMEM = {
  // Crying - Validating & Gentle
  {PSTR("Nước mắt là ngôn ngữ thầm lặng của một trái tim đã quá mạnh mẽ."), PSTR("Tears are the silent language of a heart that's been too strong."), PSTR("Hãy cứ khóc, bạn không cần phải gồng mình."), PSTR("It's okay to cry; you don't have to be tough.")},
  {PSTR("Bầu trời cũng có lúc đổ mưa để trở nên trong xanh hơn."), PSTR("Even the sky needs to rain to become clearer."), PSTR("Sau trận khóc này, lòng bạn sẽ nhẹ nhõm."), PSTR("After this, your heart will feel lighter.")},
  {PSTR("Đừng xin lỗi vì đã khóc. Đó là cách tâm hồn hít thở."), PSTR("Don't apologize for crying. It's how the soul breathes."), PSTR("Cứ để dòng lệ cuốn trôi gánh nặng."), PSTR("Let the tears carry away the burden.")},
  {PSTR("Khóc không phải yếu đuối, đó là sự can đảm để đối diện nỗi đau."), PSTR("Crying isn't weakness; it's courage to face the pain."), PSTR("Bạn rất dũng cảm khi sống thật với mình."), PSTR("You are brave for being true to yourself.")},
  {PSTR("Nước mắt là những lời mà miệng không thể thốt ra."), PSTR("Tears are words that the mouth cannot speak."), PSTR("Hãy để đôi mắt kể câu chuyện của bạn."), PSTR("Let your eyes tell your story.")},
  {PSTR("Khi trái tim quá đầy, nó sẽ tràn ra qua đôi mắt."), PSTR("When the heart is too full, it overflows through the eyes."), PSTR("Đó là một sự giải thoát cần thiết."), PSTR("It is a necessary release.")},
  {PSTR("Mỗi giọt nước mắt rơi xuống là một viên gạch xây nên sự kiên cường."), PSTR("Every tear that falls is a brick building your resilience."), PSTR("Bạn đang xây dựng một tôi mạnh mẽ hơn."), PSTR("You are building a stronger you.")},
  {PSTR("Khóc là để rửa sạch tâm hồn cho những niềm vui mới."), PSTR("Crying is cleansing the soul for new joys."), PSTR("Lau khô mắt và chờ đón điều tốt đẹp."), PSTR("Wipe your eyes and wait for good things.")},
  {PSTR("Đừng kìm nén, sự u uất lâu ngày sẽ làm bạn héo mòn."), PSTR("Don't suppress it; long-held gloom will wither you."), PSTR("Hãy cứ là một dòng sông tuôn chảy."), PSTR("Just be a flowing river.")},
  {PSTR("Khóc xong rồi nhớ uống thêm chút nước nhé."), PSTR("Remember to drink some water after crying."), PSTR("Hãy chăm sóc cơ thể nhỏ bé của bạn."), PSTR("Take care of your little body.")},
  {PSTR("Bạn không cần phải hoàn hảo, bạn chỉ cần là chính mình."), PSTR("You don't have to be perfect; you just have to be you."), PSTR("Và chính mình thì có lúc phải khóc."), PSTR("And 'you' sometimes needs to cry.")},
  {PSTR("Khóc là cách trái tim tìm thấy sự cân bằng."), PSTR("Crying is the heart's way of finding balance."), PSTR("Đừng kìm nén những giọt lệ chân thành."), PSTR("Don't suppress sincere tears.")},
  {PSTR("Nước mắt không làm bạn yếu đi, nó làm bạn nhẹ lòng hơn."), PSTR("Tears don't make you weaker; they make you lighter."), PSTR("Hãy để mọi muộn phiền trôi đi."), PSTR("Let all your sorrows flow away.")},
  {PSTR("Sau mỗi trận mưa, cỏ cây sẽ xanh tốt hơn."), PSTR("After every rain, the grass grows greener."), PSTR("Bạn cũng sẽ trở nên rạng rỡ hơn."), PSTR("You too will become more radiant.")},
  {PSTR("Khóc là ngôn ngữ của những điều không thể nói bằng lời."), PSTR("Crying is the language of things that cannot be said."), PSTR("Trái tim bạn đang được lắng nghe."), PSTR("Your heart is being heard.")},
  {PSTR("Hãy để những giọt nước mắt tưới mát tâm hồn khô héo."), PSTR("Let your tears water a parched soul."), PSTR("Niềm hy vọng mới sẽ nảy mầm."), PSTR("New hope will sprout.")}
};
const Msg mTv[] PROGMEM = {
  // Despair - Powerful & Empowering
  {PSTR("Trời tối nhất là ngay trước lúc bình minh."), PSTR("It's always darkest just before the dawn."), PSTR("Hy vọng đang ở rất gần bạn."), PSTR("Hope is closer than you think.")},
  {PSTR("Khi bạn chạm đáy, con đường duy nhất là đi lên."), PSTR("When you hit rock bottom, the only way is up."), PSTR("Đừng bỏ cuộc, hãy bắt đầu lại từ đây."), PSTR("Don't give up; rebuild from here.")},
  {PSTR("Bạn không cần thấy cả cầu thang, chỉ cần bước bước đầu tiên."), PSTR("You don't have to see the whole staircase, just take the first step."), PSTR("Hãy tin vào đôi chân của mình."), PSTR("Trust your own feet.")},
  {PSTR("Ngay cả trong tro tàn, sự sống vẫn có thể bắt đầu."), PSTR("Even in ashes, life can begin again."), PSTR("Mọi kết thúc đều là một khởi đầu mới."), PSTR("Every end is a new beginning.")},
  {PSTR("Nghịch cảnh không phải để quật ngã, mà để rèn giũa bạn."), PSTR("Adversity is not to strike you down, but to forge you."), PSTR("Bạn là thanh kiếm thép quý giá."), PSTR("You are a precious steel sword.")},
  {PSTR("Dù không ai tin tưởng, bạn vẫn phải tin vào chính mình."), PSTR("Even if no one believes, you must believe in yourself."), PSTR("Ngọn lửa bên trong bạn mạnh hơn bão tố."), PSTR("The fire inside you is stronger than the storm.")},
  {PSTR("Mỗi thất bại là một bài học đắt giá cho sự thành công."), PSTR("Every failure is a costly lesson for success."), PSTR("Bạn không thua, bạn chỉ đang học thôi."), PSTR("You didn't lose; you are just learning.")},
  {PSTR("Thế giới có thể quay lưng, nhưng bản thân bạn thì không."), PSTR("The world may turn its back, but you must not."), PSTR("Hãy là người bạn tốt nhất của chính mình."), PSTR("Be your own best friend.")},
  {PSTR("Giấc mơ bị trì hoãn không có nghĩa là giấc mơ bị từ chối."), PSTR("A dream deferred is not a dream denied."), PSTR("Hãy kiên trì thêm một chút nữa thôi."), PSTR("Just persevere a little bit more.")},
  {PSTR("Bạn được sinh ra với đôi cánh, đừng bò qua cuộc đời."), PSTR("You were born with wings; don't crawl through life."), PSTR("Hãy đứng dậy và thử dang rộng cánh."), PSTR("Stand up and try to spread your wings.")},
  {PSTR("Những điều vĩ đại thường bắt đầu từ những việc nhỏ bé."), PSTR("Great things often start from small things."), PSTR("Đừng coi thường sự nỗ lực âm thầm."), PSTR("Don't despise your silent efforts.")},
  {PSTR("Bạn là người duy nhất có chìa khóa cho hạnh phúc của mình."), PSTR("You are the only one with the key to your happiness."), PSTR("Don't give that key to anyone else."), PSTR("Đừng đưa nó cho bất kỳ ai khác cầm.")},
  {PSTR("Khi bạn cảm thấy muốn bỏ cuộc, hãy nhớ lý do bạn bắt đầu."), PSTR("When you feel like quitting, remember why you started."), PSTR("Mục tiêu của bạn vẫn đang chờ phía trước."), PSTR("Your goal is still waiting ahead.")},
  {PSTR("Chỉ cần bạn không dừng lại, việc đi chậm bao nhiêu không quan trọng."), PSTR("It doesn't matter how slowly you go as long as you don't stop."), PSTR("Kiên trì là chìa khóa của thành công."), PSTR("Persistence is the key to success.")},
  {PSTR("Đừng để những khó khăn hôm nay làm lu mờ ước mơ mai sau."), PSTR("Don't let today's difficulties overshadow tomorrow's dreams."), PSTR("Hãy vững tin vào con đường mình chọn."), PSTR("Believe firmly in your chosen path.")},
  {PSTR("Bạn sinh ra để tỏa sáng, không phải để tan biến."), PSTR("You were born to shine, not to fade away."), PSTR("Sức mạnh của bạn nằm ở bên trong."), PSTR("Your strength lies within.")},
  {PSTR("Mỗi ngày mới là một cơ hội để thay đổi tất cả."), PSTR("Every new day is a chance to change everything."), PSTR("Hãy nắm bắt lấy nó bằng cả trái tim."), PSTR("Seize it with all your heart.")}
};
const Msg mTg[] PROGMEM = {
  // Angry - Wise & Calming
  {PSTR("Một trái tim giận dữ sẽ tự thiêu rụi sự bình yên của chính nó."), PSTR("An angry heart consumes its own peace."), PSTR("Hít thở sâu, đừng để cơn giận cầm lái."), PSTR("Breathe deep; don't let anger drive.")},
  {PSTR("Lời nói lúc giận dữ là lời nói ta hối tiếc nhất."), PSTR("Words spoken in anger are the ones we regret most."), PSTR("Im lặng là sức mạnh lớn nhất lúc này."), PSTR("Silence is your greatest strength right now.")},
  {PSTR("Giữ cơn giận giống như cầm hòn than nóng định ném người khác."), PSTR("Holding anger is like holding a hot coal to throw at others."), PSTR("Bạn sẽ là người bị bỏng đầu tiên."), PSTR("You will be the first one burned.")},
  {PSTR("Sự tha thứ là món quà bạn dành cho chính mình."), PSTR("Forgiveness is the gift you give yourself."), PSTR("Buông bỏ là cách tốt nhất để trả thù."), PSTR("Letting go is the best revenge.")},
  {PSTR("Đừng để hành động của người khác phá hủy giá trị của bạn."), PSTR("Don't let others' actions destroy your value."), PSTR("Bạn cao thượng hơn những điều nhỏ nhen này."), PSTR("You are nobler than these petty things.")},
  {PSTR("Cơn giận là một cơn bão, rồi nó cũng sẽ tan biến thôi."), PSTR("Anger is a storm; it too will pass away."), PSTR("Hãy là ngọn núi đứng vững trước gió."), PSTR("Be the mountain standing firm in the wind.")},
  {PSTR("Nếu bạn đúng, bạn không cần phải nổi giận."), PSTR("If you are right, you don't need to be angry."), PSTR("Nếu bạn sai, bạn không có quyền nổi giận."), PSTR("If you are wrong, you have no right to be angry.")},
  {PSTR("Kiểm soát cơn giận là kiểm soát số phận của chính mình."), PSTR("Controlling anger is controlling your own destiny."), PSTR("Hãy làm chủ bản thân mình nhé."), PSTR("Take mastery over yourself.")},
  {PSTR("Một phút nhẫn nhịn giúp bạn tránh được trăm ngày u sầu."), PSTR("One minute of patience saves a hundred days of sorrow."), PSTR("Hãy đếm đến mười trước khi lên tiếng."), PSTR("Count to ten before you speak.")},
  {PSTR("Trả thù chỉ tạo ra một vòng lặp đau khổ vô tận."), PSTR("Revenge only creates an endless loop of suffering."), PSTR("Hãy là người cắt đứt cái vòng lặp đó."), PSTR("Be the one to break that cycle.")},
  {PSTR("Tử tế là câu trả lời mạnh mẽ nhất cho sự thô lỗ."), PSTR("Kindness is the strongest answer to rudeness."), PSTR("Hãy dùng ánh sáng để xua tan bóng tối."), PSTR("Use light to dispel the darkness.")},
  {PSTR("Cơn giận giống như một ngọn lửa, nếu không kiểm soát nó sẽ thiêu rụi bạn."), PSTR("Anger is like a fire; if you don't control it, it will burn you."), PSTR("Hãy làm chủ cảm xúc của mình."), PSTR("Take mastery of your emotions.")},
  {PSTR("Đừng để lời nói lúc nóng giận làm tổn thương người bạn yêu."), PSTR("Don't let words spoken in anger hurt the ones you love."), PSTR("Sự hối tiếc thường đến sau cơn giận."), PSTR("Regret often follows anger.")},
  {PSTR("Bình tĩnh là một loại sức mạnh tĩnh lặng."), PSTR("Calmness is a type of silent power."), PSTR("Hãy dùng sự điềm tĩnh để đối diện với mọi việc."), PSTR("Use composure to face everything.")},
  {PSTR("Kẻ thù lớn nhất không phải người khác, mà là cơn giận bên trong."), PSTR("The biggest enemy is not others, but the anger within."), PSTR("Hãy chiến thắng chính bản thân mình."), PSTR("Vanquish yourself first.")},
  {PSTR("Sự bao dung sẽ dập tắt mọi ngọn lửa hận thù."), PSTR("Tolerance will extinguish every flame of hatred."), PSTR("Hãy mở rộng lòng mình ra nhé."), PSTR("Open your heart wider.")}
};
const Msg mS[] PROGMEM = {
  // Fear - Bravery & Perspective
  {PSTR("Sợ hãi là một bóng ma, nó chỉ sống nếu bạn tin vào nó."), PSTR("Fear is a ghost; it only lives if you believe in it."), PSTR("Bạn mạnh mẽ hơn ảo giác này."), PSTR("You are stronger than this illusion.")},
  {PSTR("Can đảm không phải là không sợ, mà là làm dù vẫn sợ."), PSTR("Courage isn't the absence of fear, but doing it anyway."), PSTR("Hãy thử một bước nhỏ thôi."), PSTR("Just take one small step.")},
  {PSTR("Mọi điều bạn muốn đều nằm ở bên kia của sự sợ hãi."), PSTR("Everything you want is on the other side of fear."), PSTR("Cánh cửa đang mở, hãy bước qua."), PSTR("The door is open; walk through.")},
  {PSTR("Nỗi sợ là một phản xạ, nhưng can đảm là một lựa chọn."), PSTR("Fear is a reaction, but courage is a choice."), PSTR("Hãy chọn dũng cảm vào giây phút này."), PSTR("Choose courage at this very moment.")},
  {PSTR("Đừng sợ bóng tối, vì đó là nơi bạn thấy được những vì sao."), PSTR("Don't fear the dark, for that's where you see the stars."), PSTR("Ánh sáng bên trong bạn sẽ dẫn lối."), PSTR("The light inside you will lead the way.")},
  {PSTR("Sợ thất bại là rào cản lớn nhất của sự thành công."), PSTR("Fear of failure is the greatest barrier to success."), PSTR("Thất bại chỉ là một cách để bắt đầu lại."), PSTR("Failure is just a way to start over.")},
  {PSTR("Nỗi sợ chỉ mạnh khi bạn trốn chạy nó."), PSTR("Fear is only strong when you run away from it."), PSTR("Hãy quay lại và nhìn thẳng vào nó."), PSTR("Turn around and look directly at it.")},
  {PSTR("Bạn không cô đơn, ai cũng có nỗi sợ riêng của mình."), PSTR("You are not alone; everyone has their own fears."), PSTR("Dũng cảm là cùng nhau bước tiếp."), PSTR("Bravery is moving forward together.")},
  {PSTR("Quái vật trong đầu luôn đáng sợ hơn quái vật ngoài đời."), PSTR("The monster in your head is scarier than the one in real life."), PSTR("Đừng để trí tưởng tượng đánh lừa bạn."), PSTR("Don't let your imagination trick you.")},
  {PSTR("Mọi rào cản đều có thể bị phá vỡ bởi sự kiên trì."), PSTR("Every barrier can be broken by persistence."), PSTR("Hãy cứ gõ cửa cho đến khi nó mở."), PSTR("Keep knocking until the door opens.")},
  {PSTR("Sợ hãi chỉ là một bài kiểm tra cho ý chí của bạn."), PSTR("Fear is just a test for your will."), PSTR("Và bạn chắc chắn sẽ vượt qua nó."), PSTR("And you will surely pass it.")},
  {PSTR("Hãy tin rằng bạn được bảo vệ bởi những điều tốt đẹp."), PSTR("Believe that you are protected by good things."), PSTR("Sự an tâm nằm ở niềm tin của bạn."), PSTR("Peace of mind lies in your faith.")},
  {PSTR("Nỗi sợ chỉ là một rào cản ảo mà tâm trí tạo ra."), PSTR("Fear is just a virtual barrier created by the mind."), PSTR("Hãy can đảm bước xuyên qua nó."), PSTR("Step courageously through it.")},
  {PSTR("Làm những điều bạn sợ, và nỗi sợ sẽ biến mất."), PSTR("Do the things you fear, and fear will vanish."), PSTR("Hành động là phương thuốc tốt nhất."), PSTR("Action is the best medicine.")},
  {PSTR("Đừng để nỗi sợ thất bại ngăn cản bạn vươn tới thành công."), PSTR("Don't let the fear of failure stop you from reaching success."), PSTR("Thất bại chỉ là một bước đệm."), PSTR("Failure is just a stepping stone.")},
  {PSTR("Bạn mạnh mẽ hơn những gì nỗi sợ nói với bạn."), PSTR("You are stronger than what fear tells you."), PSTR("Hãy lắng nghe tiếng nói của trái tim."), PSTR("Listen to the voice of your heart.")},
  {PSTR("Sợ hãi là một phần của cuộc sống, nhưng đừng để nó làm chủ."), PSTR("Fear is a part of life, but don't let it rule you."), PSTR("Bạn là người cầm lái cuộc đời mình."), PSTR("You are the driver of your life.")}
};

const Msg mLA[] PROGMEM = {
  // Anxious - Present & Grounding
  {PSTR("Lo lắng không làm vơi bớt nỗi đau ngày mai, nó chỉ làm cạn sức lực hôm nay."), PSTR("Worrying doesn't empty tomorrow's sorrow; it empties today's strength."), PSTR("Hãy hít thở, bạn chỉ cần lo cho hiện tại."), PSTR("Just breathe; focus only on the now.")},
  {PSTR("Đừng để tiếng ồn của ngày mai làm lu mờ bản nhạc của hôm nay."), PSTR("Don't let the noise of tomorrow drown out the music of today."), PSTR("Mọi chuyện rồi sẽ ổn thôi."), PSTR("Everything will be okay.")},
  {PSTR("Bạn không cần phải kiểm soát mọi thứ để được bình yên."), PSTR("You don't have to control everything to be at peace."), PSTR("Thả lỏng đôi vai và hít thở sâu nhé."), PSTR("Relax your shoulders and breathe deep.")},
  {PSTR("Hít vào bình yên, thở ra lo lắng."), PSTR("Inhale peace, exhale worry."), PSTR("Bạn đang an toàn ở đây, ngay lúc này."), PSTR("You are safe here, right now.")},
  {PSTR("Suy nghĩ quá mức là nghệ thuật tạo ra vấn đề không tồn tại."), PSTR("Overthinking is the art of creating problems that don't exist."), PSTR("Đừng để tâm trí đánh lừa bạn nhé."), PSTR("Don't let your mind trick you.")},
  {PSTR("Bạn không phải là những suy nghĩ tiêu cực của mình."), PSTR("You are not your negative thoughts."), PSTR("Bạn là bầu trời, suy nghĩ chỉ là mây."), PSTR("You are the sky; thoughts are just clouds.")},
  {PSTR("Tập trung vào bước chân này, thay vì cả quãng đường dài."), PSTR("Focus on this single step, instead of the long road."), PSTR("Mọi thứ sẽ dần hiện rõ thôi."), PSTR("Everything will gradually become clear.")},
  {PSTR("Hiện tại là nơi duy nhất bạn thực sự sống."), PSTR("The present is the only place you truly live."), PSTR("Hãy quay về với nhịp thở của mình."), PSTR("Come back to the rhythm of your breath.")},
  {PSTR("Đừng lo về việc phải hoàn hảo, hãy cứ là chính mình."), PSTR("Don't worry about being perfect; just be yourself."), PSTR("Sự chân thật mang lại sự bình an."), PSTR("Authenticity brings peace.")},
  {PSTR("Thế giới vẫn quay dù bạn có lo lắng hay không."), PSTR("The world keeps spinning whether you worry or not."), PSTR("Hãy cho phép mình được nghỉ ngơi."), PSTR("Allow yourself to take a rest.")},
  {PSTR("Mọi bồn chồn rồi sẽ tan biến như sương mù buổi sáng."), PSTR("All restlessness will vanish like morning mist."), PSTR("Nắng sớm đang chờ đón bạn phía trước."), PSTR("The morning sun is waiting for you ahead.")},
  {PSTR("Bạn đã vượt qua mọi khó khăn trước đây, lần này cũng vậy."), PSTR("You've overcome all past difficulties; this time is no different."), PSTR("Tin vào sự kiên cường của chính mình."), PSTR("Trust in your own resilience.")},
  {PSTR("Hãy tập trung vào hơi thở, chỉ có hiện tại mới là thật."), PSTR("Focus on your breath; only the present is real."), PSTR("Lo lắng về tương lai chỉ là ảo giác."), PSTR("Worrying about the future is an illusion.")},
  {PSTR("Bạn không cần phải biết tất cả các câu trả lời ngay bây giờ."), PSTR("You don't need to have all the answers right now."), PSTR("Mọi thứ sẽ dần sáng tỏ theo thời gian."), PSTR("Everything will clarify in time.")},
  {PSTR("Hãy đối xử với bản thân bằng sự dịu dàng như với một người bạn."), PSTR("Treat yourself with gentleness as you would a friend."), PSTR("Bạn xứng đáng được bình yên."), PSTR("You deserve peace.")},
  {PSTR("Lo âu là một đám mây, nó sẽ trôi qua nếu bạn để yên."), PSTR("Anxiety is a cloud; it will pass if you let it be."), PSTR("Đừng cố xua đuổi, hãy cứ quan sát nó."), PSTR("Don't try to chase it; just observe it.")},
  {PSTR("Mọi chuyện rồi sẽ đâu vào đấy, hãy tin vào sự sắp đặt."), PSTR("Everything will fall into place; trust the arrangement."), PSTR("Bạn đang được che chở."), PSTR("You are being protected.")}
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
  else if(gameChoice == 6) { st=TANK; sc=0; tX=32; tY=32; tA=0; bActive=false; eAlive=true; eTX=96; eTY=32; eTA=3.14; highScore=getHighScore(6); }
  else if(gameChoice == 7) { st=DASH; sc=0; dashY=54; dashV=0; dashObsX=128; dashMode=0; highScore=getHighScore(7); }
  else if(gameChoice == 8) { st=BREAKOUT; sc=0; brkX=64; brkY=40; brkDX=1.5; brkDY=-1.5; pdX=(SW/2)-10; for(int i=0;i<8;i++)for(int j=0;j<4;j++)bricks[i][j]=true; highScore=getHighScore(8); }
  else if(gameChoice == 9) { st=MAZE; sc=0; mzX=1; mzY=1; for(int i=0;i<16;i++)for(int j=0;j<8;j++)maze[i][j]=(random(10)>7); maze[1][1]=0; maze[14][6]=0; highScore=getHighScore(9); }
  else if(gameChoice == 10) { st=INVADERS; sc=0; invX=60; invBAct=false; for(int i=0;i<10;i++){invsX[i]=(i%5)*20+10; invsY[i]=(i/5)*12+10; invsA[i]=true;} highScore=getHighScore(10); }
  else if(gameChoice == 11) { st=CATCH; sc=0; ctX=60; ctOX=random(120); ctOY=0; highScore=getHighScore(11); }
  else if(gameChoice == 12) { st=DOOM; px_doom=2.0; py_doom=2.0; pa_doom=0.0; }
  lastGame = st;
}

int getHSOffset(int gameIdx) {
  if (gameIdx == 4) return 110; // Dino HS
  if (gameIdx == 5) return 112; // Flappy Bird HS
  if (gameIdx == 6) return 116; // Tank HS
  if (gameIdx == 7) return 118; // Dash HS
  if (gameIdx == 8) return 120; // Breakout HS
  if (gameIdx == 9) return 128; // Maze HS
  if (gameIdx == 10) return 136; // Invaders HS
  if (gameIdx == 11) return 138; // Catch HS
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
  pinMode(PIN_SLP, INPUT_PULLUP);
  pinMode(PIN_VCC, OUTPUT);
  digitalWrite(PIN_VCC, HIGH); // Power on screen initially
  delay(100); // Give screen time to boot
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
  if (language > 1) language = 0; // Fix invalid language data
  // Sanitize Log Head Index to prevent crashes
  
  // Load Screen Timeout Value
  screenTimeoutValue = (EEPROM.read(130) << 24) | (EEPROM.read(131) << 16) | (EEPROM.read(132) << 8) | EEPROM.read(133);
  if (screenTimeoutValue == 0 || screenTimeoutValue > 300000) screenTimeoutValue = 30000; // Default to 30s, max 5 min

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
  nSlp.setDebounceTime(50); // Small debounce for physical sleep button
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
  lastActivityTime = millis();

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
  nL.loop(); nX.loop(); nTr.loop(); nP.loop(); nOk.loop(); nSlp.loop();

  bool anyButtonDown = (digitalRead(PIN_UP) == LOW || digitalRead(PIN_DOWN) == LOW || 
                        digitalRead(PIN_LEFT) == LOW || digitalRead(PIN_RIGHT) == LOW || 
                        digitalRead(PIN_OK) == LOW || digitalRead(PIN_SLP) == LOW);

  // Handle Manual Sleep/Wake Button (Phone-like behavior)
  if (nSlp.isPressed()) {
    if (!screenOn && manualSleep) {
      // Manual wake up from sleep button
      manualSleep = false;
      screenOn = true;
      digitalWrite(PIN_VCC, HIGH);
      delay(50); // Stabilization
      display.begin(0x3C, true);
      display.setTextColor(SH110X_WHITE);
      display.setTextWrap(false);
      lastActivityTime = millis();
      ignoreUntilAllReleased = true;
    } else if (screenOn) {
      // Manual sleep via button
      screenOn = false;
      manualSleep = true;
      noTone(BZ);
      display.oled_command(SH110X_DISPLAYOFF); // Tell the controller to shut down charge pumps
      delay(10); // Short delay for discharge
      digitalWrite(PIN_VCC, LOW);
    }
    return;
  }

  // Handle Screen Wake
  if (!screenOn) {
    if (!manualSleep && anyButtonDown) {
      // Restore physical power
      digitalWrite(PIN_VCC, HIGH);
      delay(50); // Wait for OLED controller to stabilize

      // Re-initialize display settings lost during power cut
      display.begin(0x3C, true);
      display.setTextColor(SH110X_WHITE);
      display.setTextWrap(false);
      
      screenOn = true;
      lastActivityTime = millis();
      ignoreUntilAllReleased = true; // Don't trigger action on wake-up press
    }
    return;
  }

  // Ignore input until user releases the wake-up button
  if (ignoreUntilAllReleased) {
    if (!anyButtonDown) ignoreUntilAllReleased = false;
    // Consume transition flags while waiting for release
    nL.isPressed(); nX.isPressed(); nTr.isPressed(); nP.isPressed(); nOk.isPressed(); nSlp.isPressed();
    return;
  }

  // Handle Inactivity Timeout
  if (anyButtonDown) {
    lastActivityTime = millis();
  } else if (millis() - lastActivityTime > screenTimeoutValue) {
    screenOn = false;
    manualSleep = false; // This was an auto-timeout, not manual
    display.oled_command(SH110X_DISPLAYOFF); // Graceful software shutdown
    delay(10);
    digitalWrite(PIN_VCC, LOW); // Physically cut power
    noTone(BZ); // Stop any active buzzer alerts
    return;
  }

  // Note: Fading the screen off is not implemented due to the nature of monochrome OLEDs
  // and potential complexities/risks (e.g., burn-in from partial updates or complex PWM)
  // with the current display library. A simple on/off is safer for screen longevity.


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
        int textOff = (ic[i].length() * 9);
        if (language == 0) myfont.print(xP + textOff - (myfont.getLength((char*)name.c_str()) / 2), 52, (char*)name.c_str(), SH110X_WHITE);
        else { display.setTextSize(1); display.setCursor(xP + textOff - (name.length() * 3), 52); display.print(name); }
      }
    }

    if (nTr.isPressed() && eIdx > 0) { eIdx--; targetX -= 100; beep(800, 20); }
    if (nP.isPressed() && eIdx < 12) { eIdx++; targetX += 100; beep(800, 20); }
    
    if (nOk.getState() == LOW) { // Button is being held
        if (holdStart == 0) holdStart = millis();
        unsigned long dur = millis() - holdStart;

        if (dur >= 1000) { // Held for 1 second
            if (eIdx == 12) { st = RESET_CONFIRM; mIdx = 0; }
            else if (eIdx == 11) { st = SETTINGS; mIdx = 0; adjustTime = now; setIdx = 0; returnState = E_SEL; }
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
      st = E_SEL;
      beep(1500, 100);
    }
  }

  else if (st == G_SEL) {
    display.clearDisplay(); 
    if (language == 0) myfont.print((SW - myfont.getLength("Chọn game:"))/2, 0, "Chọn game:", SH110X_WHITE);
    else { display.setCursor(28, 0); display.println("SELECT GAME:"); }
    
    const char* gNames[] = {"TETRIS", "CROSSY", "PINGPONG", "SNAKE", "DINO", "FLAPPY", "TANK", "DASH", "BREAKOUT", "MAZE", "INVADERS", "CATCH"};
    const char* exitName = (language==0) ? "Thoát" : "EXIT";
    int top = (mIdx > 4) ? mIdx - 4 : 0; // Show more items on screen or scroll sooner
    for(int i=0; i<13; i++) {
      int y = 12 + (i - top) * 9;
      if(y < 12) continue;
      if(y < 64) {
        if (language == 0) {
           myfont.print(10, y, (mIdx == i) ? "> " : "  ", SH110X_WHITE);
           myfont.print(22, y, (char*)((i < 12) ? gNames[i] : exitName), SH110X_WHITE);
        } else {
           display.setCursor(10, y);
           if(mIdx == i) display.print("> "); else display.print("  ");
           if (i < 12) display.println(gNames[i]); else display.println(exitName);
        }
      }
    }
    display.display();
    if(nL.isPressed()) { mIdx=(mIdx-1+13)%13; beep(600, 20); }
    if(nX.isPressed()) { mIdx=(mIdx+1)%13; beep(600, 20); }
    if(nOk.isPressed()) {
      if (mIdx == 12) { // THOAT
        st = E_SEL;
        beep(1500, 50);
      } else {
        gameChoice = mIdx;
        st = DIFF_SEL;
        mIdx = difficulty;
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
    int top = (mIdx > 3) ? mIdx - 3 : 0;
    for(int i=0; i<3; i++) {
      int y = 18 + (i - top) * 12;
      if (y < 12 || y >= 64) continue;
      if (language == 0) {
        myfont.print(10, y, (mIdx == i) ? "> " : "  ", SH110X_WHITE);
        myfont.print(22, y, (char*)dNamesVI[i], SH110X_WHITE);
      } else {
        display.setCursor(10, y);
        if(mIdx == i) display.print("> "); else display.print("  ");
        display.print(dNamesEN[i]);
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
    int fS = (digitalRead(PIN_DOWN) == LOW) ? 50 : baseSpeed;
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
    display.setCursor(0,0); display.print(sc); display.print(" HI:"); display.print(highScore);
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
    if (pipeX < (SW/2 + 3) && pipeX + pipeW > (SW/2 - 3)) { // Tightened hitbox
        if (fbY + 1 < pipeH || fbY + 7 > pipeH + pipeGap) collision = true;
    }

    if (collision) { saveHighScore(5, sc); playGameOver(); st = GAME_OVER; 
    }

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

  else if (st == TANK) {
    display.clearDisplay();
    // Draw Player Tank (NES Style)
    display.drawRect(tX-4, tY-4, 9, 9, SH110X_WHITE); // Body
    display.drawLine(tX, tY, tX+cos(tA)*8, tY+sin(tA)*8, SH110X_WHITE); // Barrel
    if(abs(cos(tA)) > abs(sin(tA))) { display.drawFastHLine(tX-4, tY-6, 9, SH110X_WHITE); display.drawFastHLine(tX-4, tY+5, 9, SH110X_WHITE); }
    else { display.drawFastVLine(tX-6, tY-4, 9, SH110X_WHITE); display.drawFastVLine(tX+5, tY-4, 9, SH110X_WHITE); }
    
    // Enemy Tank Logic
    if(eAlive) {
      display.drawRect(eTX-4, eTY-4, 9, 9, SH110X_WHITE); 
      display.drawLine(eTX, eTY, eTX+cos(eTA)*8, eTY+sin(eTA)*8, SH110X_WHITE);
      // Enemy Movement
      eTX += cos(eTA) * 0.4; eTY += sin(eTA) * 0.4;
      if(eTX<5||eTX>SW-5||eTY<5||eTY>SH-5) eTA += 3.14; // Bounce
      if(random(100)==0) eTA += (random(0,3)-1)*1.57; // Random turns
      // Enemy Shoot AI
      if(!ebActive && random(50)==0) { ebActive=true; ebX=eTX; ebY=eTY; ebDX=cos(eTA)*2; ebDY=sin(eTA)*2; }
      // Collision Bullet vs Enemy
      if(bActive && abs(bX_t-eTX)<6 && abs(bY_t-eTY)<6) { eAlive=false; bActive=false; sc+=10; beep(1200, 50); }
    } else if(random(200)==0) { eAlive=true; eTX=random(20,100); eTY=random(20,50); }
    else if(random(100)==0) { eAlive=true; eTX=random(20,100); eTY=random(20,50); } // Spawn faster
    if(ebActive) {
      display.drawPixel(ebX, ebY, SH110X_WHITE);
      ebX += ebDX; ebY += ebDY;
      if(ebX<0||ebX>SW||ebY<0||ebY>SH) ebActive=false;
      if(abs(ebX-tX)<5 && abs(ebY-tY)<5) { ebActive = false; saveHighScore(6, sc); st=GAME_OVER; playGameOver(); }
    }

    if(bActive) {
      display.drawPixel(bX_t, bY_t, SH110X_WHITE);
      bX_t += bDX_t; bY_t += bDY_t;
      if(bX_t<0||bX_t>SW||bY_t<0||bY_t>SH) bActive=false;
    }
    if (sc > highScore) saveHighScore(6, sc);
    display.setCursor(0,0); display.print(sc); display.print(" HI:"); display.print(highScore);
    display.display();
    if(nTr.getState()==LOW) tA -= 0.1; if(nP.getState()==LOW) tA += 0.1;
    
    bool shooting = false;
    if(nX.isPressed() && !bActive) { bActive=true; bX_t=tX; bY_t=tY; bDX_t=cos(tA)*3; bDY_t=sin(tA)*3; beep(400, 20); shooting=true; }
    
    if(!shooting) {
      if(nL.getState()==LOW) { tX+=cos(tA); tY+=sin(tA); }
      else if(nX.getState()==LOW) { tX-=cos(tA); tY-=sin(tA); }
    }
    if(nOk.isPressed()) { st = PAUSE; pauseOpt = 0; }
  }

  else if (st == DASH) {
    display.clearDisplay();
    static float dashRot = 0;
    
    // --- Progress Calculation ---
    int progress = sc % 100; // Percentage of the current 100-point "level"

    // --- Cube Physics ---
    bool onGround = (dashY >= 54);
    bool onCube = (dashObsType == 1 && dashObsX < 28 && dashObsX + 8 > 20 && dashY + 8 <= 55 && dashY + 8 >= 50 && dashV >= 0);

    if (onCube) { dashY = 46; dashV = 0; onGround = true; }
    else { dashY += dashV; dashV += 0.8; }
    
    if (dashY > 54) { dashY = 54; dashV = 0; onGround = true; }
    if (nL.isPressed() && onGround) { dashV = -6.5; beep(600, 20); }
    if (!onGround) dashRot += 0.25; else dashRot = 0;

    // --- Collisions ---
    // Side hit on platform
    if(dashObsType == 1 && dashObsX < 22 && dashObsX > 18 && dashY > 46) { saveHighScore(7, sc); st = GAME_OVER; playGameOver(); }
    // Spike Collision (Spike tip is at y=52, ground at 62)
    if(dashObsType == 0 && dashObsX < 28 && dashObsX + 10 > 20 && dashY + 8 > 52) { saveHighScore(7, sc); st = GAME_OVER; playGameOver(); }

    float speed = (3.5 + sc / 15.0);
    dashObsX -= speed;
    if(dashObsX < -15) { dashObsX = 128; sc++; dashObsType = random(2); }

    // --- Drawing ---
    // UI: Progress Bar and Percentage
    display.drawRect(0, 0, SW, 3, SH110X_WHITE);
    display.fillRect(0, 0, map(progress, 0, 100, 0, SW), 3, SH110X_WHITE);
    display.setCursor(SW/2 - 10, 5); display.print(progress); display.print("%");

    // Player
    drawRotRect(24, (int)dashY + 4, 8, dashRot); 

    // Obstacles
    if(dashObsType == 0) display.fillTriangle(dashObsX, 62, dashObsX+5, 52, dashObsX+10, 62, SH110X_WHITE); // Spike
    else display.fillRect(dashObsX, 54, 8, 8, SH110X_WHITE); // Cube

    // Ground and Scrolling Detail
    display.drawFastHLine(0, 62, 128, SH110X_WHITE);
    for(int i=0; i<128; i+=20) {
      int gx = (i - (int)(millis()/15) % 20);
      display.drawFastVLine(gx, 62, 2, SH110X_WHITE);
    }

    display.setCursor(0, 5); display.print("SC:"); display.print(sc);

    display.display();
    if(nOk.isPressed()) { st = PAUSE; pauseOpt = 0; }
  }

  else if (st == INVADERS) {
    display.clearDisplay();
    display.fillRect(invX, 58, 12, 5, SH110X_WHITE);
    if (nTr.getState()==LOW && invX > 0) invX -= 2;
    if (nP.getState()==LOW && invX < 116) invX += 2;
    if (nL.isPressed() && !invBAct) { invBAct=true; invBX=invX+6; invBY=58; beep(400,10); }
    
    if (invBAct) {
      display.fillRect(invBX, invBY, 2, 4, SH110X_WHITE);
      invBY -= 3; if (invBY < 0) invBAct=false;
      for(int i=0; i<10; i++) if(invsA[i] && abs(invBX-invsX[i])<6 && abs(invBY-invsY[i])<6) { invsA[i]=false; invBAct=false; sc+=10; beep(800,20); }
    }
    
    bool allDead = true;
    for(int i=0; i<10; i++) if(invsA[i]) {
      allDead = false;
      display.drawRect(invsX[i]-4, invsY[i]-4, 8, 8, SH110X_WHITE);
      invsX[i] += sin(millis()*0.002)*0.5;
      if (invsY[i] > 50) { saveHighScore(10, sc); st=GAME_OVER; playGameOver(); }
    }
    if (allDead) { for(int i=0; i<10; i++){invsX[i]=(i%5)*20+10; invsY[i]=(i/5)*12+10; invsA[i]=true;} }

    display.setCursor(0,0); display.print(sc); display.print(" HI:"); display.print(highScore);
    display.display();
    if(nOk.isPressed()) { st = PAUSE; pauseOpt = 0; }
  }

  else if (st == CATCH) {
    display.clearDisplay();
    display.drawFastHLine(ctX, 60, 15, SH110X_WHITE); // Basket
    if (nTr.getState()==LOW && ctX > 0) ctX -= 5;
    if (nP.getState()==LOW && ctX < 113) ctX += 5;
    
    display.fillCircle(ctOX, ctOY, 3, SH110X_WHITE);
    ctOY += 1.5 + (sc/50.0);
    
    if (ctOY + 3 >= 60) {
      if (ctOX + 3 >= ctX && ctOX - 3 <= ctX+15) { sc += 10; ctOY = 0; ctOX = random(120); beep(1000, 20); }
      else { saveHighScore(12, sc); st=GAME_OVER; playGameOver(); }
    }
    
    display.setCursor(0,0); display.print(sc); display.print(" HI:"); display.print(highScore);
    display.display();
    if(nOk.isPressed()) { st = PAUSE; pauseOpt = 0; }
  }

  else if (st == BREAKOUT) {
    display.clearDisplay();
    display.fillRect(pdX, 60, 20, 3, SH110X_WHITE);
    display.fillCircle(brkX, brkY, 2, SH110X_WHITE); // Moved higher
    display.setCursor(0, 50); display.print(sc); display.print(" HI:"); display.print(highScore);
    for(int i=0; i<8; i++) for(int j=0; j<4; j++) if(bricks[i][j]) {
      display.drawRect(i*16+2, j*8+2, 13, 6, SH110X_WHITE);
      if(brkX>i*16+2 && brkX<i*16+15 && brkY>j*8+2 && brkY<j*8+8) { bricks[i][j]=0; brkDY*=-1; sc+=10; beep(800, 20); }
    }
    brkX += brkDX; brkY += brkDY;
    if(brkX<0 || brkX>SW) brkDX *= -1;
    if(brkY<0) brkDY *= -1;
    if(brkY>60 && brkX>pdX && brkX<pdX+20) { brkDY *= -1.1; beep(400, 10); }
    if(brkY>64) { saveHighScore(8, sc); st=GAME_OVER; playGameOver(); }
    if(nTr.getState()==LOW && pdX>0) pdX-=4; if(nP.getState()==LOW && pdX<108) pdX+=4;
    display.display();
    if(nOk.isPressed()) { st = PAUSE; pauseOpt = 0; }
  }

  else if (st == MAZE) {
    display.clearDisplay();
    for(int i=0; i<16; i++) for(int j=0; j<8; j++) if(maze[i][j]) display.fillRect(i*8, j*8, 8, 8, SH110X_WHITE);
    display.fillRect(mzX*8+2, mzY*8+2, 4, 4, SH110X_WHITE); // Player
    display.setCursor(0, 0); display.print(" HI:"); display.print(highScore);
    display.drawRect(14*8+2, 6*8+2, 4, 4, SH110X_WHITE); // Exit
    if(mzX == 14 && mzY == 6) { sc+=50; saveHighScore(9, sc); st=G_SEL; beep(2000, 100); }
    if(nL.isPressed() && mzY>0 && !maze[mzX][mzY-1]) mzY--;
    if(nX.isPressed() && mzY<7 && !maze[mzX][mzY+1]) mzY++;
    if(nTr.isPressed() && mzX>0 && !maze[mzX-1][mzY]) mzX--;
    if(nP.isPressed() && mzX<15 && !maze[mzX+1][mzY]) mzX++;
    display.display();
    if(nOk.isPressed()) { st = PAUSE; pauseOpt = 0; }
  }

  else if (st == PAUSE) {
    drawPauseMenu();
    if (nL.isPressed()) { pauseOpt = (pauseOpt - 1 + 3) % 3; beep(600, 20); }
    if (nX.isPressed()) { pauseOpt = (pauseOpt + 1) % 3; beep(600, 20); }
    if (nOk.isPressed()) {
      if (pauseOpt == 0) st = lastGame;
      else if (pauseOpt == 1) { st = SETTINGS; mIdx = 0; adjustTime = now; setIdx = 0; returnState = PAUSE; }
      else { 
        // Save score before exit
        int gIdx = -1;
        if (lastGame == TETRIS) gIdx = 0;
        else if (lastGame == CROSSY) gIdx = 1;
        else if (lastGame == PINGPONG) gIdx = 2;
        else if (lastGame == SNAKE) gIdx = 3;
        else if (lastGame == DINO) gIdx = 4;
        else if (lastGame == FLAPPY) gIdx = 5;
        else if (lastGame == TANK) gIdx = 6;
        else if (lastGame == DASH) gIdx = 7;
        else if (lastGame == BREAKOUT) gIdx = 8;
        else if (lastGame == MAZE) gIdx = 9;
        else if (lastGame == INVADERS) gIdx = 10;
        else if (lastGame == CATCH) gIdx = 11;
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
    
    const char* itemsVI[] = {"Thời gian", "Âm thanh", "Ngôn ngữ", "Thời gian tắt màn", "Thoát"};
    const char* itemsEN[] = {"TIME", "SOUND", "LANGUAGE", "SCREEN OFF TIME", "EXIT"};
    
    int top = (mIdx > 3) ? mIdx - 3 : 0; // Adjusted to show 4 items, scroll when mIdx > 3
    for(int i=0; i<5; i++) { // Iterate all 5 items
      int y = 18 + (i - top) * 12;
      if (y < 12 || y >= 64) continue; // Clipping logic
      if (language == 0) {
        myfont.print(10, y, (mIdx == i) ? "> " : "  ", SH110X_WHITE);
        myfont.print(22, y, (char*)itemsVI[i], SH110X_WHITE);
        
        // Cap nhat vi tri con tro GFX de hien thi gia tri (ON/OFF, VI/EN) dung cho
        if (i >= 1 && i <= 3) { // Adjusted for new item
             int w = myfont.getLength((char*)itemsVI[i]);
             display.setCursor(22 + w, y);
        }
      } else {
        display.setCursor(10, y);
        if(mIdx == i) display.print("> "); else display.print("  ");
        display.print(itemsEN[i]);
      }
      
      if (i == 1) { display.print(": "); display.print(soundOff ? "OFF" : "ON"); }
      if (i == 2) { display.print(": "); display.print(language==0 ? "VI" : "EN"); }
      if (i == 3) { display.print(": "); display.print(screenTimeoutValue / 1000); display.print("s"); }
    }
    display.display();
    
    if(nL.isPressed()) { mIdx=(mIdx-1+5)%5; beep(600, 20); }
    if(nX.isPressed()) { mIdx=(mIdx+1)%5; beep(600, 20); }
    if(nOk.isPressed()) {
      if(mIdx == 0) { st = SET_TIME; setIdx = 0; } // Go to Time Adjust
      else if(mIdx == 1) { soundOff = !soundOff; EEPROM.write(8, soundOff); EEPROM.commit(); }
      else if(mIdx == 2) { language = 1 - language; EEPROM.write(9, language); EEPROM.commit(); }
      else if(mIdx == 3) { st = SET_SCREEN_TIMEOUT; } // Go to Screen Timeout Adjust
      else if(mIdx == 4) { st = returnState; }
      beep(1000, 50);
    }
  }

  else if (st == SET_SCREEN_TIMEOUT) {
    display.clearDisplay();
    if (language == 0) myfont.print((SW - myfont.getLength("Thời gian tắt màn"))/2, 0, "Thời gian tắt màn", SH110X_WHITE);
    else { display.setCursor(10, 0); display.print("SCREEN OFF TIME"); }
    
    display.setCursor(40, 25);
    display.print(screenTimeoutValue / 1000);
    display.print("s");
    
    if (language == 0) myfont.print((SW - myfont.getLength("OK:Lưu Tr:Về"))/2, 52, "OK:Lưu Tr:Về", SH110X_WHITE);
    else { display.setCursor(19, 50); display.print("OK:Save Tr:Back"); }
    display.display();

    if (nTr.isPressed()) { st = SETTINGS; mIdx = 3; beep(1000, 50); } // Back to Settings
    if (nL.isPressed()) { // Increase by 5 seconds
      screenTimeoutValue += 5000;
      if (screenTimeoutValue > 300000) screenTimeoutValue = 300000; // Max 5 minutes
      beep(800, 20);
    }
    if (nX.isPressed()) { // Decrease by 5 seconds
      screenTimeoutValue -= 5000;
      if (screenTimeoutValue < 5000) screenTimeoutValue = 5000; // Min 5 seconds
      beep(800, 20);
    }
    if (nOk.isPressed()) { 
      EEPROM.write(130, (screenTimeoutValue >> 24) & 0xFF);
      EEPROM.write(131, (screenTimeoutValue >> 16) & 0xFF);
      EEPROM.write(132, (screenTimeoutValue >> 8) & 0xFF);
      EEPROM.write(133, screenTimeoutValue & 0xFF);
      EEPROM.commit();
      st = SETTINGS; mIdx = 3; beep(1500, 100); 
    }
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
    const char* tNamesVI[] = {"Bấm giờ", "Hẹn giờ", "Xúc xắc", "Thiền định", "Morse", "Nhạc vui", "Thoát"};
    const char* tNamesEN[] = {"STOPWATCH", "TIMER", "DICE", "ZEN MODE", "MORSE", "MUSIC", "EXIT"};
    int top = (mIdx > 3) ? mIdx - 3 : 0;
    for(int i=0; i<7; i++) {
      int y = 18 + (i - top) * 12;
      if (y < 12 || y >= 64) continue;
      if (language == 0) {
        myfont.print(10, y, (mIdx == i) ? "> " : "  ", SH110X_WHITE);
        myfont.print(22, y, (char*)tNamesVI[i], SH110X_WHITE);
      } else {
        display.setCursor(10, y);
        if(mIdx == i) display.print("> "); else display.print("  ");
        display.print(tNamesEN[i]);
      }
    }
    display.display();
    if(nL.isPressed()) { mIdx=(mIdx-1+7)%7; beep(600, 20); }
    if(nX.isPressed()) { mIdx=(mIdx+1)%7; beep(600, 20); }
    if(nOk.isPressed()) {
      if(mIdx == 0) { st = STOPWATCH; swRun = false; swElapsed = 0; }
      else if(mIdx == 1) { st = TIMER; if(!tmRun) { tmDuration = 0; tmRem = 0; mIdx = 0; tmHour=0; tmMin=0; tmSec=0; } }
      else if(mIdx == 2) { st = DICE; diceVal = 1; }
      else if(mIdx == 3) { st = ZEN_MODE; } //
      else if(mIdx == 4) { st = MORSE_MENU; mIdx = 0; }
      else if(mIdx == 5) { st = MUSIC_PLAYER; mIdx = 0; isMusicPlaying = false; currentSongIdx = -1; }
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
    int top = (mIdx > 3) ? mIdx - 3 : 0;
    for(int i=0; i<3; i++) {
      int y = 18 + (i - top) * 12;
      if (y < 12 || y >= 64) continue;
      if (language == 0) {
        myfont.print(10, y, (mIdx == i) ? "> " : "  ", SH110X_WHITE);
        myfont.print(22, y, (char*)gNamesVI[i], SH110X_WHITE);
      } else {
        display.setCursor(10, y);
        if(mIdx == i) display.print("> "); else display.print("  ");
        display.print(gNamesEN[i]);
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
    const char* tNamesVI[] = {"Bấm giờ", "Hẹn giờ", "Xúc xắc", "Thiền định", "Thoát"}; // 5 items
    const char* tNamesEN[] = {"STOPWATCH", "TIMER", "DICE", "ZEN MODE", "EXIT"}; // 5 items
    int top = (mIdx > 3) ? mIdx - 3 : 0; // Adjusted to show 4 items, scroll when mIdx > 3
    for(int i=0; i<5; i++) { // Iterate all 5 items
      int y = 18 + (i - top) * 12;
      if (y < 12 || y >= 64) continue;
      if (language == 0) {
        myfont.print(10, y, (mIdx == i) ? "> " : "  ", SH110X_WHITE);
        myfont.print(22, y, (char*)tNamesVI[i], SH110X_WHITE);
      } else {
        display.setCursor(10, y);
        if(mIdx == i) display.print("> "); else display.print("  ");
        display.print(tNamesEN[i]);
      }
    }
    display.display(); 
    if(nL.isPressed()) { mIdx=(mIdx-1+5)%5; beep(600, 20); }
    if(nX.isPressed()) { mIdx=(mIdx+1)%5; beep(600, 20); }
    if(nOk.isPressed()) {
      if(mIdx == 0) { st = STOPWATCH; swRun = false; swElapsed = 0; }
      else if(mIdx == 1) { st = TIMER; if(!tmRun) { tmDuration = 0; tmRem = 0; mIdx = 0; tmHour=0; tmMin=0; tmSec=0; } }
      else if(mIdx == 2) { st = DICE; diceVal = 1; }
      else if(mIdx == 3) { st = ZEN_MODE; } // Added ZEN_MODE
      else st = E_SEL; // Exit option
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
    int top = (mIdx > 3) ? mIdx - 3 : 0;
    for(int i=0; i<3; i++) {
      int y = 18 + (i - top) * 12;
      if (y < 12 || y >= 64) continue;
      if (language == 0) {
        myfont.print(10, y, (mIdx == i) ? "> " : "  ", SH110X_WHITE);
        myfont.print(22, y, (char*)gNamesVI[i], SH110X_WHITE);
      } else {
        display.setCursor(10, y);
        if(mIdx == i) display.print("> "); else display.print("  ");
        display.print(gNamesEN[i]);
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

  else if (st == ZEN_MODE) {
    display.clearDisplay();
    unsigned long zenMs = millis() % 8000; // 8 second cycle
    int radius;
    const char* actionVI;
    const char* actionEN;

    if (zenMs < 4000) { // Inhale 4s
      radius = map(zenMs, 0, 4000, 5, 25);
      actionVI = "Hít vào..."; actionEN = "Inhale...";
    } else { // Exhale 4s
      radius = map(zenMs, 4000, 8000, 25, 5);
      actionVI = "Thở ra..."; actionEN = "Exhale...";
    }

    display.drawCircle(SW / 2, SH / 2 + 5, radius, SH110X_WHITE);
    if (language == 0) myfont.print((SW - myfont.getLength((char*)actionVI)) / 2, 0, (char*)actionVI, SH110X_WHITE);
    else { display.setCursor((SW - (strlen(actionEN) * 6)) / 2, 0); display.print(actionEN); }
    
    display.display();
    if (nOk.isPressed() || nTr.isPressed()) { st = GADGETS; mIdx = 3; beep(1000, 50); }
  }

  else if (st == MUSIC_PLAYER) {
    display.clearDisplay();
    if (language == 0) myfont.print((SW - myfont.getLength("Nhạc Meme"))/2, 0, "Nhạc Meme", SH110X_WHITE);
    else { display.setCursor(34, 0); display.print("MEME MUSIC"); }

    for(int i=0; i<3; i++) {
      display.setCursor(10, 18 + i*12);
      if(mIdx == i) display.print("> "); else display.print("  ");
      display.print(playlist[i].name);
      if(currentSongIdx == i && isMusicPlaying) display.print(" *");
    }

    if (language == 0) myfont.print(10, 54, "OK:Phát Tr:Thoát", SH110X_WHITE);
    else { display.setCursor(10, 54); display.print("OK:Play Tr:Exit"); }
    
    display.display();

    if (nL.isPressed()) { mIdx = (mIdx - 1 + 3) % 3; beep(600, 20); }
    if (nX.isPressed()) { mIdx = (mIdx + 1) % 3; beep(600, 20); }
    
    if (nOk.isPressed()) {
      if (currentSongIdx == mIdx && isMusicPlaying) {
        isMusicPlaying = false;
        noTone(BZ);
      } else {
        currentSongIdx = mIdx;
        currentNoteIdx = 0;
        isMusicPlaying = true;
        nextNoteTime = millis();
      }
      beep(1000, 50);
    }

    if (nTr.isPressed()) { 
      isMusicPlaying = false; 
      noTone(BZ); 
      st = GADGETS; mIdx = 5; 
      beep(1000, 50); 
    }

    if (isMusicPlaying && millis() >= nextNoteTime) {
      uint16_t note = pgm_read_word(&playlist[currentSongIdx].notes[currentNoteIdx]);
      uint16_t duration = pgm_read_word(&playlist[currentSongIdx].durs[currentNoteIdx]);
      if (note != REST) beep(note, duration);
      else noTone(BZ);
      nextNoteTime = millis() + (duration * 1.3);
      currentNoteIdx++;
      if (currentNoteIdx >= playlist[currentSongIdx].length) currentNoteIdx = 0;
    }
  }

  else if (st == MORSE_MENU) {
    display.clearDisplay();
    const char* mItemsVI[] = {"CHỮ -> MORSE", "MORSE -> CHỮ", "QUAY LẠI"};
    const char* mItemsEN[] = {"TEXT TO MORSE", "MORSE TO TEXT", "BACK"};
    for(int i=0; i<3; i++) {
      int y = 15 + i*12;
      if (language == 0) {
        myfont.print(15, y, (mIdx == i) ? "> " : "  ", SH110X_WHITE);
        myfont.print(27, y, (char*)mItemsVI[i], SH110X_WHITE);
      } else {
        display.setCursor(15, y);
        if(mIdx == i) display.print("> "); else display.print("  ");
        display.print(mItemsEN[i]);
      }
    }
    display.display();
    if(nL.isPressed()) { mIdx=(mIdx-1+3)%3; beep(600, 20); }
    if(nX.isPressed()) { mIdx=(mIdx+1)%3; beep(600, 20); }
    if(nOk.isPressed()) {
      if(mIdx == 0) { st = TEXT_TO_MORSE; morseBuffer = ""; morsePicker = 'A'; }
      else if(mIdx == 1) { st = MORSE_TO_TEXT; morseBuffer = ""; morseInputBuffer = ""; }
      else { st = GADGETS; mIdx = 4; } // Trở về mục Morse trong menu Tools
      beep(1000, 50);
    }
  }

  else if (st == TEXT_TO_MORSE) {
    display.clearDisplay();
    if (language == 0) {
      myfont.print(0, 0, (char*)"KÝ TỰ:", SH110X_WHITE);
      display.setCursor(45, 0); display.print(morsePicker);
      myfont.print(0, 12, (char*)"VĂN BẢN:", SH110X_WHITE);
      display.setCursor(65, 12); display.print(morseBuffer);
      myfont.print(0, 35, (char*)"MORSE:", SH110X_WHITE);
    } else {
      display.setCursor(0, 0); display.print("CHAR: "); display.print(morsePicker);
      display.setCursor(0, 12); display.print("TEXT: "); display.print(morseBuffer);
      display.setCursor(0, 35); display.print("MORSE:");
    }
    display.setCursor(0, 45);
    String out = ""; for(int i=0; i<morseBuffer.length(); i++) out += getMorse(morseBuffer[i]) + " ";
    display.print(out);
    display.display();
    
    if(nL.isPressed()) { 
      int idx = 0; while(morseAlpha[idx] != morsePicker) idx++;
      morsePicker = morseAlpha[(idx + 1) % 27]; beep(600, 20);
    }
    if(nX.isPressed()) { 
      int idx = 0; while(morseAlpha[idx] != morsePicker) idx++;
      morsePicker = morseAlpha[(idx - 1 + 27) % 27]; beep(600, 20);
    }
    if(nOk.isPressed()) { morseBuffer += morsePicker; beep(1000, 20); }
    if(nTr.isPressed()) { st = MORSE_MENU; mIdx = 0; }
    if(nP.isPressed() && morseBuffer.length() > 0) { morseBuffer.remove(morseBuffer.length()-1); beep(400, 20); }
  }

  else if (st == MORSE_TO_TEXT) {
    display.clearDisplay();
    if (language == 0) {
      myfont.print(0, 0, (char*)"BẤM OK ĐỂ NHẬP", SH110X_WHITE);
      myfont.print(0, 15, (char*)"TÍN HIỆU: ", SH110X_WHITE); display.setCursor(65, 15); display.print(morseInputBuffer);
      myfont.print(0, 35, (char*)"VĂN BẢN: ", SH110X_WHITE); display.setCursor(65, 35); display.print(morseBuffer);
    } else {
      display.setCursor(0, 0); display.print("TAP OK FOR SIG");
      display.setCursor(0, 15); display.print("SIG: "); display.print(morseInputBuffer);
      display.setCursor(0, 35); display.print("TEXT: "); display.print(morseBuffer);
    }
    display.display();

    static unsigned long pressStart = 0;
    static bool pressed = false;

    if(nOk.getState() == LOW) {
      if(!pressed) { pressStart = millis(); pressed = true; }
      morseTimer = millis();
    } else {
      if(pressed) {
        unsigned long dur = millis() - pressStart;
        if(dur < 250) morseInputBuffer += ".";
        else morseInputBuffer += "-";
        pressed = false;
        morseTimer = millis();
        beep(800, 50);
      }
      
      if(morseInputBuffer.length() > 0 && millis() - morseTimer > 800) {
        morseBuffer += fromMorse(morseInputBuffer);
        morseInputBuffer = "";
        beep(1200, 50);
      }
      if(morseBuffer.length() > 0 && millis() - morseTimer > 2500 && morseBuffer[morseBuffer.length()-1] != ' ') {
        morseBuffer += " ";
      }
    }
    if(nTr.isPressed()) { st = MORSE_MENU; mIdx = 1; }
    if(nP.isPressed() && morseBuffer.length() > 0) { morseBuffer.remove(morseBuffer.length()-1); beep(400, 20); }
  }

  else if (st == DOOM) {
    display.clearDisplay();
    // Raycasting engine
    for(int x = 0; x < SW; x += 2) {
      float rayA = (pa_doom - 0.5) + ((float)x / (float)SW);
      float dX = cos(rayA), dY = sin(rayA);
      float dist = 0;
      bool hit = false;
      while(!hit && dist < 12) {
        dist += 0.15;
        int testX = (int)(px_doom + dX * dist);
        int testY = (int)(py_doom + dY * dist);
        if(testX < 0 || testX >= 12 || testY < 0 || testY >= 12) { hit = true; dist = 12; }
        else if(worldMap[testY][testX] == 1) hit = true;
      }
      int h = (dist > 0) ? (SH / dist) : SH;
      if (h > SH) h = SH;
      int y0 = (SH - h) / 2;
      display.drawFastVLine(x, y0, h, SH110X_WHITE);
      display.drawFastVLine(x+1, y0, h, SH110X_WHITE);
    }
    // Minimap
    for(int i=0; i<12; i++) for(int j=0; j<12; j++) if(worldMap[j][i]) display.drawPixel(110+i, 2+j, SH110X_WHITE);
    display.drawPixel(110+(int)px_doom, 2+(int)py_doom, SH110X_WHITE);

    if(nL.getState() == LOW) { // Forward
      float nx = px_doom + cos(pa_doom) * 0.12; float ny = py_doom + sin(pa_doom) * 0.12;
      if(worldMap[(int)ny][(int)nx] == 0) { px_doom = nx; py_doom = ny; }
    }
    if(nX.getState() == LOW) { // Backward
      float nx = px_doom - cos(pa_doom) * 0.12; float ny = py_doom - sin(pa_doom) * 0.12;
      if(worldMap[(int)ny][(int)nx] == 0) { px_doom = nx; py_doom = ny; }
    }
    if(nTr.getState() == LOW) pa_doom -= 0.1; if(nP.getState() == LOW) pa_doom += 0.1;
    display.display();
    if(nOk.isPressed()) { st = PAUSE; pauseOpt = 0; }
  }
}
