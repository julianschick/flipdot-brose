import time
from enum import Enum
from typing import Optional, List
from PySide2.QtCore import QObject, Signal, QTimer, QByteArray
from commands import ErrorType, CommandResult


class State(Enum):
    FRESH = 1
    TX_PENDING = 2
    RX_PENDING = 3
    SUCCESSFUL = 4
    ERRONEOUS = 5


class QCommand(QObject):

    success = Signal(QObject)
    error = Signal((QObject, ErrorType,))

    def __init__(self, wrapped_command: 'Command', parent: Optional[QObject] = None):
        QObject.__init__(self, parent)
        self._wrapped_command = wrapped_command
        self._result = None

        self._issuedTime = time.time()
        self._txTime = None
        self._rxTime = None

        self._state = State.FRESH
        self._errorType = None

        self._bytesToWrite = 0

        self._rxTimer = QTimer(self)
        self._rxTimer.setSingleShot(True)
        self._rxTimer.setInterval(1600)
        self._rxTimer.timeout.connect(self.rxTimerTimeout)

        self._txTimer = QTimer(self)
        self._txTimer.setSingleShot(True)
        self._txTimer.setInterval(1000)
        self._txTimer.timeout.connect(self.txTimerTimeout)

    def __str__(self):
        return f"{self._wrapped_command} <STATE={self._state.name}>"

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

    def transmit(self, socket):
        socket.write(self._wrapped_command.payload)
        self._bytesToWrite = len(self._wrapped_command.payload)
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

    def check_response(self, buffer: bytearray) -> CommandResult:
        command_result: CommandResult = self._wrapped_command.check_response(buffer)
        if command_result.ok:
            self._success()
            return command_result
        else:
            if command_result.error_type != ErrorType.SHORT_RESPONSE:
                self._error(command_result.error_type)
            return command_result

    def _error(self, errorType):
        self._state = State.ERRONEOUS
        self._errorType = errorType
        print(f"{self} {errorType.name}")
        self.error.emit(self, errorType)

    def _success(self):
        self._state = State.SUCCESSFUL
        print(f"{self} success")
        self.success.emit(self)
