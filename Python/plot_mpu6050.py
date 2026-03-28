import serial
import serial.tools.list_ports
import threading
import logging
from collections import deque
from datetime import datetime

from dash import Dash, html, dcc
from dash.dependencies import Input, Output
import plotly.graph_objs as go

# ── 설정 ──────────────────────────────────────────────
SERIAL_PORT = 'COM21'  # 자동 탐색 실패 시 직접 지정
BAUD_RATE = 9600
MAX_POINTS = 200    # 그래프에 표시할 최대 데이터 수
UPDATE_MS = 200     # 그래프 갱신 주기 (ms)

# ── 데이터 저장소 ─────────────────────────────────────
timestamps = deque(maxlen=MAX_POINTS)
data = {
    'AX': deque(maxlen=MAX_POINTS),
    'AY': deque(maxlen=MAX_POINTS),
    'AZ': deque(maxlen=MAX_POINTS),
    'GX': deque(maxlen=MAX_POINTS),
    'GY': deque(maxlen=MAX_POINTS),
    'GZ': deque(maxlen=MAX_POINTS),
}

latest = {'AX': 0, 'AY': 0, 'AZ': 0, 'GX': 0, 'GY': 0, 'GZ': 0}

# ── 시리얼 포트 자동 탐색 ─────────────────────────────
def find_serial_port():
    ports = serial.tools.list_ports.comports()
    for p in ports:
        if 'USB' in p.description.upper() or 'SERIAL' in p.description.upper() or 'ST' in p.description.upper():
            return p.device
    if ports:
        return ports[0].device
    return None

# ── 파싱 ──────────────────────────────────────────────
def parse_line(line):
    """AX:0.01 AY:-0.02 AZ:1.00 GX:0.3 GY:-0.1 GZ:0.0"""
    try:
        parts = line.strip().split()
        values = {}
        for part in parts:
            key, val = part.split(':')
            values[key] = float(val)
        return values
    except (ValueError, IndexError):
        return None

# ── 시리얼 수신 스레드 ────────────────────────────────
def serial_reader():
    port = SERIAL_PORT or find_serial_port()
    if port is None:
        print("[ERROR] 시리얼 포트를 찾을 수 없습니다.")
        print("  사용 가능한 포트:")
        for p in serial.tools.list_ports.comports():
            print(f"    {p.device} - {p.description}")
        print("  SERIAL_PORT 변수에 직접 포트를 지정해주세요.")
        return

    print(f"[INFO] {port} 연결 중... (Baud: {BAUD_RATE})")
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
        print(f"[INFO] {port} 연결 완료")
    except serial.SerialException as e:
        print(f"[ERROR] 시리얼 연결 실패: {e}")
        return

    while True:
        try:
            raw = ser.readline().decode('utf-8', errors='ignore')
            if not raw.strip():
                continue

            print(f"[RX] {raw.strip()}")

            values = parse_line(raw)
            if values is None:
                print(f"[WARN] 파싱 실패: {raw.strip()}")
                continue

            now = datetime.now()
            timestamps.append(now)
            for key in data:
                val = values.get(key, 0)
                data[key].append(val)
                latest[key] = val
        except Exception as e:
            print(f"[ERROR] 수신 오류: {e}")
            continue

# ── 그래프 스타일 (Grafana 느낌) ──────────────────────
COLORS = {
    'bg':       '#0b0e11',
    'panel':    '#181b1f',
    'grid':     '#2a2e35',
    'text':     '#d1d5db',
    'AX':       '#ff6b6b',
    'AY':       '#51cf66',
    'AZ':       '#339af0',
    'GX':       '#ffd43b',
    'GY':       '#cc5de8',
    'GZ':       '#20c997',
}

CHECKBOX_STYLE = {
    'color': '#d1d5db',
    'display': 'inline-block',
    'marginRight': '16px',
    'fontSize': '13px',
    'cursor': 'pointer',
}

def make_graph_layout(title, yaxis_title):
    return go.Layout(
        title=dict(text=title, font=dict(color=COLORS['text'], size=16), x=0.02),
        paper_bgcolor=COLORS['panel'],
        plot_bgcolor=COLORS['panel'],
        font=dict(color=COLORS['text']),
        margin=dict(l=60, r=20, t=50, b=40),
        legend=dict(orientation='h', y=1.12, x=0.5, xanchor='center',
                    font=dict(size=12)),
        xaxis=dict(
            gridcolor=COLORS['grid'], showgrid=True,
            zeroline=False, tickformat='%H:%M:%S',
        ),
        yaxis=dict(
            title=yaxis_title,
            gridcolor=COLORS['grid'], showgrid=True,
            zeroline=True, zerolinecolor=COLORS['grid'],
        ),
        uirevision='constant',
    )

def make_indicator(label, key, unit):
    return html.Div(style={
        'backgroundColor': COLORS['panel'],
        'borderRadius': '8px',
        'padding': '12px 20px',
        'textAlign': 'center',
        'minWidth': '100px',
    }, children=[
        html.Div(label, style={'color': COLORS[key], 'fontSize': '13px', 'fontWeight': 'bold'}),
        html.Div(id=f'ind-{key}', style={'color': COLORS['text'], 'fontSize': '22px', 'fontFamily': 'monospace'}),
        html.Div(unit, style={'color': '#6b7280', 'fontSize': '11px'}),
    ])

