from time import sleep

from PIL import Image
from PySide2.QtCore import QObject
import commands as cx
from gfx.bitarray_utils import img_to_bitarray
from gfx.pixelfont import PixelFont, PixelFontVariant
from gfx.rect import Rect
from qutils.txworker import FlipdotTxWorker
from qutils.qcommand import QCommand


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
        c = cx.SetAllLEDs(color)
        self._tx_worker.sendCommand(QCommand(c, parent=self))

    def show_bitset(self, bitset):
        c = cx.ShowBitset(bitset)
        self._tx_worker.sendCommand(QCommand(c, parent=self))

    def clear(self):
        self._tx_worker.sendCommand(QCommand(cx.Clear(), parent=self))

    def fill(self):
        self._tx_worker.sendCommand(QCommand(cx.Fill(), parent=self))

    def test(self):
        pass
        #c = cx.SetLEDs([(1,0,255,0),(2,0,0,255)])
        #c = cx.SetAllLEDs((0,0,0))
        #c = cx.SetPixelFlipSpeed(500);
        #c = cx.SetIncrementalMode(cx.DisplayMode.INCREMENTAL)
        #c = cx.SetPixelTransitionMode(cx.PixelTransitionMode.RANDOM)

        #commands = [
        #    cx.GetLEDTransitionMode(),
        #    cx.GetPixelTransitionMode(),
        #    cx.GetPixelFlipSpeed(),
        #    cx.GetIncrementalMode()
        #]

        #for c in commands:
        #    self._tx_worker.sendCommand(QCommand(c, parent=self))

        #c = cx.Scroll([False, False, None, False], 16)
        #self._tx_worker.sendCommand(QCommand(c, parent=self))

    # callbacks
    def callbackGetAllLEDs(self):
        cmd = self.sender()
        colors = cmd.result
        print(colors)


