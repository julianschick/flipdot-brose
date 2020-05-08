from bitarray import bitarray
from PIL import Image

def convert_image_to_bitarray(img, eval_str):
    print(img.format, img.size, img.mode)

    w = min(img.width, 28*4)
    h = min(img.height, 16)

    imgRgba = img.convert("RGBA")
    imgGrayscale = img.convert("L")
    #imgOne = img.convert("1")

    #pixels = [[None for y in range(0, h)] for x in range(0, w)]
    result = bitarray(h * w, 'little')

    for y in range(0, h):
        for x in range(0, w):
            (r, g, b, a) = imgRgba.getpixel((x, y))
            l = imgGrayscale.getpixel((x, y))

            show_pixel = eval(eval_str)
            #print("x" if show_pixel else " ", end='')
            #pixels[x][y] = show_pixel
            result[x * h + y] = show_pixel

        #print("")


    return result