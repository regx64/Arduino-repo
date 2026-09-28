/* =========================================================================
 *  SAVANT - OS (Output Stationary)   PE(1,0)  [하단 좌측]
 * -------------------------------------------------------------------------
 *      UP    ←  b0 , a1
 *      C10 += a1 * b0
 *      RIGHT →  a1              (행 1 가로 흐름, PE(1,1) 용)
 *  종료 시 : RIGHT → END_BYTE , C10(16bit)
 *
 *  핀
 *    D3 CLK_U / D5 DAT_U  ←  PE(0,0) D3 / D5
 *    D2 CLK_R / D4 DAT_R  →  PE(1,1) D2 / D4
 * ========================================================================= */
#include <Arduino.h>

#define CLK_U  3
#define DAT_U  5
#define CLK_R  2
#define DAT_R  4

#define END_BYTE  255
#define CLK_HALF  50

long C10 = 0;
bool finished = false;

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

void sendByte(byte clk, byte dat, byte v) {
  for (int i = 7; i >= 0; i--) {
    digitalWrite(clk, LOW);
    digitalWrite(dat, (v >> i) & 1);
    delayMicroseconds(CLK_HALF);
    digitalWrite(clk, HIGH);
    delayMicroseconds(CLK_HALF);
  }
  digitalWrite(clk, LOW);
}

void sendInt16(byte clk, byte dat, int v) {
  sendByte(clk, dat, (byte)((v >> 8) & 0xFF));
  sendByte(clk, dat, (byte)( v       & 0xFF));
}

void setup() {
  pinMode(CLK_U, INPUT_PULLUP); pinMode(DAT_U, INPUT_PULLUP);
  pinMode(CLK_R, OUTPUT);       pinMode(DAT_R, OUTPUT);
  digitalWrite(CLK_R, LOW);     digitalWrite(DAT_R, LOW);

  Serial.begin(115200);
  Serial.println(F("[PE10-OS] wait sender..."));
  waitSenderReady(CLK_U);
  Serial.println(F("[PE10-OS] sender ready"));
}

void loop() {
  if (finished) return;

  byte b0 = receiveByte(CLK_U, DAT_U);

  if (b0 == END_BYTE) {
    sendByte (CLK_R, DAT_R, END_BYTE);
    sendInt16(CLK_R, DAT_R, (int)C10);
    Serial.print(F("[PE10-OS] DONE  C10=")); Serial.println(C10);
    finished = true;
    return;
  }

  byte a1 = receiveByte(CLK_U, DAT_U);
  C10 += (long)a1 * b0;

  sendByte(CLK_R, DAT_R, a1);            // a1 을 오른쪽으로 (가로 흐름)

  Serial.print(F("[PE10-OS] b0=")); Serial.print(b0);
  Serial.print(F(" a1="));          Serial.print(a1);
  Serial.print(F(" C10="));         Serial.println(C10);
}
