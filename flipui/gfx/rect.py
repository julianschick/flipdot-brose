from typing import Tuple
from PIL import Image


class Rect:
    def __init__(self, x: int, y: int, w: int, h: int):
        if x < 0 or y < 0 or w < 0 or h < 0:
            raise RuntimeError("All given values must be non-negative")

        self._x = x
        self._y = y
        self._width = w
        self._height = h

    def __str__(self):
        return f"⇱({self.x1},{self.y1}):⇲({self.x2},{self.y2})"

    @staticmethod
    def from_image_dimensions(img: Image):
        return Rect(0, 0, img.width, img.height)

    @property
    def x(self) -> int:
        return self._x

    @property
    def y(self) -> int:
        return self._y

    @property
    def width(self) -> int:
        return self._width

    @property
    def height(self) -> int:
        return self._height

    @property
    def top_left(self) -> Tuple[int, int]:
        return self.x1, self.y1

    @property
    def bottom_right(self) -> Tuple[int, int]:
        return self.x2, self.y2

    @property
    def x1(self) -> int:
        return self._x

    @property
    def y1(self) -> int:
        return self._y

    @property
    def x2(self) -> int:
        return self._x + self._width

    @property
    def y2(self) -> int:
        return self._y + self._height

    def is_empty(self) -> bool:
        return self._height == 0 and self._width == 0

    def contains_point(self, coord: Tuple[int, int]) -> bool:
        x, y = coord
        return self.x1 <= x < self.x2 and self.y1 <= y < self.y2

    def contains_rect(self, other: 'Rect') -> bool:
        return self.contains_point(other.top_left) and self.contains_point(other.bottom_right)

    def get_inner_rect(self) -> 'Rect':
        return Rect(self._x, self._y, max(0, self._width - 1), max(0, self._height - 1))

    def intersect(self, other: 'Rect') -> 'Rect':
        x1 = max(self.x1, other.x1)
        y1 = max(self.y1, other.y1)
        x2 = min(self.x2, other.x2)
        y2 = min(self.y2, other.y2)

        return Rect(x1, y1, max(0, x2 - x1), max(0, y2 - y1))
