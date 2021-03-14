import time
from enum import Enum
from bitarray import bitarray
from PySide2.QtCore import QByteArray, QObject, Signal, QTimer
from typing import Tuple, List


class T:
    STOP = 0x82
    CONTINUE = 0x84


class R:
    ACK = 0x80
    STATE_UNKNOWN = 0x85
    ADDR_INVALID = 0x86
    BUFFER_OVERFLOW = 0x87


class ErrorType(Enum):
    CONNECTION_ERROR = 1
    RX_TIMEOUT = 2
    TX_TIMEOUT = 3
    COMMUNICATION_ERROR = 4
    BUFFER_OVERFLOW = 5
    BAD_ARGUMENT = 6


class State(Enum):
    FRESH = 1
    TX_PENDING = 2
    RX_PENDING = 3
    SUCCESSFUL = 4
    ERRONEOUS = 5


class DisplayMode(Enum):
    OVERRIDE = 0
    INCREMENTAL = 1


class LEDTransitionMode(Enum):
    IMMEDIATE = 0x01
    LINEAR_SLOW = 0x02
    LINEAR_MEDIUM = 0x03
    LINEAR_QUICK = 0x04
    SLIDE_SLOW = 0x05
    SLIDE_MEDIUM = 0x06
    SLIDE_QUICK = 0x07


class PixelTransitionMode(Enum):
    LEFT_TO_RIGHT = 0x01
    RIGHT_TO_LEFT = 0x02
    RANDOM = 0x03
    TOP_DOWN = 0x04
    BOTTOM_UP = 0x05


class Command(QObject):

    success = Signal(QObject)
    error = Signal((QObject, ErrorType,))

    def __init__(self, name, payload, parent = None):
        QObject.__init__(self, parent)
        self._payload = payload
        self._result = None
        self._name = name

        self._issuedTime = time.time()
        self._txTime = None
        self._rxTime = None

        self._state = State.FRESH
        self._errorType = None

        self._bytesToWrite = 0

        self._rxTimer = QTimer(self)
        self._rxTimer.setSingleShot(True)
        self._rxTimer.setInterval(1000)
        self._rxTimer.timeout.connect(self.rxTimerTimeout)

        self._txTimer = QTimer(self)
        self._txTimer.setSingleShot(True)
        self._txTimer.setInterval(1000)
        self._txTimer.timeout.connect(self.txTimerTimeout)

    @property
    def name(self):
        return self._name

    def __str__(self):
        return f"{self.name} <STATE={self._state.name}>"

    @property
    def result(self):
        return self._result

    @property
    def state(self):
        return self._state

    @property
    def errorCode(self):
        return self._errorType

    @property
    def txTime(self):
        return self._txTime

    @property
    def payload(self):
        return self._payload

    @staticmethod
    def bA(list):
        return QByteArray(bytes(list))

    def transmit(self, socket):
        socket.write(self._payload)
        self._bytesToWrite = len(self._payload)
        self._txTime = time.time()
        self._state = State.TX_PENDING
        print(f"{self} tx_pending")

    def bytesWritten(self, n):
        if n <= self._bytesToWrite:
            self._bytesToWrite -= n
            self._state = State.RX_PENDING
            self._rxTimer.start()
            print(f"{self} rx_pending")
            return 0

        n -= self._bytesToWrite
        self._bytesToWrite = 0
        return n

    def receive(self):
        print(f"{self} receive")
        self._rxTime = time.time()
        self._rxTimer.stop()

    def connectionImpossible(self):
        self._error(ErrorType.CONNECTION_ERROR)

    def rxTimerTimeout(self):
        if self._state == State.RX_PENDING:
            self._error(ErrorType.TIMED_OUT)

    def txTimerTimeout(self):
        if self._state == State.TX_PENDING:
            self._error(ErrorType.TIMED_OUT)

    def checkResponse(self, buffer):
        raise NotImplementedError()

    def _error(self, errorType):
        self._state = State.ERRONEOUS
        self._errorType = errorType
        print(f"{self} {errorType.name}")
        self.error.emit(self, errorType)

    def _success(self):
        self._state = State.SUCCESSFUL
        print(f"{self} success")
        self.success.emit(self)

    def _checkSingleByteResponse(self, buffer: QByteArray) -> bool:
        if len(buffer) < 1:
            return False

        b = buffer.data()[0]
        buffer.remove(0, 1)
        if b == R.ACK:
            self._success()
        else:
            self._error(self._byteToErrorType(b))

        return True

    def _checkMultiByteResponse(self, buffer: QByteArray, n: int) -> bool:
        if len(buffer) < n:
            return False

        raw = buffer[:n].data()
        self._result = [b == R.ACK for b in raw]
        buffer.remove(0, n)

        if errorByte := next(filter(lambda x: x != R.ACK, raw), None):
            self._error(self._byteToErrorType(errorByte))
        else:
            self._success()

        return True

    @staticmethod
    def _byteToErrorType(b: int):
        if b == R.ACK:
            return None
        elif b == R.BUFFER_OVERFLOW:
            return ErrorType.BUFFER_OVERFLOW
        elif b == R.ADDR_INVALID:
            return ErrorType.BAD_ARGUMENT
        else:
            return ErrorType.COMMUNICATION_ERROR


