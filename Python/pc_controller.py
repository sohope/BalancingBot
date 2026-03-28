import serial
import struct
import threading
import time
import tkinter as tk  # <--- 아까 에러가 났던 이유는 이 줄이 빠져서 그렇습니다!

# ==========================================
# 1. 블루투스(시리얼) 설정 (본인의 COM 포트에 맞게 수정하세요!)
# ==========================================
PORT = 'COM26'       # 블루투스 모듈이 연결된 PC의 COM 포트 번호 (맞게 수정해서 쓰세요)
BAUDRATE = 115200    # STM32 설정과 동일하게 맞춤

try:
    ser = serial.Serial(PORT, BAUDRATE, timeout=0.1)
    print(f"[{PORT}] 블루투스 연결 성공!")
except Exception as e:
    print(f"연결 실패: {e}")
    exit()

# ==========================================
# 2. 프로토콜 패킷 생성 함수
# ==========================================
def crc8(data_list):
    """ STM32 코드와 동일한 XOR 방식의 CRC8 계산 """
    checksum = 0
    for byte in data_list:
        checksum ^= byte
    return checksum

def send_move_cmd(direction, speed):
    """ 방향과 속도를 패킷으로 만들어 전송 """
    SOF = 0xAA
    EOF = 0x55
    CMD_ROBOT_MOVE = 0x40
    
    # Payload
    payload = [direction, speed]
    length = len(payload)
    
    # CRC 계산용 데이터 (LEN + CMD + PAYLOAD)
    crc_data = [length, CMD_ROBOT_MOVE] + payload
    crc_value = crc8(crc_data)
    
    # 최종 패킷 조립
    packet = [SOF] + crc_data + [crc_value, EOF]
    
    # 바이트 배열로 변환하여 전송
    ser.write(bytearray(packet))
    print(f"[TX] 방향:{direction}, 속도:{speed} | 패킷: {[hex(x) for x in packet]}")

# ==========================================
# 3. STM32 응답 수신 스레드
# ==========================================
def read_serial():
    """ STM32에서 보내는 ACK나 데이터를 백그라운드에서 읽음 """
    while True:
        if ser.in_waiting > 0:
            rx_data = ser.read(ser.in_waiting)
            print(f"  --> [RX 응답 수신]: {[hex(x) for x in rx_data]}")
        time.sleep(0.01)

# 수신 스레드 시작
rx_thread = threading.Thread(target=read_serial, daemon=True)
rx_thread.start()

# ==========================================
# 4. GUI 및 키보드 이벤트 처리 (WASD 조종 및 속도 조절)
# ==========================================
SPEED = 50 # 초기 속도

def update_label():
    """ 화면에 현재 속도를 업데이트 해주는 함수 """
    label.config(text=f"현재 속도: {SPEED} %\n\n[W, A, S, D] 방향 조종\n[+, -] 속도 조절")

def on_key_press(event):
    global SPEED # 전역 변수 SPEED를 수정하기 위해 선언
    key = event.keysym.lower()
    char = event.char # 특수기호(+,-) 입력을 받기 위함
    
    # 1. 방향 조종
    if key == 'w': send_move_cmd(1, SPEED)
    elif key == 's': send_move_cmd(2, SPEED)
    elif key == 'a': send_move_cmd(3, SPEED)
    elif key == 'd': send_move_cmd(4, SPEED)
    
    # 2. 속도 조절 (+ 또는 = 키 누르면 증가, - 키 누르면 감소)
    elif char == '+' or char == '=':
        SPEED += 10
        if SPEED > 100: SPEED = 100 # 최대 속도 제한
        print(f"[시스템] 속도 증가! 현재 속도: {SPEED}")
        update_label()
        
    elif char == '-':
        SPEED -= 10
        if SPEED < 0: SPEED = 0 # 최소 속도 제한
        print(f"[시스템] 속도 감소! 현재 속도: {SPEED}")
        update_label()

def on_key_release(event):
    # 키를 뗄 때 무조건 정지하지만, +, - 키를 뗄 때는 로봇이 멈추지 않도록 예외 처리
    char = event.char
    if char not in ['+', '=', '-']:
        send_move_cmd(0, 0)

# Tkinter 윈도우 생성
root = tk.Tk()
root.title("밸런싱 로봇 조종기 테스트")
root.geometry("350x250")

# 화면 안내 문구 생성
label = tk.Label(root, text=f"현재 속도: {SPEED} %\n\n[W, A, S, D] 방향 조종\n[+, -] 속도 조절", font=("Arial", 14))
label.pack(expand=True)

# 키보드 이벤트 바인딩
root.bind('<KeyPress>', on_key_press)
root.bind('<KeyRelease>', on_key_release)

# 창 닫힐 때 시리얼 포트 닫기
def on_closing():
    ser.close()
    root.destroy()

root.protocol("WM_DELETE_WINDOW", on_closing)

# GUI 실행
print("WASD 키와 +, - 키를 눌러 조종 테스트를 시작합니다...")
root.mainloop()