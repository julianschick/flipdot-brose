from PySide2.QtWidgets import QMainWindow, QStatusBar, QMenuBar, QAction

from gfx.rect import Rect
from gui.mainwindow_ui import Ui_MainWindow
from gui.settingsdialog import SettingsDialog
from flipdotdisplay import FlipdotDisplay
from PIL import Image, ImageDraw
from gfx.pixelfont import PixelFont, PixelFontVariant, TextAlignment
from gfx.bitarray_utils import img_to_bitarray
import requests
from datetime import datetime, time, timedelta


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
        url_params = {
            "appid": "",
            "part": "current,minutely,hourly,alerts",
            "units": "metric",
            "lat": "52.48",
            "lon": "13.32"
        }

        r = requests.get("https://api.openweathermap.org/data/2.5/onecall", params = url_params)
        json = r.json()
        daily = json['daily']
        print(daily)

        now = datetime.now()
        if now.time() >= time(18, 0):
            interesting_date = now.date() + timedelta(days=1)
        else:
            interesting_date = now.date()

        for day in daily:
            date = datetime.fromtimestamp(day['dt'])
            if date.date() == interesting_date:
                icon = day['weather'][0]['icon']
                min_temp = round(float(day['temp']['min']))
                max_temp = round(float(day['temp']['max']))
                wind_speed = round(float(day['wind_speed'])* 3.6)
                wind_degrees = float(day['wind_deg'])
                rain_probability = float(day['pop'])
                rain_mm = float(day['rain']) if 'rain' in day.keys() else 0
                snow_mm = float(day['snow']) if 'snow' in day.keys() else 0
                qnh = round(float(day['pressure']))

        directions = {
            "N": 0,
            "NO": 45,
            "O": 90,
            "SO": 135,
            "S": 180,
            "SW": 225,
            "W": 270,
            "NW": 315,
        }

        wind_direction = "?"
        for (name, deg) in directions.items():
            least = deg - 45/2
            most = deg + 45/2

            if least >= 0 and least <= wind_degrees <= most:
                wind_direction = name
            if least < 0 and (wind_degrees >= least + 360 or wind_degrees <= most):
                wind_direction = name

        f = PixelFont('resources/octafont.png', {PixelFontVariant.NORMAL: 9, PixelFontVariant.BOLD: 0})
        img = Image.new('RGB', (28 * 4, 16), (255, 255, 255))

        weather_icon = Image.open(f"resources/{icon}.png")
        img.paste(weather_icon, (0, 0))

        msg1 = f"{min_temp} °C / {max_temp} °C"
        msg2 = f"{wind_speed} km/h aus {wind_direction}"
        msg4 = f"{round(wind_degrees)}°"

        if rain_probability > 0.9:
            msg3 = f"{max(rain_mm, snow_mm)} mm"
        elif rain_probability > 0.0:
            msg3 = f"{round(rain_probability*100)}%"
        else:
            msg3 = f"Q{qnh}"

        msg1variants = []
        msg1variants += [PixelFontVariant.BOLD for _ in f"{min_temp}"]
        msg1variants += [PixelFontVariant.NORMAL for _ in " °C / "]
        msg1variants += [PixelFontVariant.BOLD for _ in f"{max_temp}"]

        f.draw(msg1, img, Rect(24, 0, 80-24, 8), variant_map=msg1variants)
        f.draw(msg2, img, Rect(24, 8, 89-24, 8))
        f.draw(msg3, img, Rect(80, 0, 28*4 - 80 - 2, 8), text_alignment=TextAlignment.RIGHT)
        f.draw(msg4, img, Rect(89, 8, 28*4 - 89 - 2, 8), text_alignment=TextAlignment.RIGHT)

        bits = img_to_bitarray(img)
        self._dsp.show_bitset(bits)


