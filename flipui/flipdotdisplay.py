from PySide2.QtCore import QObject, QByteArray, QTimer
from PySide2.QtNetwork import QTcpSocket
from commands import Command, Clear, Fill, GetAllLEDs, State, WhoAreYou
import commands as cx
from bitarray import bitarray


class FlipdotTxWorker(QObject):

    def __init__(self, parent):
        QObject.__init__(self, parent)
        self._url = 'flipdot1'
        self._port = 3000
        self._socket = QTcpSocket(self)
        self._socket.error.connect(self.error)
        self._socket.connected.connect(self.connected)
        self._socket.disconnected.connect(self.socketDisconnected)
        self._socket.readyRead.connect(self.socketReadyRead)
        self._socket.bytesWritten.connect(self.socketBytesWritten)
        self._cmdList = []
        self._rxBuffer = QByteArray()
        self._closeTimer = QTimer()#.singleShot(1500, self.closeTimeout)
        self._closeTimer.setSingleShot(True)
        self._closeTimer.setInterval(1600)
        self._closeTimer.timeout.connect(self.closeTimeout)

    def error(self, socketError):
        print(socketError)

        if socketError in [QTcpSocket.ConnectionRefusedError, QTcpSocket.HostNotFoundError, QTcpSocket.NetworkError]:
            for cmd in filter(lambda c: c.state == State.FRESH, self._cmdList):
                cmd.connectionImpossible()
                self._cmdList.remove(cmd)

    def _restartTimer(self):
        if self._closeTimer.isActive():
            self._closeTimer.stop()
        self._closeTimer.start()

    def closeTimeout(self):
        if self._socket.isOpen():
            print("closing")
            self._socket.close()

    def _sendNext(self):
        toSend = next(filter(lambda cmd: cmd.state == State.FRESH, self._cmdList), None)
        if toSend:
            toSend.transmit(self._socket)

    def connected(self):
        self._restartTimer()
        self._sendNext()

    def socketBytesWritten(self, n):
        self._restartTimer()

        f = filter(lambda cmd: cmd.state == State.TX_PENDING, self._cmdList)
        while cmd := next(f, None):
            n = cmd.bytesWritten(n)
            if n == 0:
                break

        self._sendNext()

    def socketDisconnected(self):
        self._closeTimer.stop()

        if next(filter(lambda cmd: cmd.state == State.FRESH, self._cmdList), None):
            print("re-establish2")
            self._socket.connectToHost(self._url, self._port)

    def socketReadyRead(self):
        self._restartTimer()
        self._rxBuffer.append(self._socket.readAll())

        for cmd in self._cmdList:
            if cmd.state == State.RX_PENDING:
                if not cmd.checkResponse(self._rxBuffer):
                    break
                else:
                    self._cmdList.remove(cmd)

    def sendCommand(self, cmd: Command):
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
        c = GetAllLEDs()
        c.success.connect(self.callbackGetAllLEDs)
        self._tx_worker.sendCommand(c)

    def set_all_leds(self, color):
        pass

    def fill(self):
        self._tx_worker.sendCommand(Fill())

    def clear(self):
        self._tx_worker.sendCommand(Clear())

    def test(self):
        c = cx.GetBitset()
        c.success.connect(lambda: print(c.bitset))
        self._tx_worker.sendCommand(c)


    # callbacks
    def callbackGetAllLEDs(self):
        cmd = self.sender()
        colors = cmd.result
        print(colors)


