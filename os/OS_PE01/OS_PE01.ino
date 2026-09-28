/* =========================================================================
 *  SAVANT - OS (Output Stationary)   PE(0,1)  [상단 우측]
 * -------------------------------------------------------------------------
 *      LEFT  ←  a0 , b1
 *      C01 += a0 * b1
 *      DOWN  →  b1              (열 1 세로 흐름, PE(1,1) 용)
 *  종료 시 : DOWN → END_BYTE , C01(16bit)
 *
 *  핀
 *    D2 CLK_L / D4 DAT_L  ←  PE(0,0) D2 / D4
 *    D3 CLK_D / D5 DAT_D  →  PE(1,1) D3 / D5
 * ========================================================================= */
#include <Arduino.h>

#define CLK_L  2
#define DAT_L  4
#define CLK_D  3
#define DAT_D  5

#define END_BYTE  255
#define CLK_HALF  50

long C01 = 0;
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
  pinMode(CLK_L, INPUT_PULLUP); pinMode(DAT_L, INPUT_PULLUP);
  pinMode(CLK_D, OUTPUT);       pinMode(DAT_D, OUTPUT);
  digitalWrite(CLK_D, LOW);     digitalWrite(DAT_D, LOW);

  Serial.begin(115200);
  Serial.println(F("[PE01-OS] wait sender..."));
  waitSenderReady(CLK_L);
  Serial.println(F("[PE01-OS] sender ready"));
}

void loop() {
  if (finished) return;

  byte a0 = receiveByte(CLK_L, DAT_L);

  if (a0 == END_BYTE) {
    sendByte (CLK_D, DAT_D, END_BYTE);
    sendInt16(CLK_D, DAT_D, (int)C01);
    Serial.print(F("[PE01-OS] DONE  C01=")); Serial.println(C01);
    finished = true;
    return;
  }

  byte b1 = receiveByte(CLK_L, DAT_L);
  C01 += (long)a0 * b1;

  sendByte(CLK_D, DAT_D, b1);            // b1 을 아래로 (세로 흐름)

  Serial.print(F("[PE01-OS] a0=")); Serial.print(a0);
  Serial.print(F(" b1="));          Serial.print(b1);
  Serial.print(F(" C01="));         Serial.println(C01);
}
