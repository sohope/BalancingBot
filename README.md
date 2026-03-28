# BalancingBot - STM32F411RE 밸런싱 로봇

## 개발 환경
- MCU: STM32F411RETx (LQFP64)
- IDE: STM32CubeIDE
- FW Package: STM32Cube FW_F4 V1.28.3

## 주변장치 설정

### 1. RCC (클럭)
- HSE 8MHz 외부 오실레이터
- PLL 설정을 통해 SYSCLK 100MHz, Timer 100MHz 동작

### 2. USART2 (블루투스)
- 블루투스 모듈 통신용
- 핀: PA2 (TX), PA3 (RX)
- Baud Rate: 9600
- DMA 사용 (RX) - DMA1_Stream5

### 3. I2C1 (자이로 센서)
- Gyro 센서 데이터 수신용
- 핀: PB8 (SCL), PB9 (SDA)

### 4. TIM3 (모터 제어 PWM)
- CH1 (PA6): Motor1
- CH2 (PA7): Motor2
- Prescaler: 99 → Timer 클럭: 100MHz / 100 = 1MHz
- Period: 999 → PWM 주파수: 1MHz / 1000 = 1kHz
- AutoReload Preload 활성화

### 5. NVIC (인터럽트)
- DMA1_Stream5 (USART2 RX)
- USART2
- SysTick
