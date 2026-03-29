import sys
import math
import numpy as np
from PyQt5.QtWidgets import *
from PyQt5.QtCore import *
from PyQt5.QtGui import *
import pyqtgraph as pg
import random
from serial_com import SerialCom

# =======================================================
# 자연스러운 풍경 + 실제 로봇 비주얼라이저 (레이더 추가)
# =======================================================
class RobotVisualizer(QWidget):
    def __init__(self):
        super().__init__()
        self.pitch = 0.0
        self.yaw = 0.0  # 좌우 회전각(Heading)
        self.scroll_x = 0.0
        self.cloud_offset = 0.0
        self.wheel_angle = 0.0
        self.setMinimumWidth(250)
        
        rng = random.Random(42)
        self._trees = []
        for i in range(25):
            bx = i * 65 + rng.randint(-20, 20)
            sc = 0.5 + rng.random() * 0.7
            tt = rng.choice([0, 0, 0, 1, 1])  
            depth = rng.uniform(0.55, 0.66)  
            self._trees.append((bx, sc, tt, depth))
            
        self._rocks = []
        for i in range(12):
            rx = i * 110 + rng.randint(-30, 30)
            rw = rng.randint(12, 30)
            rh = rng.randint(6, 14)
            self._rocks.append((rx, rw, rh))

    def set_state(self, pitch, yaw, speed):
        self.pitch = pitch
        self.yaw = yaw
        self.scroll_x += speed * 0.6
        self.cloud_offset += 0.3
        self.wheel_angle += speed * 3.0
        self.wheel_angle %= 360
        self.update()

    def _draw_sky(self, p, w, h):
        g = QLinearGradient(0, 0, 0, h * 0.70)
        g.setColorAt(0.0, QColor("#4A90D9")); g.setColorAt(0.25, QColor("#6CB4E8"))
        g.setColorAt(0.5, QColor("#9DD1F5")); g.setColorAt(0.75, QColor("#C8E6FA"))
        g.setColorAt(1.0, QColor("#E2EFD9"))
        p.setBrush(QBrush(g)); p.setPen(Qt.NoPen); p.drawRect(5, 5, w - 10, int(h * 0.70))

    def _draw_sun(self, p, w, h):
        sx = int(w * 0.82 - self.scroll_x * 0.01); sy = int(h * 0.11); sr = 24
        for i in range(6, 0, -1):
            alpha = 15 + i * 8
            p.setBrush(QBrush(QColor(255, 230, 120, alpha))); p.setPen(Qt.NoPen)
            r2 = sr + i * 8; p.drawEllipse(sx - r2, sy - r2, r2 * 2, r2 * 2)
        sg = QRadialGradient(sx, sy, sr)
        sg.setColorAt(0.0, QColor("#FFFEF0")); sg.setColorAt(0.5, QColor("#FFE082")); sg.setColorAt(1.0, QColor("#FFB300"))
        p.setBrush(QBrush(sg)); p.drawEllipse(sx - sr, sy - sr, sr * 2, sr * 2)

    def _draw_clouds(self, p, w, h):
        p.setPen(Qt.NoPen)
        clouds = [(0.08, 0.07, 70, 26), (0.30, 0.14, 90, 32), (0.55, 0.05, 65, 24), (0.78, 0.11, 80, 28), (0.95, 0.08, 55, 20)]
        for cx_r, cy_r, cw, ch in clouds:
            cx = ((cx_r * w * 2.5 - self.scroll_x * 0.12 + self.cloud_offset) % (w + 160)) - 80
            cy = cy_r * h + 8
            for j in range(5):
                ox = cw * (j * 0.2 - 0.3) + (j % 3) * 5; oy = ch * 0.15 * math.sin(j * 1.5)
                sw = cw * (0.45 + 0.15 * math.sin(j * 2.1)); sh = ch * (0.65 + 0.15 * math.cos(j * 1.7))
                p.setBrush(QBrush(QColor(255, 255, 255, 140 + j * 8)))
                p.drawEllipse(int(cx + ox), int(cy + oy), int(sw), int(sh))

    def _smooth_mountain_path(self, points, w, h, base_y):
        path = QPainterPath(); path.moveTo(5, base_y)
        if len(points) < 2: return path
        path.lineTo(points[0][0], points[0][1])
        for i in range(len(points) - 1):
            x0, y0 = points[i]; x1, y1 = points[i + 1]
            cpx = (x0 + x1) / 2
            path.cubicTo(cpx, y0, cpx, y1, x1, y1)
        path.lineTo(w - 5, base_y); path.lineTo(5, base_y); path.closeSubpath()
        return path

    def _draw_far_mountains(self, p, w, h):
        off = self.scroll_x * 0.08; pts = []
        for i in range(9):
            x = 5 + (i / 8) * (w - 10); raw_x = x + off
            y = h * 0.38 + h * 0.08 * math.sin(raw_x * 0.008) + h * 0.05 * math.sin(raw_x * 0.015 + 1.2) + h * 0.03 * math.sin(raw_x * 0.025 + 2.5)
            pts.append((x, y))
        path = self._smooth_mountain_path(pts, w, h, h * 0.58)
        g = QLinearGradient(0, h * 0.25, 0, h * 0.58)
        g.setColorAt(0.0, QColor("#8BA4BE")); g.setColorAt(0.4, QColor("#97B3C5")); g.setColorAt(0.7, QColor("#A5C2B0")); g.setColorAt(1.0, QColor("#B5CEAA"))
        p.setBrush(QBrush(g)); p.setPen(Qt.NoPen); p.drawPath(path)
        mist = QLinearGradient(0, h * 0.50, 0, h * 0.58)
        mist.setColorAt(0.0, QColor(181, 206, 170, 0)); mist.setColorAt(1.0, QColor(181, 206, 170, 80))
        p.setBrush(QBrush(mist)); p.drawRect(5, int(h * 0.50), w - 10, int(h * 0.08))

    def _draw_mid_mountains(self, p, w, h):
        off = self.scroll_x * 0.20; pts = []
        for i in range(10):
            x = 5 + (i / 9) * (w - 10); raw_x = x + off
            y = h * 0.44 + h * 0.06 * math.sin(raw_x * 0.010 + 0.5) + h * 0.04 * math.sin(raw_x * 0.018 + 1.8) + h * 0.02 * math.sin(raw_x * 0.030 + 3.0)
            pts.append((x, y))
        path = self._smooth_mountain_path(pts, w, h, h * 0.64)
        g = QLinearGradient(0, h * 0.38, 0, h * 0.64)
        g.setColorAt(0.0, QColor("#5A8F3E")); g.setColorAt(0.3, QColor("#6AA34D")); g.setColorAt(0.6, QColor("#78B45A")); g.setColorAt(1.0, QColor("#8CC86B"))
        p.setBrush(QBrush(g)); p.setPen(Qt.NoPen); p.drawPath(path)

    def _draw_near_hills(self, p, w, h):
        off = self.scroll_x * 0.40; pts = []
        for i in range(12):
            x = 5 + (i / 11) * (w - 10); raw_x = x + off
            y = h * 0.54 + h * 0.04 * math.sin(raw_x * 0.012 + 0.3) + h * 0.025 * math.sin(raw_x * 0.022 + 1.5) + h * 0.015 * math.sin(raw_x * 0.035 + 2.8)
            pts.append((x, y))
        path = self._smooth_mountain_path(pts, w, h, h * 0.70)
        g = QLinearGradient(0, h * 0.50, 0, h * 0.70)
        g.setColorAt(0.0, QColor("#3E7B22")); g.setColorAt(0.4, QColor("#4D8E30")); g.setColorAt(1.0, QColor("#5FA03E"))
        p.setBrush(QBrush(g)); p.setPen(Qt.NoPen); p.drawPath(path)

    def _draw_tree(self, p, x, y, scale, tt):
        p.setPen(Qt.NoPen)
        if tt == 0:
            tw = int(5 * scale); th = int(20 * scale)
            trunk_g = QLinearGradient(x - tw/2, y, x + tw/2, y)
            trunk_g.setColorAt(0.0, QColor("#5D4037")); trunk_g.setColorAt(1.0, QColor("#4E342E"))
            p.setBrush(QBrush(trunk_g)); p.drawRect(int(x - tw/2), int(y - th), tw, th)
            for ox, oy, lw, lh, color in [(0, -th - 8*scale, 22*scale, 18*scale, QColor("#2E7D32")), (-5*scale, -th - 2*scale, 18*scale, 15*scale, QColor("#388E3C")), (4*scale, -th - 5*scale, 16*scale, 14*scale, QColor("#43A047")), (-2*scale, -th - 14*scale, 14*scale, 12*scale, QColor("#4CAF50"))]:
                p.setBrush(QBrush(color)); p.drawEllipse(int(x + ox - lw/2), int(y + oy - lh/2), int(lw), int(lh))
        else:
            tw = int(4 * scale); th = int(18 * scale)
            p.setBrush(QBrush(QColor("#5D4037"))); p.drawRect(int(x - tw/2), int(y - th), tw, th)
            greens = [QColor("#1B5E20"), QColor("#2E7D32"), QColor("#388E3C"), QColor("#43A047")]
            for i, (bw, bh, yoff) in enumerate([(22, 16, 0), (18, 14, -10), (14, 12, -20), (10, 10, -28)]):
                bw2 = int(bw * scale); bh2 = int(bh * scale); ty = int(y - th + yoff * scale)
                p.setBrush(QBrush(greens[i])); p.drawPolygon(QPolygonF([QPointF(x, ty - bh2), QPointF(x - bw2/2, ty), QPointF(x + bw2/2, ty)]))

    def _draw_trees(self, p, w, h):
        off = self.scroll_x * 0.55
        for bx, sc, tt, depth_ratio in self._trees:
            tx = (bx - off) % (w * 2 + 200) - 200
            if -80 <= tx <= w + 80:
                self._draw_tree(p, tx, h * depth_ratio, sc * (0.6 + (depth_ratio - 0.55) * 3.5), tt)

    def _draw_ground(self, p, w, h):
        fy = h * 0.68
        gg = QLinearGradient(0, fy, 0, h - 5)
        gg.setColorAt(0.0, QColor("#5E9A35")); gg.setColorAt(0.40, QColor("#8B7355")); gg.setColorAt(1.0, QColor("#5C4A3A"))
        p.setBrush(QBrush(gg)); p.setPen(Qt.NoPen); p.drawRect(5, int(fy), w - 10, int(h - fy - 5))
        grass_off = self.scroll_x * 0.8
        p.setPen(QPen(QColor("#4A8520"), 1)); rng = random.Random(7)
        for i in range(40):
            gx = (i * 30 + rng.randint(-10, 10) - grass_off) % (w + 60) - 30
            if 5 <= gx <= w - 5:
                gh = rng.randint(3, 7); p.drawLine(int(gx), int(fy), int(gx - 1), int(fy - gh))
        off = self.scroll_x * 0.9
        for base_x, rw, rh in self._rocks:
            rx = (base_x - off) % (w + 200) - 100
            if -20 <= rx <= w + 20:
                p.setBrush(QBrush(QColor(0, 0, 0, 20))); p.drawEllipse(int(rx + 2), int(fy + 7), rw, int(rh * 0.6))
                rock_g = QRadialGradient(rx + rw/2 - 2, fy + 4 + rh/2 - 2, max(rw, rh) * 0.7)
                rock_g.setColorAt(0.0, QColor("#A09888")); rock_g.setColorAt(1.0, QColor("#706860"))
                p.setBrush(QBrush(rock_g)); p.drawEllipse(int(rx), int(fy + 4), rw, rh)
        road_y = int(fy + 1); p.setPen(QPen(QColor("#9E9680"), 1, Qt.DashDotLine))
        for i in range(0, w + 200, 28):
            x = (i - self.scroll_x * 1.0) % (w + 100) - 50
            if 8 < x < w - 8: p.drawLine(int(x), road_y, int(x + 10), road_y)

    def _draw_wheel_real(self, p, cx, cy, r, s):
        tg = QRadialGradient(cx - int(2*s), cy - int(2*s), r)
        tg.setColorAt(0.0, QColor("#4A4A4A")); tg.setColorAt(0.55, QColor("#2A2A2A"))
        tg.setColorAt(0.8, QColor("#1A1A1A")); tg.setColorAt(1.0, QColor("#0A0A0A"))
        p.setBrush(QBrush(tg)); p.setPen(QPen(QColor("#050505"), max(1, int(2*s))))
        p.drawEllipse(cx-r, cy-r, r*2, r*2)
        p.setPen(QPen(QColor("#3A3A3A"), max(1, int(1.5*s))))
        for i in range(20):
            a = math.radians(i*18 + self.wheel_angle)
            x1=cx+int((r-5*s)*math.cos(a)); y1=cy+int((r-5*s)*math.sin(a))
            x2=cx+int((r-1)*math.cos(a)); y2=cy+int((r-1)*math.sin(a))
            p.drawLine(x1, y1, x2, y2)
        rim_r = int(r*0.58)
        p.setBrush(QBrush(QColor("#F9A825"))); p.setPen(QPen(QColor("#F57F17"), max(1, int(1.5*s))))
        p.drawEllipse(cx-rim_r, cy-rim_r, rim_r*2, rim_r*2)
        si = int(rim_r*0.35); so = int(rim_r*0.85)
        p.setBrush(QBrush(QColor("#2A2A2A"))); p.setPen(Qt.NoPen)
        for i in range(5):
            a1 = math.radians(i*72 + self.wheel_angle + 10)
            a2 = math.radians(i*72 + self.wheel_angle + 55)
            am = math.radians(i*72 + self.wheel_angle + 32.5)
            path = QPainterPath()
            path.moveTo(cx+int(si*math.cos(a1)), cy+int(si*math.sin(a1)))
            path.lineTo(cx+int(so*math.cos(a1)), cy+int(so*math.sin(a1)))
            path.cubicTo(cx+int((so+2)*math.cos(am)), cy+int((so+2)*math.sin(am)),
                         cx+int((so+2)*math.cos(am)), cy+int((so+2)*math.sin(am)),
                         cx+int(so*math.cos(a2)), cy+int(so*math.sin(a2)))
            path.lineTo(cx+int(si*math.cos(a2)), cy+int(si*math.sin(a2)))
            path.closeSubpath(); p.drawPath(path)
        hub_r = int(si)
        p.setBrush(QBrush(QColor("#F9A825"))); p.setPen(QPen(QColor("#E65100"), 1))
        p.drawEllipse(cx-hub_r, cy-hub_r, hub_r*2, hub_r*2)
        p.setBrush(QBrush(QColor("#5D4037"))); p.setPen(Qt.NoPen)
        cr = max(2, int(2*s)); p.drawEllipse(cx-cr, cy-cr, cr*2, cr*2)
        hl = QRadialGradient(cx-int(r*0.25), cy-int(r*0.25), int(r*0.45))
        hl.setColorAt(0.0, QColor(255,255,255,50)); hl.setColorAt(1.0, QColor(255,255,255,0))
        p.setBrush(QBrush(hl)); p.setPen(Qt.NoPen)
        p.drawEllipse(cx-r, cy-r, r*2, r*2)

    # =============== 원본 로봇 디테일 완벽 복구 ===============
    def _draw_robot(self, p, w, h):
        fy = h * 0.68
        p.save()
        p.translate(w/2, fy - 2)
        p.rotate(self.pitch)
        s = min(w, h) / 300.0
        WR = int(34*s)
        dx = int(10*s); dy = int(-6*s)

        # 그림자
        p.setBrush(QBrush(QColor(0,0,0,30))); p.setPen(Qt.NoPen)
        p.drawEllipse(int(-WR-12*s), int(WR), int(WR*2+24*s), int(14*s))
        # 뒤쪽 바퀴
        bwr = int(WR*0.82)
        p.setBrush(QBrush(QColor("#1A1A1A"))); p.setPen(QPen(QColor("#0A0A0A"), max(1, int(1.5*s))))
        p.drawEllipse(dx-bwr, dy-bwr, bwr*2, bwr*2)
        p.setBrush(QBrush(QColor("#C8A415"))); p.setPen(Qt.NoPen)
        brimr = int(bwr*0.55); p.drawEllipse(dx-brimr, dy-brimr, brimr*2, brimr*2)

        # 프레임
        fw = int(65*s); plate_h = int(4*s)
        p1_y = int(-WR + 3*s); p2_y = p1_y - int(50*s)
        pw = int(4*s); spacer_h = abs(p2_y - p1_y)
        sp1 = QColor("#DAA520"); sp2 = QColor("#B8860B")
        acrylic = QColor(200, 220, 235, 100)
        acrylic_pen = QPen(QColor(180, 200, 215, 150), max(1, int(1*s)))

        # 뒤쪽 기둥+플레이트
        for sp_x in [int(-fw/2+6*s)+dx, int(fw/2-10*s)+dx]:
            sg2 = QLinearGradient(sp_x, p1_y+dy, sp_x+pw, p1_y+dy)
            sg2.setColorAt(0.0, sp1); sg2.setColorAt(0.5, QColor("#FFD700")); sg2.setColorAt(1.0, sp2)
            p.setBrush(QBrush(sg2)); p.setPen(Qt.NoPen)
            p.drawRect(sp_x, p2_y+plate_h+dy, pw, spacer_h-plate_h)
        for py in [p1_y+dy, p2_y+dy]:
            p.setBrush(QBrush(acrylic)); p.setPen(acrylic_pen)
            p.drawRoundedRect(int(-fw/2)+dx, py, fw, plate_h, int(1*s), int(1*s))

        # 축 연결
        p.setBrush(QBrush(QColor("#666666"))); p.setPen(Qt.NoPen)
        p.drawPolygon(QPolygonF([QPointF(-3*s,-2*s),QPointF(dx-3*s,dy-2*s),
                                  QPointF(dx+3*s,dy+2*s),QPointF(3*s,2*s)]))

        # 앞쪽 기둥
        for sp_x in [int(-fw/2+6*s), int(fw/2-10*s)]:
            sg3 = QLinearGradient(sp_x, p1_y, sp_x+pw, p1_y)
            sg3.setColorAt(0.0, sp1); sg3.setColorAt(0.5, QColor("#FFD700")); sg3.setColorAt(1.0, sp2)
            p.setBrush(QBrush(sg3)); p.setPen(QPen(QColor("#8B6914"), 1))
            p.drawRect(sp_x, p2_y+plate_h, pw, spacer_h-plate_h)

        # 1층 컴포넌트: 노란 모터
        mw2=int(22*s); mh2=int(16*s); my=int(p1_y-mh2+2*s)
        mg2=QLinearGradient(-fw/2+4*s,my,-fw/2+4*s+mw2,my+mh2)
        mg2.setColorAt(0.0,QColor("#FFD54F"));mg2.setColorAt(0.5,QColor("#FBC02D"));mg2.setColorAt(1.0,QColor("#F9A825"))
        p.setBrush(QBrush(mg2));p.setPen(QPen(QColor("#F57F17"),1))
        p.drawRect(int(-fw/2+4*s),my,mw2,mh2)
        p.setBrush(QBrush(QColor("#757575")));p.setPen(QPen(QColor("#616161"),1))
        p.drawRect(int(-fw/2+4*s+mw2),my+int(3*s),int(8*s),int(10*s))
        p.setPen(QPen(QColor("#9E9E9E"),max(1,int(2*s))))
        p.drawLine(int(-fw/2+4*s+mw2+8*s),my+int(8*s),0,0)

        # 1층 컴포넌트: L298N
        lw=int(20*s);lh=int(18*s);lx=int(-lw/2+2*s);ly=int(p1_y-lh-2*s)
        p.setBrush(QBrush(QColor("#C62828")));p.setPen(QPen(QColor("#B71C1C"),1))
        p.drawRect(lx,ly,lw,lh)
        p.setBrush(QBrush(QColor("#212121")));p.setPen(Qt.NoPen)
        p.drawRect(lx+int(3*s),ly+int(2*s),int(14*s),int(10*s))
        p.setBrush(QBrush(QColor("#1565C0")))
        p.drawRect(lx+int(1*s),ly+lh-int(4*s),int(6*s),int(4*s))
        p.drawRect(lx+lw-int(7*s),ly+lh-int(4*s),int(6*s),int(4*s))
        p.setBrush(QBrush(QColor("#F44336")));p.drawEllipse(lx+int(8*s),ly+int(13*s),int(3*s),int(3*s))

        # 1층 컴포넌트: 18650 배터리
        bw2=int(28*s);bh2=int(8*s);bx2=int(5*s)
        for bi in range(2):
            by=int(p1_y-8*s-bi*int(9*s))
            bg2=QLinearGradient(bx2,by,bx2+bw2,by)
            bg2.setColorAt(0.0,QColor("#1565C0"));bg2.setColorAt(0.3,QColor("#1E88E5"))
            bg2.setColorAt(0.7,QColor("#1565C0"));bg2.setColorAt(1.0,QColor("#0D47A1"))
            p.setBrush(QBrush(bg2));p.setPen(QPen(QColor("#0D47A1"),1))
            p.drawRoundedRect(bx2,by,bw2,bh2,int(3*s),int(3*s))
            p.setBrush(QBrush(QColor("#90A4AE")));p.setPen(Qt.NoPen)
            p.drawRect(bx2+bw2,by+int(2*s),int(2*s),int(4*s))
            p.setPen(QPen(QColor("#E3F2FD"),1))
            f=p.font();f.setPixelSize(max(4,int(4*s)));f.setBold(True);p.setFont(f)
            p.drawText(bx2,by,bw2,bh2,Qt.AlignCenter,"18650")

        # 앞쪽 아크릴 + 3D 면
        top_color = QColor(220, 235, 248, 100)
        for py, plate_y in [(p1_y, p1_y), (p2_y, p2_y)]:
            p.setBrush(QBrush(QColor(210,230,245,130)));p.setPen(acrylic_pen)
            p.drawRoundedRect(int(-fw/2),py,fw,plate_h,int(1*s),int(1*s))
            tp=QPolygonF([QPointF(int(-fw/2),py),QPointF(int(-fw/2)+dx,py+dy),
                           QPointF(int(fw/2)+dx,py+dy),QPointF(int(fw/2),py)])
            p.setBrush(QBrush(top_color));p.setPen(QPen(QColor(180,200,215,80),1));p.drawPolygon(tp)

        # 2층: NUCLEO 보드
        nw=int(48*s);nh=int(28*s);nx=int(-nw/2);ny=p2_y-nh-int(2*s)
        ng=QLinearGradient(nx,ny,nx+nw,ny+nh)
        ng.setColorAt(0.0,QColor("#F5F5F5"));ng.setColorAt(0.5,QColor("#EEEEEE"));ng.setColorAt(1.0,QColor("#E0E0E0"))
        p.setBrush(QBrush(ng));p.setPen(QPen(QColor("#BDBDBD"),1))
        p.drawRoundedRect(nx,ny,nw,nh,int(2*s),int(2*s))
        p.setBrush(QBrush(QColor("#1565C0")));p.setPen(Qt.NoPen)
        p.drawRect(nx+int(2*s),ny+int(2*s),int(8*s),int(5*s))
        p.setPen(QPen(QColor("#FFFFFF"),1))
        f2=p.font();f2.setPixelSize(max(4,int(4*s)));f2.setBold(True);p.setFont(f2)
        p.drawText(nx+int(2*s),ny+int(2*s),int(8*s),int(5*s),Qt.AlignCenter,"ST")
        p.setBrush(QBrush(QColor("#1A1A1A")));p.setPen(Qt.NoPen)
        cs=int(10*s);p.drawRect(nx+int(14*s),ny+int(8*s),cs,cs)
        p.setPen(QPen(QColor("#9E9E9E"),1))
        f3=p.font();f3.setPixelSize(max(3,int(3*s)));p.setFont(f3)
        p.drawText(nx+int(14*s),ny+int(8*s),cs,cs,Qt.AlignCenter,"STM")
        p.setBrush(QBrush(QColor("#212121")));p.setPen(Qt.NoPen)
        p.drawRect(nx+int(1*s),ny+int(9*s),int(3*s),int(16*s))
        p.drawRect(nx+nw-int(4*s),ny+int(9*s),int(3*s),int(16*s))
        p.setPen(QPen(QColor("#FFD600"),max(1,int(0.8*s))))
        for pi in range(8):
            py2=ny+int(10*s)+int(pi*2*s)
            p.drawLine(nx,py2,nx-int(2*s),py2);p.drawLine(nx+nw,py2,nx+nw+int(2*s),py2)
        p.setBrush(QBrush(QColor("#E0E0E0")));p.setPen(QPen(QColor("#9E9E9E"),1))
        p.drawRect(nx+nw-int(2*s),ny+int(3*s),int(6*s),int(5*s))
        p.setBrush(QBrush(QColor("#333333")));p.drawRect(nx+nw,ny+int(4*s),int(3*s),int(3*s))
        p.setBrush(QBrush(QColor("#2196F3")));p.setPen(Qt.NoPen)
        p.drawEllipse(nx+int(28*s),ny+int(3*s),int(3*s),int(3*s))
        for ri in range(3,0,-1):
            p.setBrush(QBrush(QColor(244,67,54,40+ri*30)))
            p.drawEllipse(nx+int(33*s)-ri,ny+int(3*s)-ri,int(3*s)+ri*2,int(3*s)+ri*2)
        p.setBrush(QBrush(QColor("#F44336")));p.drawEllipse(nx+int(33*s),ny+int(3*s),int(3*s),int(3*s))

        # 2층: MPU6050
        p.setBrush(QBrush(QColor("#7B1FA2")));p.setPen(QPen(QColor("#4A148C"),1))
        p.drawRect(nx+int(35*s),ny-int(2*s),int(8*s),int(8*s))
        p.setBrush(QBrush(QColor("#1A1A1A")));p.setPen(Qt.NoPen)
        p.drawRect(nx+int(37*s),ny,int(4*s),int(4*s))

        # 점퍼 와이어 연결
        wires=[QColor("#F44336"),QColor("#FF5722"),QColor("#2196F3"),QColor("#4CAF50"),QColor("#212121"),QColor("#FF9800")]
        p.setBrush(Qt.NoBrush)
        for i, wc in enumerate(wires):
            p.setPen(QPen(wc,max(1,int(1.5*s))))
            sx2=nx+int((5+i*7)*s)
            path=QPainterPath();path.moveTo(sx2,ny+nh)
            ch2=int((-15-i*4)*s)
            path.cubicTo(sx2+int((i-3)*6*s),ny+ch2,sx2+int((i-2)*8*s),ny+ch2-int(10*s),
                         int((-8+i*5)*s),ly+int(2*s))
            p.drawPath(path)

        # 메인 휠
        self._draw_wheel_real(p, 0, 0, WR, s)
        p.restore()

    # =============== 레이더 (방향 지시기) ===============
    def _draw_radar(self, p, w, h):
        cx = w - 70; cy = 70; r = 45
        p.save()
        p.setBrush(QBrush(QColor(20, 30, 50, 200)))
        p.setPen(QPen(QColor("#7dcfff"), 2))
        p.drawEllipse(cx - r, cy - r, r * 2, r * 2)

        p.setPen(QPen(QColor(125, 207, 255, 100), 1, Qt.DashLine))
        p.drawLine(cx - r, cy, cx + r, cy)
        p.drawLine(cx, cy - r, cx, cy + r)

        p.translate(cx, cy)
        p.rotate(self.yaw) 

        bw, bh = 24, 34
        p.setBrush(QBrush(QColor(220, 230, 240, 220)))
        p.setPen(QPen(QColor(100, 150, 200), 1))
        p.drawRoundedRect(-bw//2, -bh//2, bw, bh, 3, 3)

        p.setBrush(QBrush(QColor("#1a1b26"))); p.setPen(Qt.NoPen)
        p.drawRect(-bw//2 - 6, -10, 6, 20) 
        p.drawRect(bw//2, -10, 6, 20)      

        p.setBrush(QBrush(QColor("#f7768e")))
        p.drawEllipse(-3, -bh//2 - 4, 6, 6)
        p.restore()
        
        p.setPen(QPen(QColor("#c0caf5"), 1))
        f = p.font(); f.setPixelSize(10); f.setBold(True); p.setFont(f)
        p.drawText(cx - r, cy - r - 15, r * 2, 15, Qt.AlignCenter, "HEADING (YAW)")

    def paintEvent(self, event):
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)
        w = self.width(); h = self.height()
        path = QPainterPath(); path.addRoundedRect(QRectF(5, 5, w-10, h-10), 12, 12)
        p.setClipPath(path)
        
        self._draw_sky(p, w, h); self._draw_sun(p, w, h); self._draw_clouds(p, w, h)
        self._draw_far_mountains(p, w, h); self._draw_mid_mountains(p, w, h); self._draw_near_hills(p, w, h)
        self._draw_trees(p, w, h); self._draw_ground(p, w, h)
        
        p.setClipping(False)
        self._draw_robot(p, w, h)
        self._draw_radar(p, w, h)
        
        p.setBrush(Qt.NoBrush); p.setPen(QPen(QColor("#414868"), 2))
        p.drawRoundedRect(5, 5, w-10, h-10, 12, 12)


# =======================================================
# 메인 대시보드 GUI (탭 구조)
# =======================================================
class BalancingBotGUI(QMainWindow):
    def __init__(self, serial_com):
        super().__init__()
        self.ser = serial_com
        self.setWindowTitle("TeleMetrix | Balancing Bot Dashboard")
        self.resize(1050, 780)
        self.setFocusPolicy(Qt.StrongFocus)

        self.setStyleSheet("""
            QMainWindow { background-color: #1a1b26; color: #c0caf5; }
            QLabel { color: #c0caf5; font-family: 'Segoe UI', sans-serif; }
            #HeaderFrame { background-color: #24283b; border-bottom: 3px solid #7aa2f7; }
            QGroupBox { background-color: #24283b; border: 2px solid #414868; border-radius: 12px; margin-top: 25px; padding-top: 20px; font-weight: bold; color: #c0caf5; }
            QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top center; top: 0px; padding: 4px 15px; background-color: #414868; border-radius: 8px; color: #7dcfff; }
            QProgressBar { border: 2px solid #414868; border-radius: 5px; text-align: center; background-color: #1a1b26; color: white; font-weight: bold; }
            QProgressBar::chunk { background-color: #e0af68; }
            QTabWidget::pane { border: 1px solid #414868; background-color: #1a1b26; }
            QTabBar::tab { background-color: #24283b; color: #c0caf5; padding: 10px 30px; border: 1px solid #414868; border-bottom: none; border-top-left-radius: 8px; border-top-right-radius: 8px; font-weight: bold; font-size: 14px; }
            QTabBar::tab:selected { background-color: #1a1b26; color: #7dcfff; border-bottom: 2px solid #7dcfff; }
        """)

        mw = QWidget(); self.setCentralWidget(mw)
        ml = QVBoxLayout(mw); ml.setContentsMargins(0,0,0,0); ml.setSpacing(0)

        # 헤더
        hf = QFrame(); hf.setObjectName("HeaderFrame")
        hl = QHBoxLayout(hf); hl.setContentsMargins(20,15,20,15)
        tl = QLabel("TeleMetrix | Balancing Bot Dashboard")
        tl.setStyleSheet("font-size:22px;font-weight:900;color:#7dcfff;")
        self.status_label = QLabel("DISCONNECTED")
        self.status_label.setStyleSheet("font-size:14px;font-weight:bold;color:#1a1b26;background-color:#f7768e;padding:8px 15px;border-radius:6px;")
        hl.addWidget(tl); hl.addStretch(); hl.addWidget(self.status_label)
        ml.addWidget(hf)

        # 탭
        self.tabs = QTabWidget()
        self.demo_tab = QWidget()
        self.debug_tab = QWidget()
        self.tabs.addTab(self.demo_tab, "DEMO")
        self.tabs.addTab(self.debug_tab, "DEBUG")

        # 데모탭: placeholder
        demo_layout = QVBoxLayout(self.demo_tab)
        demo_layout.addWidget(QLabel("Demo tab - coming soon"))

        # 디버깅탭: placeholder
        debug_layout = QVBoxLayout(self.debug_tab)
        debug_layout.addWidget(QLabel("Debug tab - coming soon"))

        ml.addWidget(self.tabs)

        # 시리얼 연결 상태 업데이트
        self.timer = QTimer()
        self.timer.setInterval(500)
        self.timer.timeout.connect(self._update_status)
        self.timer.start()

    def _update_status(self):
        if self.ser.is_connected:
            t = self.ser.telemetry
            self.status_label.setText(f" PITCH: {t.angle:5.1f} | PID: {t.pid_output:5.1f} | L: {t.left_cmd:4d} | R: {t.right_cmd:4d} ")
            self.status_label.setStyleSheet("font-size:14px;font-weight:bold;color:#1a1b26;background-color:#9ece6a;padding:8px 15px;border-radius:6px;")
        else:
            self.status_label.setText("DISCONNECTED")
            self.status_label.setStyleSheet("font-size:14px;font-weight:bold;color:#1a1b26;background-color:#f7768e;padding:8px 15px;border-radius:6px;")

    def closeEvent(self, event):
        self.ser.disconnect()
        event.accept()


if __name__ == "__main__":
    port = sys.argv[1] if len(sys.argv) > 1 else None
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else 9600

    ser = SerialCom()
    if port:
        if ser.connect(port, baud):
            print(f"[INFO] {port} 연결 완료 (Baud: {baud})")
        else:
            print(f"[WARN] {port} 연결 실패, 오프라인 모드로 실행")
    else:
        print("[INFO] COM 포트 미지정, 오프라인 모드로 실행")
        print("  사용법: python dashboard.py COM27 9600")

    app = QApplication(sys.argv)
    window = BalancingBotGUI(ser)
    window.show()
    sys.exit(app.exec_())