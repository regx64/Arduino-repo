# SAVANT — 폰 노이만 vs 시스톨릭 어레이(OS) 처리 시간 비교 실험

아두이노 나노로 단일 보드(폰 노이만 방식 모사)와 2×2 Output Stationary(OS)
시스톨릭 어레이의 처리 시간을 측정·비교하는 실험 코드입니다.

`C(2×2) = A(2×K) · B(K×2)`, 내부 차원 K = 2 / 4 / 6 / 8

## 📁 파일 구성

Arduino IDE 규칙에 따라 각 스케치는 같은 이름의 폴더 안에 있습니다.

| 폴더 | 파일 | 설명 |
|------|------|------|
| `single/` | `single_K2.ino` ~ `single_K8.ino` | 단일 보드(대조군), 결과는 시리얼 모니터(9600) |
| `os/` | `OS_PE00_K2.ino` ~ `OS_PE00_K8.ino` | OS PE(0,0) 마스터 / 입력 주입 (K별) |
| `os/` | `OS_PE01.ino` | OS PE(0,1) 상단 우측, `C01` 누산 후 b1을 아래로 전달 (K 공통) |
| `os/` | `OS_PE10.ino` | OS PE(1,0) 하단 좌측, `C10` 누산 후 a1을 오른쪽으로 전달 (K 공통) |
| `os/` | `OS_PE11.ino` | OS PE(1,1) 최종 노드, I2C LCD에 결과 표시 (K 공통) |

## 🔌 핀 요약 (OS)

- PE(0,0): D2 CLK_R / D4 DAT_R → PE(0,1), D3 CLK_D / D5 DAT_D → PE(1,0), D6 TRIG → PE(1,1) D6
- PE(0,1): D2 CLK_L / D4 DAT_L ← PE(0,0), D3 CLK_D / D5 DAT_D → PE(1,1)
- PE(1,0): D3 CLK_U / D5 DAT_U ← PE(0,0), D2 CLK_R / D4 DAT_R → PE(1,1)
- PE(1,1): D2 CLK_L / D4 DAT_L ← PE(1,0), D3 CLK_T / D5 DAT_T ← PE(0,1), A4 SDA / A5 SCL → I2C LCD
- 모든 보드 GND 공통 접지 필수
- LCD 주소 기본 `0x27` (안 나오면 `OS_PE11.ino`의 `LCD_ADDR`를 `0x3F`로 변경)
- 필요 라이브러리: `LiquidCrystal_I2C`

## 📊 정답표

| K | C | 단일 보드 BUS 접근 |
|---|---|---|
| 2 | `{{11,14},{35,46}}` | 16 |
| 4 | `{{50,60},{114,140}}` | 32 |
| 6 | `{{161,182},{377,434}}` | 48 |
| 8 | `{{372,408},{884,984}}` | 64 |
