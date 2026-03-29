"""
바이너리 패킷 시리얼 통신 모듈
프로토콜: [SOF(0xAA)] [LEN] [CMD] [PAYLOAD...] [CRC8]
"""
import serial
import serial.tools.list_ports
import threading
import struct

PROTO_SOF = 0xAA
PROTO_MAX_PAYLOAD = 32

# PC -> STM32 Commands
CMD_ROBOT_MOVE     = 0x40
CMD_ROBOT_SET_PID  = 0x41
CMD_ROBOT_SET_MODE = 0x42

# STM32 -> PC Responses
CMD_ACK             = 0x82
CMD_ERROR           = 0x83
CMD_ROBOT_TELEMETRY = 0x88


def crc8(data):
    checksum = 0
    for b in data:
        checksum ^= b
    return checksum


class TelemetryData:
    """텔레메트리 패킷(0x88) 파싱 결과"""
    def __init__(self):
        self.angle = 0.0
        self.gyro_rate = 0.0
        self.pid_output = 0.0
        self.left_cmd = 0
        self.right_cmd = 0

    def parse(self, payload):
        if len(payload) < 10:
            return False
        self.angle      = struct.unpack('>h', bytes(payload[0:2]))[0] / 10.0
        self.gyro_rate  = struct.unpack('>h', bytes(payload[2:4]))[0] / 10.0
        self.pid_output = struct.unpack('>h', bytes(payload[4:6]))[0] / 10.0
        self.left_cmd   = struct.unpack('>h', bytes(payload[6:8]))[0]
        self.right_cmd  = struct.unpack('>h', bytes(payload[8:10]))[0]
        return True


class SerialCom:
    """바이너리 패킷 시리얼 송수신"""

    def __init__(self):
        self._ser = None
        self._running = False
        self._thread = None
        self._telemetry = TelemetryData()
        self._on_telemetry = None  # 콜백
        self._on_ack = None
        # 파서 상태
        self._parse_state = 0  # 0=SOF, 1=LEN, 2=CMD, 3=PAYLOAD, 4=CRC
        self._parse_len = 0
        self._parse_cmd = 0
        self._parse_payload = []
        self._parse_idx = 0

    @property
    def telemetry(self):
        return self._telemetry

    @property
    def is_connected(self):
        return self._ser is not None and self._ser.is_open

    def connect(self, port, baudrate=9600):
        try:
            self._ser = serial.Serial(port, baudrate, timeout=0.1)
            self._running = True
            self._thread = threading.Thread(target=self._read_loop, daemon=True)
            self._thread.start()
            return True
        except serial.SerialException as e:
            print(f"[ERROR] 연결 실패: {e}")
            return False

    def disconnect(self):
        self._running = False
        if self._ser and self._ser.is_open:
            self._ser.close()

    def set_on_telemetry(self, callback):
        self._on_telemetry = callback

    def set_on_ack(self, callback):
        self._on_ack = callback

    # ── 송신 ──

    def _build_packet(self, cmd, payload):
        buf = [PROTO_SOF, len(payload), cmd] + list(payload)
        crc_data = [len(payload), cmd] + list(payload)
        buf.append(crc8(crc_data))
        return bytearray(buf)

    def _send(self, pkt):
        if not self.is_connected:
            return
        try:
            self._ser.write(pkt)
        except Exception:
            pass

    def send_move(self, direction, speed):
        self._send(self._build_packet(CMD_ROBOT_MOVE, [direction, speed]))

    def send_set_pid(self, pid_type, value):
        """pid_type: 'P', 'I', 'D', value: float"""
        raw = int(value * 100)
        hi = (raw >> 8) & 0xFF
        lo = raw & 0xFF
        self._send(self._build_packet(CMD_ROBOT_SET_PID, [ord(pid_type), hi, lo]))

    def send_set_mode(self, mode):
        self._send(self._build_packet(CMD_ROBOT_SET_MODE, [mode]))

    # ── 수신 ──

    def _parse_reset(self):
        self._parse_state = 0
        self._parse_len = 0
        self._parse_cmd = 0
        self._parse_payload = []
        self._parse_idx = 0

    def _parse_byte(self, byte):
        if self._parse_state == 0:  # SOF
            if byte == PROTO_SOF:
                self._parse_state = 1
        elif self._parse_state == 1:  # LEN
            if byte > PROTO_MAX_PAYLOAD:
                self._parse_reset()
            else:
                self._parse_len = byte
                self._parse_state = 2
        elif self._parse_state == 2:  # CMD
            self._parse_cmd = byte
            self._parse_idx = 0
            self._parse_payload = []
            self._parse_state = 4 if self._parse_len == 0 else 3
        elif self._parse_state == 3:  # PAYLOAD
            self._parse_payload.append(byte)
            self._parse_idx += 1
            if self._parse_idx >= self._parse_len:
                self._parse_state = 4
        elif self._parse_state == 4:  # CRC
            crc_data = [self._parse_len, self._parse_cmd] + self._parse_payload
            expected = crc8(crc_data)
            if byte == expected:
                self._handle_packet(self._parse_cmd, self._parse_payload)
            self._parse_reset()

    def _handle_packet(self, cmd, payload):
        if cmd == CMD_ROBOT_TELEMETRY:
            if self._telemetry.parse(payload):
                if self._on_telemetry:
                    self._on_telemetry(self._telemetry)
        elif cmd == CMD_ACK:
            if self._on_ack:
                self._on_ack(payload)

    def _read_loop(self):
        while self._running:
            try:
                if self._ser and self._ser.in_waiting > 0:
                    data = self._ser.read(self._ser.in_waiting)
                    for b in data:
                        self._parse_byte(b)
            except Exception:
                continue
