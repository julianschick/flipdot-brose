from PySide2.QtWidgets import QApplication, QLabel, QGroupBox, QVBoxLayout, QHBoxLayout, QWidget, QPushButton, QSlider, QGridLayout, QMainWindow, QAction, QFileDialog, QCheckBox, QComboBox, QColorDialog
from PySide2 import QtCore, QtGui
import socket
from functools import partial
import requests
import concurrent.futures
from flipdotwidget import FlipdotWidget
from queue import Queue
from txworker import TxWorker
import time
import image2bitarray
from PIL import Image
from bitarray import bitarray
from flipdotdisplay import FlipdotDisplay

executor = concurrent.futures.ThreadPoolExecutor(max_workers=1)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
#sock.setsockopt(socket.SOL_IP, socket.IP_TTL, 50)
#sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
#sock.setblocking(0)
#IP = "192.168.178.70"
IP = "flipdot1"
PORT = 3000
BUFFER_SIZE = 2048

color_dirty = False
img = None


def timer_timeout():
    print("timer1")



def post_message(msg):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(5)
    s.connect((IP, PORT))
    s.send(msg)
    #time.sleep(0.8)
    data = s.recv(BUFFER_SIZE)
    print("r = " + str(data))
    s.close()

def inc_button_clicked():
    msg = bytes([0xD0, 0x01])
    post_message(msg);


def ovr_button_clicked():
    msg = bytes([0xD0, 0x00])
    post_message(msg);



def clear_display():
    dsp.clear()


def light_display():
    dsp.fill()


def load_image():
    global img
    file_name = QFileDialog.getOpenFileName()
    img = Image.open(file_name[0])
    slider_value_changed()


