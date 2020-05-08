import socket
from PySide2.QtGui import QPainter, QRegion, QColor
from PySide2.QtWidgets import QWidget
from PySide2.QtCore import QRect, Qt


class FlipdotWidget(QWidget):

    def __init__(self, pixel_queue):
        super().__init__()

        self.pix_width = 28 * 4
        self.pix_height = 16
        self.pix_array = [[False for y in range(self.pix_height)] for x in range(self.pix_width)]

        self.last_mouse_i = -1
        self.last_mouse_j = -1

        self.pixel_queue = pixel_queue

    def setPixWidth(self, value):
        self.pix_width = value

    def setPixHeight(self, value):
        self.pix_height = value

    def paintEvent(self, paintEvent):

        painter = QPainter()
        painter.begin(self)
        self.drawWidget(paintEvent, painter)
        painter.end()

    def mousePressEvent(self, event):
        x = event.x()
        y = event.y()

        size = self.size()
        w = size.width()
        h = size.height()
        tile_w = w / self.pix_width
        tile_h = h / self.pix_height

        i = min(int((x / self.size().width()) * self.pix_width), self.pix_width - 1)
        j = min(int((y / self.size().height()) * self.pix_height), self.pix_height - 1)

        self.pix_array[i][j] = not self.pix_array[i][j]
        self.flipPixel(i, j, self.pix_array[i][j])

        self.update(QRect(i * tile_w, j * tile_h, tile_w, tile_h))

    def mouseMoveEvent(self, event):
        x = event.x()
        y = event.y()

        size = self.size()
        w = size.width()
        h = size.height()
        tile_w = w / self.pix_width
        tile_h = h / self.pix_height


        i = min(int((x / self.size().width()) * self.pix_width), self.pix_width-1)
        j = min(int((y / self.size().height()) * self.pix_height), self.pix_height-1)

        if self.last_mouse_i != -1 and self.last_mouse_j != -1:
            if self.last_mouse_i != i or self.last_mouse_j != j:
                if event.buttons() == Qt.LeftButton:
                    self.pix_array[i][j] = True
                elif event.buttons() == Qt.RightButton:
                    self.pix_array[i][j] = False

                self.flipPixel(i, j, self.pix_array[i][j])
                self.update(QRect(i * tile_w, j * tile_h, tile_w, tile_h))

        self.last_mouse_i = i
        self.last_mouse_j = j


    def flipPixel(self, x, y, direction):
        self.pixel_queue.put((min(255, max(x, 0)), min(255, max(y, 0)), direction))


    def drawWidget(self, paintEvent, painter):

        size = self.size()
        w = size.width()
        h = size.height()
        tile_w = w / self.pix_width
        tile_h = h / self.pix_height

        for i in range (self.pix_width):
            for j in range(self.pix_height):

                r = QRect(i*tile_w + 2, j*tile_h + 2, tile_w - 4, tile_h - 4)

                if paintEvent.region().contains(r):
                    painter.setPen(QColor(0, 0, 0))
                    if self.pix_array[i][j]:
                        painter.setBrush(QColor(255, 128, 0))
                    else:
                        painter.setBrush(QColor(0, 0, 0))

                    painter.drawRect(r)