from PySide2.QtWidgets import QDialog
from PySide2.QtCore import QSettings
from gui.settingsdialog_ui import Ui_SettingsDialog


class SettingsDialog(QDialog):
    def __init__(self):
        super().__init__()

        self._ui = Ui_SettingsDialog()
        self._ui.setupUi(self)
        self.accepted.connect(self.__accepted)

        settings = QSettings()
        self._ui.hostLineEdit.setText(settings.value("hostname", ""))
        self._ui.portSpinBox.setValue(settings.value("port", 3000, int))

    def __accepted(self):
        settings = QSettings()
        settings.setValue("hostname", self._ui.hostLineEdit.text())
        settings.setValue("port", self._ui.portSpinBox.value())
