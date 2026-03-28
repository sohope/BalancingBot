import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation

# 1. 블루투스 포트 설정 (본인의 환경에 맞게 COM포트 확인)
SERIAL_PORT = 'COM26' 
BAUD_RATE = 9600

# 시리얼 포트 열기
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"{SERIAL_PORT} 연결 성공!")
except Exception as e:
    print(f"포트 열기 실패: {e}")
    exit()

# 2. 그래프 설정
fig, ax = plt.subplots()
y_data = [] # 데이터를 담을 리스트

def update(frame):
    try:
        # 3. STM32에서 쏜 데이터 한 줄(\n 기준) 읽어오기
        line = ser.readline().decode('utf-8').strip()
        
        if line.isdigit(): # 읽어온 데이터가 숫자인지 확인
            val = int(line)
            y_data.append(val)
            
            # 그래프 창에 데이터가 너무 많아지면 밀어내기 (최근 50개만 표시)
            if len(y_data) > 50:
                y_data.pop(0)

            # 4. 화면 지우고 새로 그리기
            ax.clear()
            ax.plot(y_data, marker='o', color='blue')
            ax.set_title("STM32 Bluetooth Fake Data")
            ax.set_ylim(-10, 110) # 0~99 데이터이므로 y축 범위 고정
            
    except Exception as e:
        pass # 쓰레기 값이 들어오면 무시

# 5. 50ms 간격으로 update 함수를 계속 실행하며 애니메이션 만들기
ani = animation.FuncAnimation(fig, update, interval=50)

plt.show() # 창 띄우기

# 창을 닫으면 포트 정리
ser.close()