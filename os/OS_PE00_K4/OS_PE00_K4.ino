/* =========================================================================
 *  SAVANT - OS (Output Stationary)   PE(0,0)  [마스터 / 입력 주입]   ***  K_SIZE = 4  ***
 * -------------------------------------------------------------------------
 *  C = A · B   각 PE 가 자신의 출력 원소를 끝까지 누산한다.
 *      A(2x4) = {{1,2,3,4}, {5,6,7,8}}
 *      B(4x2) = {{1,2}, {3,4}, {5,6}, {7,8}}
 *      정답 C = {{50,60},{114,140}}
 *      LCD 표시 : 60 114 140   /   PE00 C00 = 50
 *
 *  PE(0,0) 은 C00 을 누산하며, 나머지 PE 에 필요한 값을 주입한다.
 *      C00 += A[0][k] * B[k][0]
 *      RIGHT →  A[0][k] , B[k][1]        (PE(0,1) 용)
 *      (SKEW)
 *      DOWN  →  B[k][0] , A[1][k]        (PE(1,0) 용)
 *
 *  핀
 *    D2 CLK_R / D4 DAT_R  →  PE(0,1) D2 / D4
 *    D3 CLK_D / D5 DAT_D  →  PE(1,0) D2 / D4
 *    D6 TRIG              →  PE(1,1) D6
 * ========================================================================= */
#include <Arduino.h>

#define CLK_R  2
#define DAT_R  4
#define CLK_D  3
#define DAT_D  5
#define TRIG   6

#define K_SIZE    4
#define END_BYTE  255
#define CLK_HALF  50
#define SKEW_US   4000

const byte A[2][K_SIZE] = { {1,2,3,4}, {5,6,7,8} };
const byte B[K_SIZE][2] = { {1,2}, {3,4}, {5,6}, {7,8} };

long C00 = 0;

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

void setup() {
  pinMode(CLK_R, OUTPUT); pinMode(DAT_R, OUTPUT);
  pinMode(CLK_D, OUTPUT); pinMode(DAT_D, OUTPUT);
  pinMode(TRIG,  OUTPUT);
  digitalWrite(CLK_R, LOW); digitalWrite(DAT_R, LOW);
  digitalWrite(CLK_D, LOW); digitalWrite(DAT_D, LOW);
  digitalWrite(TRIG,  LOW);

  Serial.begin(115200);
  Serial.println(F("[PE00-OS] init, wait 3s"));
  delay(3000);

  digitalWrite(TRIG, HIGH);
  Serial.println(F("[PE00-OS] TRIG HIGH, start"));

  for (int k = 0; k < K_SIZE; k++) {
    C00 += (long)A[0][k] * B[k][0];        // 자신의 누산

    sendByte(CLK_R, DAT_R, A[0][k]);       // ① 오른쪽 : a0
    sendByte(CLK_R, DAT_R, B[k][1]);       //           b1
    delayMicroseconds(SKEW_US);
    sendByte(CLK_D, DAT_D, B[k][0]);       // ② 아래   : b0
    sendByte(CLK_D, DAT_D, A[1][k]);       //           a1

    Serial.print(F("[PE00-OS] k=")); Serial.print(k);
    Serial.print(F(" C00="));        Serial.println(C00);
  }

  // ★ 종료 시퀀스 전에 여유를 둔다.
  //   마지막 데이터 직후 곧바로 END 를 쏘면 PE(1,1) 이 아직 직전 값을
  //   처리하는 중이라 END 의 첫 비트를 놓쳐 254 로 오독한다.
  //   이 구간은 측정 시간에 포함되지 않는다 (PE11 이 마지막 MAC 시점까지만 잼).
  delay(20);

  sendByte(CLK_R, DAT_R, END_BYTE);
  delayMicroseconds(SKEW_US);
  sendByte(CLK_D, DAT_D, END_BYTE);

  digitalWrite(TRIG, LOW);
  Serial.print(F("[PE00-OS] DONE  C00=")); Serial.println(C00);
}

void loop() {}
