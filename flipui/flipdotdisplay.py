from PySide2.QtCore import QObject
import commands as cx
from txworker import FlipdotTxWorker
from bitarray import bitarray


class FlipdotDisplay(QObject):

    def __init__(self, width: int, height: int, leds: int, parent = None):
        QObject.__init__(self, parent)
        self._width = width
        self._height = height
        self._leds = leds
        self._pixels = [[False for y in range(height)] for x in range(width)]
        self._tx_worker = FlipdotTxWorker(self)

    @property
    def width(self):
        return self._width

    @property
    def height(self):
        return self._height

    @property
    def leds(self):
        return self._leds

    def connect(self, host, port):
        pass

    def disconnect(self):
        pass

    def is_connected(self):
        pass

    def refresh(self):
        c = cx.GetAllLEDs()
        c.success.connect(self.callbackGetAllLEDs)
        self._tx_worker.sendCommand(c)

    def set_all_leds(self, color):
        pass

    def clear(self):
        self._tx_worker.sendCommand(cx.Clear())

    def fill(self):
        self._tx_worker.sendCommand(cx.Fill())



    def test(self):
        c = cx.Fill()
        c.success.connect(lambda c: print(c))
        self._tx_worker.sendCommand(c)


    # callbacks
    def callbackGetAllLEDs(self):
        cmd = self.sender()
        colors = cmd.result
        print(colors)


