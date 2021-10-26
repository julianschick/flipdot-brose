from PySide2.QtCore import QObject, QByteArray, QTimer
from PySide2.QtNetwork import QTcpSocket
from qutils.qcommand import State, QCommand
from typing import Iterable, List
import util


class FlipdotTxWorker(QObject):

    def __init__(self, parent):
        QObject.__init__(self, parent)
        self._url: str = '192.168.178.22'
        self._port: int = 3000
        self._socket = QTcpSocket(self)
        self._socket.error.connect(self.__socketError)
        self._socket.connected.connect(self.__socketConnected)
        self._socket.disconnected.connect(self.__socketDisconnected)
        self._socket.readyRead.connect(self.__socketReadyRead)
        self._socket.bytesWritten.connect(self.__socketBytesWritten)
        self._cmdList: List[QCommand] = []
        self._rxBuffer: bytearray = bytearray()
        self._closeTimer: QTimer = util.createSingleShotTimer(1600, self.__closeTimerTimeout)

    def setTarget(self, host: str, port: int):
        pass

    def sendCommands(self, cmds: Iterable[QCommand]) -> None:
        for cmd in cmds:
            self.sendCommand(cmd)

    def sendCommand(self, cmd: QCommand) -> None:
        if cmd.state != State.FRESH:
            raise RuntimeWarning("Unfresh command given, the command has not been queued.")
            return

        if self._socket.state() == QTcpSocket.UnconnectedState:
            print("re-establish")
            self._rxBuffer.clear()
            self._socket.connectToHost(self._url, self._port)
            print("Queued: " + str(cmd))
            self._cmdList.append(cmd)
        elif self._socket.state() == QTcpSocket.HostLookupState:
            print("Queued: " + str(cmd))
            self._cmdList.append(cmd)
        elif self._socket.state() == QTcpSocket.ConnectedState:
            print("Queued: " + str(cmd))
            self._cmdList.append(cmd)
            self.__sendNext()

    def __socketError(self, socketError):
        print(socketError)

        if socketError in [QTcpSocket.ConnectionRefusedError, QTcpSocket.HostNotFoundError, QTcpSocket.NetworkError]:
            for cmd in filter(lambda c: c.state == State.FRESH, self._cmdList):
                cmd.connectionImpossible()
                self._cmdList.remove(cmd)

    def _restartTimer(self):
        if self._closeTimer.isActive():
            self._closeTimer.stop()
        self._closeTimer.start()

    def __closeTimerTimeout(self):
        if self._socket.isOpen():
            print("closing")
            self._socket.close()
        self._rxBuffer.clear()

    def __sendNext(self):
        toSend = next(filter(lambda cmd: cmd.state == State.FRESH, self._cmdList), None)
        if toSend:
            toSend.transmit(self._socket)

    def __socketConnected(self):
        print("connected")
        self._restartTimer()
        self.__sendNext()

    def __socketBytesWritten(self, n):
        self._restartTimer()

        f = filter(lambda cmd: cmd.state == State.TX_PENDING, self._cmdList)
        while cmd := next(f, None):
            n = cmd.bytesWritten(n)
            if n == 0:
                break

        self.__sendNext()

    def __socketDisconnected(self):
        self._closeTimer.stop()

        if next(filter(lambda cmd: cmd.state == State.FRESH, self._cmdList), None):
            print("re-establish2")
            self._socket.connectToHost(self._url, self._port)

    def __socketReadyRead(self):
        self._restartTimer()
        self._rxBuffer += bytearray(self._socket.readAll())

        copied = self._cmdList.copy()
        for cmd in copied:
            if cmd.state == State.RX_PENDING:
                r = cmd.check_response(self._rxBuffer)
                if r.ok:
                    self._cmdList.remove(cmd)
                    self.__sendNext()
                else:
                    break