# ── Dash 앱 ──────────────────────────────────────────
logging.getLogger('werkzeug').setLevel(logging.ERROR)

app = Dash(__name__, update_title=None)
app.title = 'MPU6050 Dashboard'

app.layout = html.Div(style={
    'backgroundColor': COLORS['bg'],
    'minHeight': '100vh',
    'padding': '20px',
    'fontFamily': 'Segoe UI, sans-serif',
}, children=[
    # 헤더
    html.Div(style={'display': 'flex', 'alignItems': 'center', 'marginBottom': '20px'}, children=[
        html.H1('MPU6050 Dashboard', style={
            'color': COLORS['text'], 'margin': '0', 'fontSize': '24px', 'fontWeight': '600',
        }),
        html.Div('● LIVE', style={
            'color': '#51cf66', 'marginLeft': '16px', 'fontSize': '13px', 'fontWeight': 'bold',
        }),
    ]),

    # 현재 값 표시
    html.Div(style={
        'display': 'flex', 'gap': '12px', 'marginBottom': '20px', 'flexWrap': 'wrap',
    }, children=[
        make_indicator('Accel X', 'AX', 'g'),
        make_indicator('Accel Y', 'AY', 'g'),
        make_indicator('Accel Z', 'AZ', 'g'),
        make_indicator('Gyro X', 'GX', 'deg/s'),
        make_indicator('Gyro Y', 'GY', 'deg/s'),
        make_indicator('Gyro Z', 'GZ', 'deg/s'),
    ]),

    # 가속도 체크박스 + 그래프
    html.Div(style={'backgroundColor': COLORS['panel'], 'borderRadius': '8px',
                     'padding': '12px 16px', 'marginBottom': '16px'}, children=[
        dcc.Checklist(
            id='accel-check',
            options=[
                {'label': html.Span(' AX', style={'color': COLORS['AX']}), 'value': 'AX'},
                {'label': html.Span(' AY', style={'color': COLORS['AY']}), 'value': 'AY'},
                {'label': html.Span(' AZ', style={'color': COLORS['AZ']}), 'value': 'AZ'},
            ],
            value=['AX', 'AY', 'AZ'],
            inline=True,
            style={'marginBottom': '8px'},
            inputStyle={'marginRight': '4px'},
            labelStyle=CHECKBOX_STYLE,
        ),
        dcc.Graph(id='accel-graph', style={'height': '280px'},
                  config={'displayModeBar': False}),
    ]),

    # 자이로 체크박스 + 그래프
    html.Div(style={'backgroundColor': COLORS['panel'], 'borderRadius': '8px',
                     'padding': '12px 16px'}, children=[
        dcc.Checklist(
            id='gyro-check',
            options=[
                {'label': html.Span(' GX', style={'color': COLORS['GX']}), 'value': 'GX'},
                {'label': html.Span(' GY', style={'color': COLORS['GY']}), 'value': 'GY'},
                {'label': html.Span(' GZ', style={'color': COLORS['GZ']}), 'value': 'GZ'},
            ],
            value=['GX', 'GY', 'GZ'],
            inline=True,
            style={'marginBottom': '8px'},
            inputStyle={'marginRight': '4px'},
            labelStyle=CHECKBOX_STYLE,
        ),
        dcc.Graph(id='gyro-graph', style={'height': '280px'},
                  config={'displayModeBar': False}),
    ]),

    # 타이머
    dcc.Interval(id='interval', interval=UPDATE_MS, n_intervals=0),
])

@app.callback(
    Output('accel-graph', 'figure'),
    Output('gyro-graph', 'figure'),
    Output('ind-AX', 'children'),
    Output('ind-AY', 'children'),
    Output('ind-AZ', 'children'),
    Output('ind-GX', 'children'),
    Output('ind-GY', 'children'),
    Output('ind-GZ', 'children'),
    Input('interval', 'n_intervals'),
    Input('accel-check', 'value'),
    Input('gyro-check', 'value'),
)
def update(_, accel_axes, gyro_axes):
    t = list(timestamps)

    # 가속도 그래프 — 체크된 축만 표시
    accel_fig = go.Figure(layout=make_graph_layout('Accelerometer', 'g'))
    for axis in accel_axes:
        accel_fig.add_trace(go.Scatter(
            x=t, y=list(data[axis]), name=axis,
            line=dict(color=COLORS[axis], width=2),
        ))

    # 자이로 그래프 — 체크된 축만 표시
    gyro_fig = go.Figure(layout=make_graph_layout('Gyroscope', 'deg/s'))
    for axis in gyro_axes:
        gyro_fig.add_trace(go.Scatter(
            x=t, y=list(data[axis]), name=axis,
            line=dict(color=COLORS[axis], width=2),
        ))

    # 현재 값 텍스트
    indicators = []
    for key in ['AX', 'AY', 'AZ', 'GX', 'GY', 'GZ']:
        indicators.append(f'{latest[key]:.2f}')

    return accel_fig, gyro_fig, *indicators

# ── 실행 ──────────────────────────────────────────────
if __name__ == '__main__':
    thread = threading.Thread(target=serial_reader, daemon=True)
    thread.start()
    print("[INFO] 브라우저에서 http://127.0.0.1:8050 을 열어주세요")
    app.run(debug=False, host='127.0.0.1', port=8050)
