from PySide2.QtCore import QTimer


def createSingleShotTimer(interval: int, timeoutSlot):
    result = QTimer()
    result.setSingleShot(True)
    result.setInterval(interval)
    result.timeout.connect(timeoutSlot)
    return result
