#include <string.h>

#include "hardware/dma.h"

#include "ili9225_lcd.h"
#include "ili9225_font.h"

void ili9225_blit(uint16_t *fbuf, uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    ili9225_set_window(x, w, y, h);
    ili9225_write_cmd(ILI9225_REG_GRAM_RW);
    ili9225_set_rc_cs(1, 0);

    for(uint8_t i = 0; i < (w * h); i++) {
        ili9225_write_pixel(fbuf[i]);
    }

    ili9225_set_rc_cs(1, 1);
}

void ili9225_get_letter(uint16_t *fbuf, char l,uint16_t color,uint16_t bgcolor) {
    uint8_t letter[8];
    uint8_t row;

    switch(l)
    {
        case 'a':
        case 'A':
        {
            const uint8_t letter_[8]={0b00111100,
                                      0b01100110,
                                      0b01100110,
                                      0b01111110,
                                      0b01100110,
                                      0b01100110,
                                      0b01100110,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case 'b':
        case 'B':
        {
            const uint8_t letter_[8]={0b01111100,
                                      0b01100110,
                                      0b01100110,
                                      0b01111100,
                                      0b01100110,
                                      0b01100110,
                                      0b01111100,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case 'c':
        case 'C':
        {
            const uint8_t letter_[8]={0b00011110,
                                      0b00110000,
                                      0b01100000,
                                      0b01100000,
                                      0b01100000,
                                      0b00110000,
                                      0b00011110,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case 'd':
        case 'D':
        {
            const uint8_t letter_[8]={0b01111000,
                                      0b01101100,
                                      0b01100110,
                                      0b01100110,
                                      0b01100110,
                                      0b01101100,
                                      0b01111000,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case 'e':
        case 'E':
        {
            const uint8_t letter_[8]={0b01111110,
                                      0b01100000,
                                      0b01100000,
                                      0b01111000,
                                      0b01100000,
                                      0b01100000,
                                      0b01111110,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case 'f':
        case 'F':
        {
            const uint8_t letter_[8]={0b01111110,
                                      0b01100000,
                                      0b01100000,
                                      0b01111000,
                                      0b01100000,
                                      0b01100000,
                                      0b01100000,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case 'g':
        case 'G':
        {
            const uint8_t letter_[8]={0b00111100,
                                      0b01100110,
                                      0b01100000,
                                      0b01101110,
                                      0b01100110,
                                      0b01100110,
                                      0b00111110,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case 'h':
        case 'H':
        {
            const uint8_t letter_[8]={0b01100110,
                                      0b01100110,
                                      0b01100110,
                                      0b01111110,
                                      0b01100110,
                                      0b01100110,
                                      0b01100110,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case 'i':
        case 'I':
        {
            const uint8_t letter_[8]={0b00111100,
                                      0b00011000,
                                      0b00011000,
                                      0b00011000,
                                      0b00011000,
                                      0b00011000,
                                      0b00111100,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case 'j':
        case 'J':
        {
            const uint8_t letter_[8]={0b00000110,
                                      0b00000110,
                                      0b00000110,
                                      0b00000110,
                                      0b00000110,
                                      0b01100110,
                                      0b00111100,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case 'k':
        case 'K':
        {
            const uint8_t letter_[8]={0b11000110,
                                      0b11001100,
                                      0b11011000,
                                      0b11110000,
                                      0b11011000,
                                      0b11001100,
                                      0b11000110,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case 'l':
        case 'L':
        {
            const uint8_t letter_[8]={0b01100000,
                                      0b01100000,
                                      0b01100000,
                                      0b01100000,
                                      0b01100000,
                                      0b01100000,
                                      0b01111110,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case 'm':
        case 'M':
        {
            const uint8_t letter_[8]={0b11000110,
                                      0b11101110,
                                      0b11111110,
                                      0b11010110,
                                      0b11000110,
                                      0b11000110,
                                      0b11000110,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case 'n':
        case 'N':
        {
            const uint8_t letter_[8]={0b11000110,
                                      0b11100110,
                                      0b11110110,
                                      0b11011110,
                                      0b11001110,
                                      0b11000110,
                                      0b11000110,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case 'o':
        case 'O':
        {
            const uint8_t letter_[8]={0b00111100,
                                      0b01100110,
                                      0b01100110,
                                      0b01100110,
                                      0b01100110,
                                      0b01100110,
                                      0b00111100,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case 'p':
        case 'P':
        {
            const uint8_t letter_[8]={0b01111100,
                                      0b01100110,
                                      0b01100110,
                                      0b01111100,
                                      0b01100000,
                                      0b01100000,
                                      0b01100000,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case 'q':
        case 'Q':
        {
            const uint8_t letter_[8]={0b01111000,
                                      0b11001100,
                                      0b11001100,
                                      0b11001100,
                                      0b11001100,
                                      0b11011100,
                                      0b01111110,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case 'r':
        case 'R':
        {
            const uint8_t letter_[8]={0b01111100,
                                      0b01100110,
                                      0b01100110,
                                      0b01111100,
                                      0b01101100,
                                      0b01100110,
                                      0b01100110,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case 's':
        case 'S':
        {
            const uint8_t letter_[8]={0b00111100,
                                      0b01100110,
                                      0b01110000,
                                      0b00111100,
                                      0b00001110,
                                      0b01100110,
                                      0b00111100,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case 't':
        case 'T':
        {
            const uint8_t letter_[8]={0b01111110,
                                      0b00011000,
                                      0b00011000,
                                      0b00011000,
                                      0b00011000,
                                      0b00011000,
                                      0b00011000,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case 'u':
        case 'U':
        {
            const uint8_t letter_[8]={0b01100110,
                                      0b01100110,
                                      0b01100110,
                                      0b01100110,
                                      0b01100110,
                                      0b01100110,
                                      0b00111100,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case 'v':
        case 'V':
        {
            const uint8_t letter_[8]={0b01100110,
                                      0b01100110,
                                      0b01100110,
                                      0b01100110,
                                      0b00111100,
                                      0b00111100,
                                      0b00011000,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case 'w':
        case 'W':
        {
            const uint8_t letter_[8]={0b11000110,
                                      0b11000110,
                                      0b11000110,
                                      0b11010110,
                                      0b11111110,
                                      0b11101110,
                                      0b11000110,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case 'x':
        case 'X':
        {
            const uint8_t letter_[8]={0b11000011,
                                      0b01100110,
                                      0b00111100,
                                      0b00011000,
                                      0b00111100,
                                      0b01100110,
                                      0b11000011,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case 'y':
        case 'Y':
        {
            const uint8_t letter_[8]={0b11000011,
                                      0b01100110,
                                      0b00111100,
                                      0b00011000,
                                      0b00011000,
                                      0b00011000,
                                      0b00011000,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case 'z':
        case 'Z':
        {
            const uint8_t letter_[8]={0b11111110,
                                      0b00001100,
                                      0b00011000,
                                      0b00110000,
                                      0b01100000,
                                      0b11000000,
                                      0b11111110,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case '-':
        {
            const uint8_t letter_[8]={0b00000000,
                                      0b00000000,
                                      0b00000000,
                                      0b01111110,
                                      0b00000000,
                                      0b00000000,
                                      0b00000000,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case '(':
        case '[':
        case '{':
        {
            const uint8_t letter_[8]={0b00001100,
                                      0b00011000,
                                      0b00110000,
                                      0b00110000,
                                      0b00110000,
                                      0b00011000,
                                      0b00001100,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case ')':
        case ']':
        case '}':
        {
            const uint8_t letter_[8]={0b00110000,
                                      0b00011000,
                                      0b00001100,
                                      0b00001100,
                                      0b00001100,
                                      0b00011000,
                                      0b00110000,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case ',':
        {
            const uint8_t letter_[8]={0b00000000,
                                      0b00000000,
                                      0b00000000,
                                      0b00000000,
                                      0b00000000,
                                      0b00011000,
                                      0b00011000,
                                      0b00110000};
            memcpy(letter,letter_,8);
            break;
        }

        case '.':
        {
            const uint8_t letter_[8]={0b00000000,
                                      0b00000000,
                                      0b00000000,
                                      0b00000000,
                                      0b00000000,
                                      0b00011000,
                                      0b00011000,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case '!':
        {
            const uint8_t letter_[8]={0b00011000,
                                      0b00011000,
                                      0b00011000,
                                      0b00011000,
                                      0b00011000,
                                      0b00000000,
                                      0b00011000,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case '&':
        {
            const uint8_t letter_[8]={0b00111000,
                                      0b01101100,
                                      0b01101000,
                                      0b01110110,
                                      0b11011100,
                                      0b11001110,
                                      0b01111011,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case '\'':
        {
            const uint8_t letter_[8]={0b00011000,
                                      0b00011000,
                                      0b00110000,
                                      0b00000000,
                                      0b00000000,
                                      0b00000000,
                                      0b00000000,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case '0':
        {
            const uint8_t letter_[8]={0b00111100,
                                      0b01100110,
                                      0b01101110,
                                      0b01111110,
                                      0b01110110,
                                      0b01100110,
                                      0b00111100,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case '1':
        {
            const uint8_t letter_[8]={0b00011000,
                                      0b00111000,
                                      0b01111000,
                                      0b00011000,
                                      0b00011000,
                                      0b00011000,
                                      0b00011000,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case '2':
        {
            const uint8_t letter_[8]={0b00111100,
                                      0b01100110,
                                      0b00000110,
                                      0b00001100,
                                      0b00011000,
                                      0b00110000,
                                      0b01111110,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case '3':
        {
            const uint8_t letter_[8]={0b00111100,
                                      0b01100110,
                                      0b00000110,
                                      0b00011100,
                                      0b00000110,
                                      0b01100110,
                                      0b00111100,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case '4':
        {
            const uint8_t letter_[8]={0b00011100,
                                      0b00111100,
                                      0b01101100,
                                      0b11001100,
                                      0b11111110,
                                      0b00001100,
                                      0b00001100,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case '5':
        {
            const uint8_t letter_[8]={0b01111110,
                                      0b01100000,
                                      0b01111100,
                                      0b00000110,
                                      0b00000110,
                                      0b01100110,
                                      0b00111100,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case '6':
        {
            const uint8_t letter_[8]={0b00011100,
                                      0b00110000,
                                      0b01100000,
                                      0b01111100,
                                      0b01100110,
                                      0b01100110,
                                      0b00111100,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case '7':
        {
            const uint8_t letter_[8]={0b01111110,
                                      0b00000110,
                                      0b00000110,
                                      0b00001100,
                                      0b00011000,
                                      0b00011000,
                                      0b00011000,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case '8':
        {
            const uint8_t letter_[8]={0b00111100,
                                      0b01100110,
                                      0b01100110,
                                      0b00111100,
                                      0b01100110,
                                      0b01100110,
                                      0b00111100,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        case '9':
        {
            const uint8_t letter_[8]={0b00111100,
                                      0b01100110,
                                      0b01100110,
                                      0b00111110,
                                      0b00000110,
                                      0b00001100,
                                      0b00111000,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }

        default:
        {
            const uint8_t letter_[8]={0b00000000,
                                      0b00000000,
                                      0b00000000,
                                      0b00000000,
                                      0b00000000,
                                      0b00000000,
                                      0b00000000,
                                      0b00000000};
            memcpy(letter,letter_,8);
            break;
        }
    }

    for(uint8_t y = 0; y < 8; y++) {
        row = letter[y];
        for(uint8_t x = 0; x < 8; x++) {
            if(row & 0x80) {
                fbuf[y * 8 + x] = color;
            } else {
                fbuf[y * 8 + x] = bgcolor;
            }
            row = row << 1;
        }
    }
}

void ili9225_text(char *s, uint8_t x, uint8_t y, uint16_t color, uint16_t bgcolor) {
    uint16_t fbuf[8 * 8];
    for(uint8_t i = 0; i < strlen(s); i++) {
        ili9225_get_letter(fbuf, s[i], color, bgcolor);
        ili9225_blit(fbuf, x, y, 8, 8);
        x += 8;
        if(x > 27 * 8) {
            break;
        }
    }
}
