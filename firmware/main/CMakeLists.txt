set(
    COMPONENT_SRCS
        "main.cpp"
        "flipdotdisplay.cpp"
        "lowlevel/flipdotdriver.cpp"
        "lowlevel/ws2812driver.cpp"
        "libs/wifi.cpp"
        "libs/nvs.cpp"
        "libs/http_server.cpp"
        "libs/tcp_server.cpp"
        "libs/comx.cpp"
        "libs/blue.cpp"
        "util/bitarray.cpp"
        "util/pixelmap.cpp"
        "util/pixelstring.cpp"
)
set(COMPONENT_ADD_INCLUDEDIRS "")

register_component()