def slider_value_changed():
    if img:
        if not checkbox.isChecked():
            op = " < "
        else:
            op = " >= "

        bits = image2bitarray.convert_image_to_bitarray(img, "l" + op + str(img_slider.value()))
        msg = [0x90, bits.length() // 8];
        msg.extend(bits.tobytes())
        executor.submit(post_message, bytes(msg))


def show_pixels(pixels):
    bits = bitarray(16*28*4, 'little')

    for x in range(0, 28*4):
        for y in range(0, 16):
            bits[x*16 + y] = pixels[x][y]

    msg = [0x90, bits.length() // 8];
    msg.extend(bits.tobytes())
    executor.submit(post_message, bytes(msg))


def _ab_clicked(a, b):
    pixels = [[False for y in range(0, 16)] for x in range(0, 28*4)]

    for x in range(0, 28*4):
        for y in range(0, 16):
            pixels[x][y] = x % 2 == a and y % 2 == b

    show_pixels(pixels)

def _11_clicked():
    _ab_clicked(1, 1)

def _10_clicked():
    _ab_clicked(1, 0)

def _01_clicked():
    _ab_clicked(0, 1)

def _00_clicked():
    _ab_clicked(0, 0)


def set_speed():
    speed = 1000
    lo = speed & 0xFF
    hi = (speed >> 8) & 0xFF
    MESSAGE = bytes([0xD1, lo, hi])
    post_message(MESSAGE)


def test2():
    dsp.test()




def test():
    #MESSAGE = bytes([0xA4, 0x00, 0, 0, 0x01, 1, 1, 0x01])
    #executor.submit(post_message, MESSAGE)

    #MESSAGE = bytes([0xC1, 0, 0, 1, 1])
    #executor.submit(post_message, MESSAGE)
    str = f"{chr(0x12)}Sonderzug{chr(0x11)} nach Pankow\n15:42h   Kurzzug hält vorn"


    msg = [0x93, (len(str) << 2)|0x00]
    msg.extend(str.encode('iso8859_15'))
    post_message(bytes(msg))

    # msg = bytes([0xD0])
    # s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # s.settimeout(5)
    # s.connect((IP, PORT))
    # s.send(msg)
    # data = s.recv(BUFFER_SIZE)
    # s.close()
    #
    # r = []
    # g = []
    # b = []
    #
    # for i in range(0, len(data) // 3):
    #     r.append(data[i*3 + 0])
    #     g.append(data[i*3 + 1])
    #     b.append(data[i*3 + 2])
    #
    # global tx_color
    # tx_color = False
    # dial_map[0].setValue(round(sum(r) / len(r)))
    # dial_map[1].setValue(round(sum(g) / len(g)))
    # dial_map[2].setValue(round(sum(b) / len(b)))
    # tx_color = True


def value_update():
    if led_mode_combo.currentIndex() == 0:
        led_queue.put((dial_map[0].value(), dial_map[1].value(), dial_map[2].value()))
        #MESSAGE = bytes([0xb0, dial_map[0].value(), dial_map[1].value(), dial_map[2].value()])
        #executor.submit(post_message, MESSAGE)

    #sock.sendto(MESSAGE, (UDP_IP, UDP_PORT))
    #session.post(url="http://192.168.178.70/led", data=bytes([dial_map[0].value(), dial_map[1].value(), dial_map[2].value()]))

def commit_leds():
    MESSAGE = bytes([0xA0, dial_map[0].value(), dial_map[1].value(), dial_map[2].value(), 0x82])
    executor.submit(post_message, MESSAGE)

class Window(QMainWindow):
    def __init__(self):
        super(Window, self).__init__()
        self.setGeometry(100, 100, 500, 300)
        self.setWindowTitle("PyQT Show Image")

        openFile = QAction("&File", self)
        openFile.setShortcut("Ctrl+O")
        openFile.setStatusTip("Open File")
        openFile.triggered.connect(self.file_open)

        self.statusBar()

        mainMenu = self.menuBar()

        fileMenu = mainMenu.addMenu('&File')
        fileMenu.addAction(openFile)

        self.lbl = QLabel(self)
        self.setCentralWidget(self.lbl)

        self.home()

    def home(self):
        self.show()

    def file_open(self):
        name = QFileDialog.getOpenFileName(self, 'Open File')
        print(name)
        pixmap = QtGui.QPixmap(name[0])
        self.lbl.setPixmap(pixmap.scaled(self.lbl.size()))


led_queue = Queue()
pixel_queue = Queue()
tx_worker = TxWorker(led_queue, pixel_queue)
tx_worker.setDaemon(True)


app = QApplication([])
group_interactive = QGroupBox('Interactive')
group_display = QGroupBox('Display')
group_leds = QGroupBox('Illumination')
base_layout = QVBoxLayout()

main_window = QWidget()
main_window.setLayout(base_layout)

base_layout.addWidget(group_interactive)
base_layout.addWidget(group_display)
base_layout.addWidget(group_leds)

clear_button = QPushButton("Clear")
clear_button.clicked.connect(clear_display)

light_button = QPushButton("Fill")
light_button.clicked.connect(light_display)

inc_button = QPushButton("Inc")
inc_button.clicked.connect(inc_button_clicked)

ovr_button = QPushButton("Ovr")
ovr_button.clicked.connect(ovr_button_clicked)

test_button = QPushButton("Test")
test_button.clicked.connect(test2)

set_speed_button = QPushButton("Speed")
set_speed_button.clicked.connect(set_speed)


def set_trx_mode(index):
    msg = bytes([0xD3, index + 1])
    post_message(msg)


combo = QComboBox()
combo.addItem("Left to Right")
combo.addItem("Right to Left")
combo.addItem("Random")
combo.addItem("Top down")
combo.addItem("Bottom up")
combo.currentIndexChanged.connect(set_trx_mode)

#update_led_button = QPushButton("Update LEDs")
#update_led_button.clicked.connect(value_update)

image_button = QPushButton("Load image")
image_button.clicked.connect(load_image)

group_interactive.setLayout(QVBoxLayout())
group_interactive.layout().addWidget(FlipdotWidget(pixel_queue))

group_display.setLayout(QVBoxLayout())
box1 = QWidget()
box1.setLayout(QHBoxLayout())
group_display.layout().addWidget(box1)
box1.layout().addWidget(clear_button)
box1.layout().addWidget(light_button)
box1.layout().addWidget(inc_button)
box1.layout().addWidget(ovr_button)

box1.layout().addWidget(test_button)
#box1.layout().addWidget(update_led_button)
box1.layout().addWidget(image_button)
box1.layout().addWidget(combo)
box1.layout().addWidget(set_speed_button)

box2 = QWidget()
box2.setLayout(QHBoxLayout())
group_display.layout().addWidget(box2)

img_slider = QSlider()
img_slider.setMaximum(255)
img_slider.setMinimum(0)
img_slider.setOrientation(QtCore.Qt.Horizontal)
img_slider.valueChanged.connect(slider_value_changed)

checkbox = QCheckBox()
checkbox.stateChanged.connect(slider_value_changed)

box2.layout().addWidget(img_slider)
box2.layout().addWidget(checkbox)

box3 = QWidget()
box3.setLayout(QHBoxLayout())
group_display.layout().addWidget(box3)

_11_button = QPushButton("11")
_10_button = QPushButton("10")
_01_button = QPushButton("01")
_00_button = QPushButton("00")

box3.layout().addWidget(_11_button)
box3.layout().addWidget(_10_button)
box3.layout().addWidget(_01_button)
box3.layout().addWidget(_00_button)

_11_button.clicked.connect(_11_clicked)
_10_button.clicked.connect(_10_clicked)
_01_button.clicked.connect(_01_clicked)
_00_button.clicked.connect(_00_clicked)

group_leds.setLayout(QGridLayout())
color_map = {0: "Red", 1: "Green", 2: "Blue"}
dial_map = {}


def cv(label, value):
    label.setText(str(value))


for i in range(0, 3):
    dial = QSlider()
    dial_map[i] = dial
    label_name = QLabel(color_map[i])
    label_value = QLabel()
    label_name.setAlignment(QtCore.Qt.AlignCenter)
    label_value.setAlignment(QtCore.Qt.AlignCenter)

    dial.valueChanged.connect(partial(cv, label_value))
    dial.valueChanged.connect(value_update)

    dial.setMinimum(0)
    dial.setMaximum(255)
    dial.setValue(0)
    dial.setOrientation(QtCore.Qt.Horizontal)
    label_value.setText("0")

    group_leds.layout().addWidget(label_name, i, 0)
    group_leds.layout().addWidget(dial, i, 1)
    group_leds.layout().addWidget(label_value, i, 2)

led_mode_combo = QComboBox()
led_mode_combo.addItem("Immediate")
led_mode_combo.addItem("Linear Slow")
led_mode_combo.addItem("Linear Medium")
led_mode_combo.addItem("Linear Quick")
led_mode_combo.addItem("Slide Slow")
led_mode_combo.addItem("Slide Medium")
led_mode_combo.addItem("Slide Quick")
led_mode_combo.setCurrentIndex(1)

def set_led_trx_mode(index):
    msg = bytes([0xD2, index + 1])
    post_message(msg)

led_mode_combo.currentIndexChanged.connect(set_led_trx_mode)
commit_led_button = QPushButton("Send")
commit_led_button.clicked.connect(commit_leds)

def color_button_clicked():
    old = QtGui.QColor(dial_map[0].value(), dial_map[1].value(), dial_map[2].value())
    c = QColorDialog.getColor(initial=old)

    if (c != old):
        dial_map[0].setValue(c.red())
        dial_map[1].setValue(c.green())
        dial_map[2].setValue(c.blue())




color_button = QPushButton("color")
color_button.clicked.connect(color_button_clicked)

group_leds.layout().addWidget(led_mode_combo, 3, 1)
group_leds.layout().addWidget(commit_led_button, 3, 2)
group_leds.layout().addWidget(color_button, 3, 0)


dsp = FlipdotDisplay(4*28, 16, main_window)
#dsp.refresh()

#timer = QtCore.QTimer()
#timer.setInterval(5000)
#timer.timeout.connect(timer_timeout)
#timer.start()


tx_worker.start()
main_window.show()
#GUI = Window()
app.exec_()
