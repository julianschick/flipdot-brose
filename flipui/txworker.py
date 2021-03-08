from PySide2.QtCore import QObject, QByteArray
from PySide2.QtNetwork import QTcpSocket
from commands import State, Command
from typing import Iterable
import util


class FlipdotTxWorker(QObject):

    def __init__(self, parent):
        QObject.__init__(self, parent)
        self._url = 'flipdot42'
        self._port = 3000
        self._socket = QTcpSocket(self)
        self._socket.error.connect(self.__socketError)
        self._socket.connected.connect(self.__socketConnected)
        self._socket.disconnected.connect(self.__socketDisconnected)
        self._socket.readyRead.connect(self.__socketReadyRead)
        self._socket.bytesWritten.connect(self.__socketBytesWritten)
        self._cmdList = []
        self._rxBuffer = QByteArray()
        self._closeTimer = util.createSingleShotTimer(1600, self.__closeTimerTimeout)

    def setTarget(self, host: str, port: int):
        pass

    def sendCommands(self, cmds: Iterable[Command]) -> None:
        for cmd in cmds:
            self.sendCommand(cmd)

    def sendCommand(self, cmd: Command) -> None:
        if cmd.state != State.FRESH:
            raise RuntimeWarning("Unfresh command given, the command has not been queued.")
            return

        if self._socket.state() == QTcpSocket.UnconnectedState:
            print("re-establish")
            self._socket.connectToHost(self._url, self._port)
            self._cmdList.append(cmd)
        elif self._socket.state() == QTcpSocket.ConnectedState:
            cmd.transmit(self._socket)
            self._cmdList.append(cmd)

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

    def __sendNext(self):
        toSend = next(filter(lambda cmd: cmd.state == State.FRESH, self._cmdList), None)
        if toSend:
            toSend.transmit(self._socket)

    def __socketConnected(self):
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
        self._rxBuffer.append(self._socket.readAll())

        for cmd in self._cmdList:
            if cmd.state == State.RX_PENDING:
                if not cmd.checkResponse(self._rxBuffer):
                    break
                else:
                    self._cmdList.remove(cmd)
