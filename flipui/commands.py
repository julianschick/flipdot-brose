from enum import Enum
from math import ceil

from bitarray import bitarray
from typing import Tuple, List, Optional


class T:
    STOP = 0x82
    CONTINUE = 0x84


class R:
    ACK = 0x80
    INVALID_COMMAND = 0x83
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
    SHORT_RESPONSE = 7


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


class CommandResult:
    def __init__(self, ok: bool, error_type: Optional[ErrorType] = None, result_data = None):
        self._ok = ok
        self._error_type = error_type
        self._result_data = result_data

    @staticmethod
    def error(error_type: ErrorType) -> 'CommandResult':
        return CommandResult(False, error_type=error_type)

    @staticmethod
    def success(result_data=None) -> 'CommandResult':
        return CommandResult(True, result_data=result_data)

    @staticmethod
    def short():
        return CommandResult(False, error_type=ErrorType.SHORT_RESPONSE)

    @property
    def ok(self) -> bool:
        return self._ok

    @property
    def error_type(self) -> ErrorType:
        return self._error_type

    @property
    def result_data(self):
        return self._result_data

    @property
    def is_response_too_short(self):
        return not self.ok and self.error == ErrorType.SHORT_RESPONSE


class Command:

    def __init__(self, name, payload):
        self._name = name
        self._payload = payload

    @property
    def payload(self):
        return self._payload

    @property
    def name(self):
        return self._name

    def __str__(self):
        return self.name

    def check_response(self, buffer: bytearray) -> CommandResult:
        raise NotImplementedError()

    def _check_single_byte_response(self, buffer: bytearray) -> CommandResult:
        if len(buffer) < 1:
            return CommandResult.error(ErrorType.SHORT_RESPONSE)

        b = buffer[0]
        del buffer[0]

        if b == R.ACK:
            return CommandResult.success()

        return CommandResult.error(self._byte_to_error_type(b))

    def _check_multi_byte_response(self, buffer: bytearray, n: int) -> 'CommandResult':
        if len(buffer) < n:
            return CommandResult.error(ErrorType.SHORT_RESPONSE)

        raw = buffer[:n]
        self._result = [b == R.ACK for b in raw]
        del buffer[0:n]

        if errorByte := next(filter(lambda x: x != R.ACK, raw), None):
            return CommandResult.error(self._byte_to_error_type(errorByte))

        return CommandResult.success()

    @staticmethod
    def _byte_to_error_type(b: int) -> ErrorType:
        if b == R.BUFFER_OVERFLOW:
            return ErrorType.BUFFER_OVERFLOW
        elif b == R.ADDR_INVALID:
            return ErrorType.BAD_ARGUMENT
        else:
            return ErrorType.COMMUNICATION_ERROR


class WhoAreYou(Command):
    def __init__(self):
        super().__init__("Who are you?", bytearray([0x81]))

    def check_response(self, buffer: bytearray) -> CommandResult:
        print(buffer)
        if len(buffer) < 2:
            CommandResult.error(ErrorType.SHORT_RESPONSE)

        signature = buffer[0:2]
        del buffer[0:2]

        if signature[0] == 0x70 and signature[1] == 0x07:
            return CommandResult.success()

        return CommandResult.error(ErrorType.COMMUNICATION_ERROR)


