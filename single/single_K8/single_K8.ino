/* =========================================================================
 *  SAVANT - 단일 보드 (대조군)   Von Neumann 방식 모사   ***  K_SIZE = 8  ***
 * -------------------------------------------------------------------------
 *  전역 BUS 변수를 통해서만 행렬 원소에 접근한다.
 *  모든 데이터가 하나의 통로(BUS)를 순차적으로 통과하는 구조로
 *  폰 노이만 구조의 접근 패턴을 소프트웨어 수준에서 재현.
 *
 *  결과는 시리얼 모니터(9600)로 출력.  LCD 없음.
 *  행렬 크기 변경 : K_SIZE 와 A, B 배열만 수정
 * ========================================================================= */
#include <Arduino.h>

#define K_SIZE 8                    // 내부 차원 N (2 / 4 / 8)

// C(2x2) = A(2xK) · B(Kx2)
const int A[2][K_SIZE] = { {1,2,3,4,5,6,7,8}, {9,10,11,12,13,14,15,16} };
const int B[K_SIZE][2] = { {1,2}, {3,4}, {5,6}, {7,8}, {9,10}, {11,12}, {13,14}, {15,16} };
// 정답 : C = {{372,408},{884,984}}
//        BUS 접근 = 2 x 2 x 8 x 2 = 64

volatile int BUS = 0;               // 공유 버스 (한 번에 하나만 통과)
long C[2][2];
unsigned long accessCount = 0;

// 모든 메모리 접근은 반드시 이 함수를 거친다
inline int busRead(int value) {
  BUS = value;
  accessCount++;
  return BUS;
}

void setup() {
  Serial.begin(115200);
  delay(300);

  for (int i = 0; i < 2; i++)
    for (int j = 0; j < 2; j++)
      C[i][j] = 0;

  unsigned long t0 = micros();

  for (int i = 0; i < 2; i++)
    for (int j = 0; j < 2; j++)
      for (int k = 0; k < K_SIZE; k++) {
        int a = busRead(A[i][k]);
        int b = busRead(B[k][j]);
        C[i][j] += (long)a * b;
      }

  unsigned long elapsed = micros() - t0;

  Serial.println(F("===== SINGLE BOARD (Von Neumann) ====="));
  Serial.print(F("K_SIZE       = ")); Serial.println(K_SIZE);
  Serial.print(F("C[0][0..1]   = ")); Serial.print(C[0][0]);
  Serial.print(F(", "));              Serial.println(C[0][1]);
  Serial.print(F("C[1][0..1]   = ")); Serial.print(C[1][0]);
  Serial.print(F(", "));              Serial.println(C[1][1]);
  Serial.print(F("BUS access   = ")); Serial.println(accessCount);
  Serial.print(F("Elapsed (us) = ")); Serial.println(elapsed);
  Serial.print(F("Clocks(16MHz)= ")); Serial.println(elapsed * 16UL);
}

void loop() {}
