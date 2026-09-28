/* =========================================================================
 *  SAVANT - OS (Output Stationary)   PE(1,1)  [최종 노드 / LCD]
 * -------------------------------------------------------------------------
 *      TOP   ←  b1        PE(0,1) 에서 (세로 흐름)
 *      LEFT  ←  a1        PE(1,0) 에서 (가로 흐름)
 *      C11 += a1 * b1
 *  종료 시 TOP 에서 C01, LEFT 에서 C10 을 함께 받아 LCD 에 표시
 *
 *  수신 순서가 [위쪽 → 왼쪽] 으로 고정되므로 폴링 불필요.
 *
 *  핀
 *    D2 CLK_L / D4 DAT_L  ←  PE(1,0) D3 / D5
 *    D3 CLK_T / D5 DAT_T  ←  PE(0,1) D3 / D5
 *    D6 TRIG              ←  PE(0,0) D6
 *    A4 SDA / A5 SCL      →  I2C LCD
 * ========================================================================= */
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define CLK_L  2
#define DAT_L  4
#define CLK_T  3
#define DAT_T  5
#define TRIG   6

#define LCD_ADDR 0x27              // 화면 안 나오면 0x3F 로 변경
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

#define END_BYTE  255

long C11 = 0;
int  C01 = 0, C10 = 0;
unsigned long t_start = 0;
unsigned long t_last  = 0;   // 마지막 MAC 완료 시각

// CLK 가 10ms 동안 연속 LOW 일 때만 통과 (노이즈로 인한 가짜 시작 차단)
void waitSenderReady(byte clk) {
  int stable = 0;
  while (stable < 200) {
    if (digitalRead(clk) == LOW) stable++;
    else stable = 0;
    delayMicroseconds(50);
  }
}

// 수신 중 CLK 가 2ms 이상 LOW 로 머물면 비트 카운터를 리셋한다.
// 송신측 리셋 시 핀이 Hi-Z 가 되어 풀업으로 HIGH 가 뜨면서
// 가짜 클럭 1 회가 세어지는 문제를 자동으로 제거한다.
byte receiveByte(byte clk, byte dat) {
  byte v = 0;
  int  i = 0;
  while (i < 8) {
    unsigned long lowStart = micros();
    while (digitalRead(clk) == LOW) {
      if (micros() - lowStart > 2000UL) {   // 프레임 재정렬
        i = 0; v = 0;
        lowStart = micros();
      }
    }
    v = (v << 1) | (digitalRead(dat) & 1);
    while (digitalRead(clk) == HIGH) { ; }
    i++;
  }
  return v;
}

int receiveInt16(byte clk, byte dat) {
  byte hi = receiveByte(clk, dat);
  byte lo = receiveByte(clk, dat);
  return (int)(((int)hi << 8) | lo);
}

void setup() {
  pinMode(CLK_L, INPUT_PULLUP); pinMode(DAT_L, INPUT_PULLUP);
  pinMode(CLK_T, INPUT_PULLUP); pinMode(DAT_T, INPUT_PULLUP);
  pinMode(TRIG,  INPUT);

  Serial.begin(115200);
  Wire.begin();
  lcd.init(); lcd.backlight();
  lcd.setCursor(0,0); lcd.print("OS PE(1,1)");
  lcd.setCursor(0,1); lcd.print("wait senders");

  waitSenderReady(CLK_T);
  waitSenderReady(CLK_L);
  lcd.setCursor(0,1); lcd.print("armed...     ");
  Serial.println(F("[PE11-OS] senders ready"));

  // 이전 실행이 진행 중이면(TRIG 가 이미 HIGH) 끝날 때까지 기다린 뒤
  // 새로운 실행의 상승 엣지를 잡는다.
  // 모니터 연결 등으로 이 보드만 중간에 리셋되어도 다음 회차부터 정상 동작한다.
  while (digitalRead(TRIG) == HIGH) { ; }   // 이전 회차 종료 대기
  while (digitalRead(TRIG) == LOW)  { ; }   // 새 회차 시작 대기
  t_start = micros();

  // ★ 여기서 LCD 를 갱신하면 I2C 통신이 수 ms 를 점유하여
  //    PE(0,1) 의 첫 전송(TRIG 후 약 1.6ms)을 놓친다.
  //    LCD 는 finish() 에서만 갱신한다.
  Serial.println(F("[PE11-OS] TRIG detected"));
}

void loop() {
  byte b1 = receiveByte(CLK_T, DAT_T);           // ① 위쪽

  // 데이터 원소는 항상 200 미만이므로 200 이상이면 종료 마커로 본다
  if (b1 >= 200) {
    C01 = receiveInt16(CLK_T, DAT_T);            // PE(0,1) 의 최종 C01
    byte mark = receiveByte(CLK_L, DAT_L);       // 왼쪽 종료 마커
    (void)mark;
    C10 = receiveInt16(CLK_L, DAT_L);            // PE(1,0) 의 최종 C10
    finish();
    return;
  }

  byte a1 = receiveByte(CLK_L, DAT_L);           // ② 왼쪽
  C11 += (long)a1 * b1;
  t_last = micros();                             // ★ 마지막 MAC 완료 시각 갱신

  Serial.print(F("[PE11-OS] a1=")); Serial.print(a1);
  Serial.print(F(" b1="));          Serial.print(b1);
  Serial.print(F(" C11="));         Serial.println(C11);
}

void finish() {
  // 종료 시퀀스(END 전파)는 제외하고, 첫 MAC 시작 ~ 마지막 MAC 완료 구간만 측정
  unsigned long elapsed = t_last - t_start;

  Serial.println(F("===== OS RESULT ====="));
  Serial.print(F("C01 = ")); Serial.println(C01);
  Serial.print(F("C10 = ")); Serial.println(C10);
  Serial.print(F("C11 = ")); Serial.println(C11);
  Serial.println(F("(C00 은 PE(0,0) 시리얼에서 확인)"));
  Serial.print(F("Elapsed (us) = ")); Serial.println(elapsed);
  Serial.print(F("Clocks(16MHz)= ")); Serial.println(elapsed * 16UL);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(C01); lcd.print(" ");
  lcd.print(C10); lcd.print(" ");
  lcd.print(C11);
  lcd.setCursor(0,1);
  lcd.print("T:"); lcd.print(elapsed); lcd.print("us");

  while (true) { ; }
}
