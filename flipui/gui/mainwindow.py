from PIL import Image
from PySide2.QtWidgets import QMainWindow

from gfx.bitarray_utils import img_to_bitarray
from gfx.pixelfont import PixelFont, PixelFontVariant
from gfx.rect import Rect
from gui.mainwindow_ui import Ui_MainWindow
from gui.settingsdialog import SettingsDialog
from qutils.flipdotdisplay import FlipdotDisplay


class MainWindow(QMainWindow):

    def __init__(self):
        super().__init__()

        self._ui = Ui_MainWindow()
        self._ui.setupUi(self)

        self._ui.actionExit.triggered.connect(self.__onActionExit)
        self._ui.actionSettings.triggered.connect(self.__onActionSettings)

        self._dsp = FlipdotDisplay(4 * 28, 16, 36, self)

    def __onActionExit(self):
        self.close()

    def __onActionSettings(self):
        self._settingsDialog = SettingsDialog()
        self._settingsDialog.show()

    def clearButtonClicked(self):
        self._dsp.clear()

    def fillButtonClicked(self):
        self._dsp.fill()

    def testButtonClicked(self):
        # f: 'PixelFont' = PixelFont('resources/octafont.png', {PixelFontVariant.NORMAL: 9, PixelFontVariant.BOLD: 0})
        # img: Image = Image.new('RGB', (28 * 4, 16), (255, 255, 255))
        #
        # " 🡠 🡢 🡡 🡣 🡤 🡥 🡦 🡧  🡥🡢🡦🡣🡧🡠🡤💧"
        # f.draw("🡡🡥🡢🡦🡣🡧🡠🡤\nFAT DROP 💧", img, Rect(0, 0, 1000, 16), variant=PixelFontVariant.NORMAL)
        #
        # bits = img_to_bitarray(img)
        # self._dsp.show_bitset(bits)

        self._dsp.test()



