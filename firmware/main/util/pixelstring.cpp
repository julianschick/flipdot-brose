#include "pixelstring.h"

#include <vector>
#include "pixelmap.h"
#include "util.h"

#include <esp_log.h>

PixelString::PixelString(string &str) : str(str) {}

void PixelString::print(BitArray &target, PixelMap &pixelmap, PixelFont &regular_font, PixelFont &bold_font,
                        PixelString::TextAlignment text_alignment) {

    PixelCoord coords = {0, 0};
    vector<string> lines = split(str, '\n');
    PixelFont* active_font = &regular_font;

    for (int i = 0; i < lines.size(); i++) {
        ESP_LOGI("display", "line[%d] = %s", i, lines[i].c_str());
    }

    int line_cursor = 0;
    while(coords.y + 7 < pixelmap.get_height() && line_cursor < lines.size()) {
        string& s = lines.at(line_cursor++);

        //dry run
        int x_cursor = 0;
        int line_width = 0;
        int line_char_count = 0;
        bool whitespace = false;

        PixelFont* tmp = active_font;

        for (int i = 0; i < s.size(); i++) {
            char c = s[i];

            if (c == 0x20) {
                whitespace = true;
            } else if (c == 0x11) {
                active_font = &regular_font;
            } else if (c == 0x12) {
                active_font = &bold_font;
            } else if (active_font->has_char(c)) {
                int char_width = active_font->get_width(c);
                int space = (x_cursor == 0 ? 0 : 1) + (whitespace ? 2 : 0);
                whitespace = false;

                if (x_cursor + space + char_width > pixelmap.get_width()) {
                    line_width = x_cursor;
                    line_char_count = i;
                    break;
                } else {
                    x_cursor += space + char_width;
                }
            }

            if (i == s.size() - 1) {
                line_width = x_cursor;
                line_char_count = s.size();
            }
        }

        active_font = tmp;
        switch (text_alignment) {
            case LEFT: coords.x = 0; break;
            case CENTERED: coords.x = ((pixelmap.get_width() - line_width) / 2) - 1; break;
            case RIGHT: coords.x = pixelmap.get_width() - line_width - 1; break;
        }

        ESP_LOGI("display", "line length = %d, offset = %d, chars = %d, content = %s", line_width, coords.x, line_char_count, s.c_str());

        // actual printing
        for (int i = 0; i < line_char_count; i++) {
            char c = s[i];

            if (c == 0x20) {
                coords.x += 2;
            } else if (c == 0x11) {
                active_font = &regular_font;
            } else if (c == 0x12) {
                ESP_LOGI("display", "boldify");
                active_font = &bold_font;
            } else if (active_font->has_char(c)) {
                coords.x += coords.x != 0;

                for (size_t j = 0; j < active_font->get_width(c); j++) {
                    target.set8(pixelmap.index(coords), active_font->get_octet(c, j));
                    coords.x++;
                }
            }
        }

        coords.y += 8;
    }



}