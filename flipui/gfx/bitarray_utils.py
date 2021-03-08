from bitarray import bitarray
from PIL import Image


def img_to_bitarray(img: Image) -> bitarray:
    img_gray = img.convert("L")
    result = bitarray(img_gray.height * img_gray.width, 'little')

    for y in range(0, img_gray.height):
        for x in range(0, img_gray.width):
            show_pixel = img_gray.getpixel((x, y)) < 255
            result[x * img_gray.height + y] = show_pixel

    return result


def bitarray_to_img(bits: bitarray, width: int, height: int, bg_color=(255, 255, 255), fg_color=(0, 0, 0)) -> Image:
    img = Image.new('RGB', (width, height), color=bg_color)

    for (i, bit) in enumerate(bits):
        if bit:
            img.putpixel((i // img.height, i % img.height), fg_color)

    return img