class WhoAreYou(Command):
    def __init__(self):
        super().__init__("Who are you?", self.bA([0x81]))

    def checkResponse(self, buffer : QByteArray):
        print(buffer)
        if len(buffer) < 2:
            return False

        if buffer.data()[0] == 0x70 and buffer.data()[1] == 0x07:
            self._result = True
        else:
            self._result = False

        buffer.remove(0, 2)
        self._success()
        return True


class ShowBitset(Command):

    def __init__(self, bitset: bitarray):
        super().__init__("ShowBitset", self.bA(bytes([0x90, len(bitset) // 8]) + bitset.tobytes()))

    def checkResponse(self, buffer):
        self._checkSingleByteResponse(buffer)


class Clear(Command):

    def __init__(self):
        super().__init__("Clear", self.bA([0x91]))

    def checkResponse(self, buffer):
        print(buffer)
        return self._checkSingleByteResponse(buffer)


class Fill(Command):

    def __init__(self):
        super().__init__("Fill", self.bA([0x92]))

    def checkResponse(self, buffer):
        print(buffer)
        return self._checkSingleByteResponse(buffer)


class SetPixels(Command):

    def __init__(self, pixels: List[Tuple[int, int, bool]]):
        payload = [0x94]
        self._numberOfPixels = len(pixels)

        for pixel in pixels:
            payload.append(0x01 if pixel[2] else 0x00)
            payload.append(max(0, min(255, pixel[0])))
            payload.append(max(0, min(255, pixel[1])))
        payload.append(T.STOP)

        super().__init__("SetPixels", self.bA(payload))

    def checkResponse(self, buffer: QByteArray):
        return self._checkMultiByteResponse(buffer, self._numberOfPixels)


class SetAllLEDs(Command):

    def __init__(self, color: Tuple[int, int, int]):
        super().__init__("SetAllLEDs", self.bA([0xA0] + list(color) + [T.STOP]))

    def checkResponse(self, buffer) -> bool:
        return self._checkSingleByteResponse(buffer)


class SetLEDs(Command):

    def __init__(self, leds: List[Tuple[int, int, int, int]]):
        payload = [0xA1]
        self._numberOfCommands = len(leds)

        for led in leds:
            payload.append(0x7F & led[0])
            payload.extend(list(led)[1:])
        payload.append(T.STOP)

        super().__init__("SetLEDs", self.bA(payload))

    def checkResponse(self, buffer: QByteArray) -> bool:
        return self._checkMultiByteResponse(buffer, self._numberOfCommands)


class GetBitset(Command):

    def __init__(self):
        self._bitset = None
        super().__init__("GetBitset", self.bA([0xB0]))

    def checkResponse(self, buffer):
        if len(buffer) >= 1 and buffer.data()[0] == R.STATE_UNKNOWN:
            buffer.remove(0, 1)
            self._success()
            return True

        n = (112*16 + 7) // 8
        if len(buffer) < n:
            return False

        raw = buffer.data()[:n]
        buffer.remove(0, n)
        self._bitset = bitarray(endian = 'little')
        self._bitset.frombytes(raw)

        self._success()

    @property
    def bitset(self):
        return self._bitset


class GetPixels(Command):

    def __init__(self, pixels: List[Tuple[int, int]]):
        payload = [0xB1]
        self._numberOfPixels = len(pixels)
        self._pixelState = None

        for i in range(len(pixels)):
            payload.extend(list(pixels[i]))
            if i < len(pixels)-1:
                payload.append(T.CONTINUE)
            else:
                payload.append(T.STOP)

        super().__init__("GetPixels", self.bA(payload))

    def checkResponse(self, buffer):
        if len(buffer) < self._numberOfPixels:
            return False

        raw = buffer[:self._numberOfPixels].data()
        print(raw)
        self._result = [b != R.ADDR_INVALID for b in raw]
        self._pixelState = [b if b == 0x00 or b == 0x01 else None for b in raw]
        buffer.remove(0, self._numberOfPixels)

        if errorByte := next(filter(lambda x: x == R.ADDR_INVALID, raw), None):
            self._error(self._byteToErrorType(errorByte))
        else:
            self._success()

        return True

    @property
    def pixelState(self):
        return self._pixelState


class GetAllLEDs(Command):

    def __init__(self):
        super().__init__("GetAllLEDs", self.bA([0xC0]))

    def checkResponse(self, buffer):
        if len(buffer) < 35 * 3:
            return False

        self._result = []
        for i in range(35):
            r = buffer.data()[i * 3 + 0]
            g = buffer.data()[i * 3 + 1]
            b = buffer.data()[i * 3 + 2]
            self._result.append((r,g,b))
        buffer.remove(0, 35 * 3)
        self._success()


class SetIncrementalMode(Command):

    def __init__(self, mode: DisplayMode):
        super().__init__("SetIncrementalMode", self.bA([0xD0, mode.value]))

    def checkResponse(self, buffer):
        return self._checkSingleByteResponse(buffer)


class SetPixelFlipSpeed(Command):

    def __init__(self, speed: int):
        super().__init__("SetPixelFlipSpeed", self.bA([0xD1, speed & 0x00FF, (speed & 0xFF00) >> 8]))

    def checkResponse(self, buffer):
        return self._checkSingleByteResponse(buffer)


class SetLEDTransitionMode(Command):

    def __init__(self, mode: LEDTransitionMode):
        super().__init__("SetLEDTransitionMode", self.bA([0xD2, mode.value]))

    def checkResponse(self, buffer):
        return self._checkSingleByteResponse(buffer)


class SetPixelTransitionMode(Command):

    def __init__(self, mode: PixelTransitionMode):
        super().__init__("SetPixelTransitionMode", self.bA([0xD3, mode.value]))

    def checkResponse(self, buffer):
        return self._checkSingleByteResponse(buffer)


class GetIncrementalMode(Command):

    def __init__(self):
        super().__init__("GetIncrementalMode", self.bA([0xE0]))
        self._mode = None

    @property
    def mode(self) -> DisplayMode:
        return self._mode

    def checkResponse(self, buffer):
        if len(buffer) < 1:
            return False

        value = buffer.data()[0]
        buffer.remove(0, 1)
        try:
            self._mode = DisplayMode(value)
            self._success()
        except ValueError:
            self._error(ErrorType.COMMUNICATION_ERROR)

        return True


class GetPixelFlipSpeed(Command):

    def __init__(self):
        super().__init__("GetPixelFlipSpeed", self.bA([0xE1]))
        self._speed = None

    @property
    def speed(self) -> int:
        return self._speed

    def checkResponse(self, buffer):
        if len(buffer) < 2:
            return False

        lower = buffer.data()[0]
        higher = buffer.data()[1]
        self._speed = ((higher << 8) & 0xFF00) | (lower & 0x00FF)
        self._success()
        return True


class GetLEDTransitionMode(Command):

    def __init__(self):
        super().__init__("GetLEDTransitionMode", self.bA([0xE2]))
        self._mode = None

    @property
    def mode(self) -> LEDTransitionMode:
        return self._mode

    def checkResponse(self, buffer):
        if len(buffer) < 1:
            return False

        value = buffer.data()[0]
        buffer.remove(0, 1)
        try:
            self._mode = LEDTransitionMode(value)
            self._success()
        except ValueError:
            self._error(ErrorType.COMMUNICATION_ERROR)

        return True


class GetPixelTransitionMode(Command):

    def __init__(self):
        super().__init__("GetPixelTransitionMode", self.bA([0xE3]))
        self._mode = None

    @property
    def mode(self) -> PixelTransitionMode:
        return self._mode

    def checkResponse(self, buffer):
        if len(buffer) < 1:
            return False

        value = buffer.data()[0]
        buffer.remove(0, 1)
        try:
            self._mode = PixelTransitionMode(value)
            self._success()
        except ValueError:
            self._error(ErrorType.COMMUNICATION_ERROR)

        return True