class ShowBitset(Command):

    def __init__(self, bitset: bitarray):
        super().__init__("ShowBitset", bytearray(bytes([0x90, len(bitset) // 8]) + bitset.tobytes()))

    def check_response(self, buffer: bytearray) -> CommandResult:
        return self._check_single_byte_response(buffer)


class Clear(Command):

    def __init__(self):
        super().__init__("Clear", bytearray([0x91]))

    def check_response(self, buffer: bytearray) -> CommandResult:
        return self._check_single_byte_response(buffer)


class Fill(Command):

    def __init__(self):
        super().__init__("Fill", bytearray([0x92]))

    def check_response(self, buffer: bytearray) -> CommandResult:
        return self._check_single_byte_response(buffer)


class SetPixels(Command):

    def __init__(self, pixels: List[Tuple[int, int, bool]]):
        payload = [0x94]
        self._numberOfPixels = len(pixels)

        for pixel in pixels:
            payload.append(0x01 if pixel[2] else 0x00)
            payload.append(max(0, min(255, pixel[0])))
            payload.append(max(0, min(255, pixel[1])))
        payload.append(T.STOP)

        super().__init__("SetPixels", bytearray(payload))

    def check_response(self, buffer: bytearray) -> CommandResult:
        return self._check_multi_byte_response(buffer, self._numberOfPixels)


class Scroll(Command):

    def __init__(self, pixels: List[Optional[bool]], display_height: int):
        payload = [0x95]

        data_bits = bitarray(endian='little')
        mask_bits = bitarray(endian='little')

        if len(pixels) > display_height:
            truncated_pixels = pixels[0:display_height]
        else:
            truncated_pixels = pixels

        for pixel in truncated_pixels:
            if pixel is None:
                mask_bits.append(False)
                data_bits.append(False)
            else:
                mask_bits.append(True)
                data_bits.append(pixel)

        data_bytes = data_bits.tobytes()
        mask_bytes = mask_bits.tobytes()

        expected_bytes = int(ceil(display_height / 8))

        payload += data_bytes
        if len(data_bytes) < expected_bytes:
            payload += [0x00 for _ in range(expected_bytes - len(data_bytes))]
        payload += mask_bytes
        if len(data_bytes) < expected_bytes:
            payload += [0x00 for _ in range(expected_bytes - len(data_bytes))]

        super().__init__('Scroll', bytearray(payload))

    def check_response(self, buffer: bytearray) -> CommandResult:
            return self._check_single_byte_response(buffer)


class SetAllLEDs(Command):

    def __init__(self, color: Tuple[int, int, int]):
        super().__init__("SetAllLEDs", bytearray([0xA0] + list(color) + [T.STOP]))

    def check_response(self, buffer: bytearray) -> CommandResult:
        return self._check_single_byte_response(buffer)


class SetLEDs(Command):

    def __init__(self, leds: List[Tuple[int, int, int, int]]):
        payload = [0xA1]
        self._numberOfCommands = len(leds)

        for led in leds:
            payload.append(0x7F & led[0])
            payload.extend(list(led)[1:])
        payload.append(T.STOP)

        print(payload)

        super().__init__("SetLEDs", bytearray(payload))

    def check_response(self, buffer: bytearray) -> CommandResult:
        return self._check_multi_byte_response(buffer, self._numberOfCommands)


class GetBitset(Command):

    def __init__(self):
        self._bitset = None
        super().__init__("GetBitset", bytearray([0xB0]))

    def check_response(self, buffer: bytearray) -> CommandResult:
        if len(buffer) >= 1 and buffer[0] == R.STATE_UNKNOWN:
            del buffer[0]
            self._bitset = None
            return CommandResult.success()

        n = (112*16 + 7) // 8
        if len(buffer) < n:
            return CommandResult.error(ErrorType.SHORT_RESPONSE)

        raw = buffer[:n]
        del buffer[:n]
        self._bitset = bitarray(endian='little')
        self._bitset.frombytes(bytes(raw))

        return CommandResult.success(result_data=self._bitset)

    @property
    def bitset(self):
        return self._bitset


class GetPixels(Command):

    def __init__(self, pixels: List[Tuple[int, int]]):
        payload = [0xB1]
        self._numberOfPixels = len(pixels)
        self._pixel_state = None

        for (i, px) in enumerate(pixels):
            payload.extend(list(px))
            if i < len(pixels) - 1:
                payload.append(T.CONTINUE)

        payload.append(T.STOP)

        super().__init__("GetPixels", bytearray(payload))

    def check_response(self, buffer: bytearray) -> CommandResult:
        if len(buffer) < self._numberOfPixels:
            return CommandResult.error(ErrorType.SHORT_RESPONSE)

        raw = buffer[:self._numberOfPixels]
        del buffer[:self._numberOfPixels]

        for error_byte in [R.BUFFER_OVERFLOW, R.INVALID_COMMAND]:
            if error_byte in raw:
                return CommandResult.error(self._byte_to_error_type(error_byte))

        for b in raw:
            if b not in (0x00, 0x01, R.ADDR_INVALID, R.STATE_UNKNOWN):
                return CommandResult.error(ErrorType.COMMUNICATION_ERROR)

        self._pixel_state = raw
        return CommandResult.success(result_data=self._pixel_state)

    @property
    def pixel_state(self):
        return self._pixel_state


class GetAllLEDs(Command):

    def __init__(self, expected_number: int):
        self._expected_number = expected_number
        self._colors = None
        super().__init__("GetAllLEDs", bytearray([0xC0]))

    def check_response(self, buffer: bytearray) -> CommandResult:
        if len(buffer) < self._expected_number * 3:
            return CommandResult.short()

        self._colors = []
        for i in range(self._expected_number):
            r = buffer[i * 3 + 0]
            g = buffer[i * 3 + 1]
            b = buffer[i * 3 + 2]
            self._colors.append((r,g,b))
        del buffer[0:self._expected_number*3]
        return CommandResult.success(result_data=self._colors)

    @property
    def colors(self):
        return self._colors


class SetIncrementalMode(Command):

    def __init__(self, mode: DisplayMode):
        super().__init__("SetIncrementalMode", bytearray([0xD0, mode.value]))

    def check_response(self, buffer):
        return self._check_single_byte_response(buffer)


class SetPixelFlipSpeed(Command):

    def __init__(self, speed: int):
        super().__init__("SetPixelFlipSpeed", bytearray([0xD1, speed & 0x00FF, (speed & 0xFF00) >> 8]))

    def check_response(self, buffer):
        return self._check_single_byte_response(buffer)


class SetLEDTransitionMode(Command):

    def __init__(self, mode: LEDTransitionMode):
        super().__init__("SetLEDTransitionMode", bytearray([0xD2, mode.value]))

    def check_response(self, buffer):
        return self._check_single_byte_response(buffer)


class SetPixelTransitionMode(Command):

    def __init__(self, mode: PixelTransitionMode):
        super().__init__("SetPixelTransitionMode", bytearray([0xD3, mode.value]))

    def check_response(self, buffer):
        return self._check_single_byte_response(buffer)


class GetIncrementalMode(Command):

    def __init__(self):
        super().__init__("GetIncrementalMode", bytearray([0xE0]))
        self._mode = None

    @property
    def mode(self) -> DisplayMode:
        return self._mode

    def check_response(self, buffer: bytearray) -> CommandResult:
        if len(buffer) < 1:
            return CommandResult.short()

        value = buffer[0]
        del buffer[0]
        try:
            self._mode = DisplayMode(value)
        except ValueError:
            return CommandResult.error(ErrorType.COMMUNICATION_ERROR)

        print(self._mode)
        return CommandResult.success()


class GetPixelFlipSpeed(Command):

    def __init__(self):
        super().__init__("GetPixelFlipSpeed", bytearray([0xE1]))
        self._speed = None

    @property
    def speed(self) -> int:
        return self._speed

    def check_response(self, buffer: bytearray) -> CommandResult:
        if len(buffer) < 2:
            return CommandResult.short()

        lower = buffer[0]
        higher = buffer[1]
        del buffer[0:2]

        self._speed = ((higher << 8) & 0xFF00) | (lower & 0x00FF)
        return CommandResult.success()


class GetLEDTransitionMode(Command):

    def __init__(self):
        super().__init__("GetLEDTransitionMode", bytearray([0xE2]))
        self._mode = None

    @property
    def mode(self) -> LEDTransitionMode:
        return self._mode

    def check_response(self, buffer):
        if len(buffer) < 1:
            return CommandResult.short()

        value = buffer[0]
        del buffer[0]
        try:
            self._mode = LEDTransitionMode(value)
        except ValueError:
            return CommandResult.error(ErrorType.COMMUNICATION_ERROR)

        return CommandResult.success()


class GetPixelTransitionMode(Command):

    def __init__(self):
        super().__init__("GetPixelTransitionMode", bytearray([0xE3]))
        self._mode = None

    @property
    def mode(self) -> PixelTransitionMode:
        return self._mode

    def check_response(self, buffer):
        if len(buffer) < 1:
            return CommandResult.short()

        value = buffer[0]
        del buffer[0]
        try:
            self._mode = PixelTransitionMode(value)
        except ValueError:
            CommandResult.error(ErrorType.COMMUNICATION_ERROR)

        return CommandResult.success()

