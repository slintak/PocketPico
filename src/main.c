/**
 * Copyright (C) 2022 by Mahyar Koshkouei <mk@deltabeard.com>
 * Copyright (C) 2024 by Vlastimil Slintak <slintak@uart.cz>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

// Peanut-GB emulator settings
#define ENABLE_LCD  1
#define ENABLE_SOUND    1
#define ENABLE_SDCARD   1
#define PEANUT_GB_HIGH_LCD_ACCURACY 1
#define PEANUT_GB_USE_BIOS 0

#define ENABLE_DEBUG 0

/**
 * Reducing VSYNC calculation to lower multiple.
 * When setting a clock IRQ to DMG_CLOCK_FREQ_REDUCED, count to
 * SCREEN_REFRESH_CYCLES_REDUCED to obtain the time required each VSYNC.
 * DMG_CLOCK_FREQ_REDUCED = 2^18, and SCREEN_REFRESH_CYCLES_REDUCED = 4389.
 * Currently unused.
 */
#define VSYNC_REDUCTION_FACTOR 16u
#define SCREEN_REFRESH_CYCLES_REDUCED (SCREEN_REFRESH_CYCLES/VSYNC_REDUCTION_FACTOR)
#define DMG_CLOCK_FREQ_REDUCED (DMG_CLOCK_FREQ/VSYNC_REDUCTION_FACTOR)

/* C Headers */
#include <stdlib.h>
#include <string.h>

/* RP2040 Headers */
#include <hardware/pio.h>
#include <hardware/clocks.h>
#include <hardware/dma.h>
#include <hardware/spi.h>
#include <hardware/sync.h>
#include <hardware/flash.h>
#include <hardware/timer.h>
#include <hardware/vreg.h>
#include <pico/bootrom.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <sys/unistd.h>
#include <hardware/irq.h>

/* Project headers */
#include "debug.h"
#include "hedley.h"
#include "minigb_apu.h"
#include "ili9225_lcd.h"
#include "ili9225_font.h"
#include "sdcard.h"
#include "i2s.h"
#include "gbcolors.h"

/* GPIO Connections. */
#define GPIO_UP     2
#define GPIO_DOWN   3
#define GPIO_LEFT   4
#define GPIO_RIGHT  5
#define GPIO_A      6
#define GPIO_B      7
#define GPIO_SELECT 8
#define GPIO_START  9
#define GPIO_CS     17
#define GPIO_CLK    18
#define GPIO_SDA    19
#define GPIO_RS     20
#define GPIO_RST    21
#define GPIO_LED    22

#if ENABLE_SOUND

typedef enum {
    AUDIO_CMD_IDLE = 0,
    AUDIO_CMD_PLAYBACK,
    AUDIO_CMD_VOLUME_UP,
    AUDIO_CMD_VOLUME_DOWN,
    AUDIO_CMD_INVALID
} audio_commands_e;

/**
 * Global variables for audio task
 * stream contains N=AUDIO_SAMPLES samples
 * each sample is 32 bits
 * 16 bits for the left channel + 16 bits for the right channel in stereo interleaved format)
 * This is intended to be played at AUDIO_SAMPLE_RATE Hz
 */
int16_t *stream;
struct minigb_apu_ctx apu_ctx = {0};

#define audio_read(a)      audio_read(&apu_ctx, (a))
#define audio_write(a, v)  audio_write(&apu_ctx, (a), (v));
#include "peanut_gb.h"
#undef audio_read
#undef audio_write
#else
#include "peanut_gb.h"
#endif

/** Definition of ROM data
 * We're going to erase and reprogram a region 1Mb from the start of the flash
 * Once done, we can access this at XIP_BASE + 1Mb.
 * Game Boy DMG ROM size ranges from 32768 bytes (e.g. Tetris) to 1,048,576 bytes (e.g. Pokemod Red)
 */
#define FLASH_TARGET_OFFSET (1024 * 1024)
const uint8_t *rom = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);
static unsigned char rom_bank0[65536];

static uint8_t ram[32768];
static int lcd_line_busy = 0;
static palette_t palette;   // Colour palette
static uint8_t manual_palette_selected=0;

static struct
{
    unsigned a  : 1;
    unsigned b  : 1;
    unsigned select : 1;
    unsigned start  : 1;
    unsigned right  : 1;
    unsigned left   : 1;
    unsigned up : 1;
    unsigned down   : 1;
} prev_joypad_bits;

/* Pixel data is stored in here. */
static uint16_t pixels_buffer[LCD_WIDTH];

/**
 * Returns a byte from the ROM file at the given address.
 */
uint8_t gb_rom_read(struct gb_s *gb, const uint_fast32_t addr)
{
    (void) gb;
    if(addr < sizeof(rom_bank0))
        return rom_bank0[addr];

    return rom[addr];
}

/**
 * Returns a byte from the cartridge RAM at the given address.
 */
uint8_t gb_cart_ram_read(struct gb_s *gb, const uint_fast32_t addr)
{
    (void) gb;
    return ram[addr];
}

/**
 * Writes a given byte to the cartridge RAM at the given address.
 */
void gb_cart_ram_write(struct gb_s *gb, const uint_fast32_t addr,
               const uint8_t val)
{
    ram[addr] = val;
}

/**
 * Ignore all errors.
 */
void gb_error(struct gb_s *gb, const enum gb_error_e gb_err, const uint16_t addr)
{
#if 1
    const char* gb_err_str[4] = {
            "UNKNOWN",
            "INVALID OPCODE",
            "INVALID READ",
            "INVALID WRITE"
        };
    DBG_INFO("Error %d occurred: %s at %04X\n.\n", gb_err, gb_err_str[gb_err], addr);
//  abort();
#endif
}

#if ENABLE_LCD
void lcd_draw_line(struct gb_s *gb, const uint8_t pixels[LCD_WIDTH],
           const uint_fast8_t line)
{
    #if PEANUT_FULL_GBC_SUPPORT
    if (gb->cgb.cgbMode) {
        for (unsigned int x = 0; x < LCD_WIDTH; x++) {
            pixels_buffer[x] = gb->cgb.fixPalette[pixels[x]] << 1;
        }
    } else {
    #endif
        for (unsigned int x = 0; x < LCD_WIDTH; x++) {
            pixels_buffer[x] = palette[(pixels[x] & LCD_PALETTE_ALL) >> 4][pixels[x] & 3];
        }
    #if PEANUT_FULL_GBC_SUPPORT
    }
    #endif

    ili9225_write_pixels_wait();
    if(line == 0) {
        ili9225_write_pixels_start(30, 16);
    } else if(line == LCD_HEIGHT) {
        ili9225_write_pixels_end();
    } else {
        ili9225_write_pixels_chunk(pixels_buffer, LCD_WIDTH);
    }

}
#endif

#if ENABLE_SDCARD
/**
 * Load a save file from the SD card
 */
void read_cart_ram_file(struct gb_s *gb) {
    char filename[16];
    uint_fast32_t save_size;
    UINT br;

    gb_get_rom_name(gb,filename);
    save_size=gb_get_save_size(gb);
    if(save_size>0) {
        sd_card_t *pSD=sd_get_by_num(0);
        FRESULT fr=f_mount(&pSD->fatfs,pSD->pcName,1);
        if (FR_OK!=fr) {
            DBG_INFO("E f_mount error: %s (%d)\n",FRESULT_str(fr),fr);
            return;
        }

        FIL fil;
        fr=f_open(&fil,filename,FA_READ);
        if (fr==FR_OK) {
            f_read(&fil,ram,f_size(&fil),&br);
        } else {
            DBG_INFO("E f_open(%s) error: %s (%d)\n",filename,FRESULT_str(fr),fr);
        }

        fr=f_close(&fil);
        if(fr!=FR_OK) {
            DBG_INFO("E f_close error: %s (%d)\n", FRESULT_str(fr), fr);
        }
        f_unmount(pSD->pcName);
        DBG_INFO("I read_cart_ram_file(%s) COMPLETE (%lu bytes)\n",filename,save_size);
    } else {
        DBG_INFO("I read_cart_ram_file(%s) SKIPPED\n", filename);
    }

}

/**
 * Write a save file to the SD card
 */
void write_cart_ram_file(struct gb_s *gb) {
    char filename[16];
    uint_fast32_t save_size;
    UINT bw;

    gb_get_rom_name(gb, filename);
    save_size = gb_get_save_size(gb);
    if(save_size > 0) {
        sd_card_t *sd = sd_get_by_num(0);
        FRESULT fr=f_mount(&sd->fatfs, sd->pcName, 1);
        if (FR_OK != fr) {
            DBG_INFO("E f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
            return;
        }

        FIL fil;
        fr = f_open(&fil, filename, FA_CREATE_ALWAYS | FA_WRITE);
        if (fr == FR_OK) {
            f_write(&fil, ram, save_size, &bw);
        } else {
            DBG_INFO("E f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
        }

        fr = f_close(&fil);
        if(fr != FR_OK) {
            DBG_INFO("E f_close error: %s (%d)\n", FRESULT_str(fr), fr);
        }

        f_unmount(sd->pcName);
    }

    DBG_INFO("I write_cart_ram_file(%s) COMPLETE (%lu bytes)\n",filename, bw);
}

/**
 * Read a save file with internal GB enumalor state from the SD card.
 * This state will allow to resume game from the last run.
 */
void read_gb_emulator_state(struct gb_s *gb) {
    char filename[16];
    char filename_state[32];
    UINT br = 0;
    FIL fil;

    sd_card_t *sd = sd_get_by_num(0);
    FRESULT fr = f_mount(&sd->fatfs, sd->pcName, 1);

    gb_get_rom_name(gb, filename);
    sprintf(filename_state, "%s_state.bin", filename);
    fr = f_open(&fil, filename_state, FA_READ);

    if(fr == FR_OK) {
        f_read(&fil, (uint8_t *)gb, sizeof(struct gb_s), &br);
    } else {
        DBG_INFO("W read_gb_emulator_state(%s): SKIPPED (no previous state)\n", filename_state);
        goto finish;
    }

    DBG_INFO("I read_gb_emulator_state(%s) COMPLETED (%lu bytes)\n", filename_state, br);

finish:
    fr = f_close(&fil);
    if(fr != FR_OK) {
        DBG_INFO("W f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    }

    f_unmount(sd->pcName);
}

/**
 * Write a save file with internal GB enumalor state to the SD card.
 * When loaded, this state will allow to resume game from the last run.
 */
void write_gb_emulator_state(struct gb_s *gb) {
    char filename[16];
    char filename_state[32];
    UINT bw;
    FIL fil;

    sd_card_t *sd = sd_get_by_num(0);
    FRESULT fr = f_mount(&sd->fatfs, sd->pcName,1);

    gb_get_rom_name(gb, filename);
    sprintf(filename_state, "%s_state.bin", filename);
    fr = f_open(&fil, filename_state, FA_CREATE_ALWAYS | FA_WRITE);

    if(fr == FR_OK) {
        f_write(&fil, (uint8_t *)gb, sizeof(struct gb_s), &bw);
    } else {
        DBG_INFO("E write_gb_emulator_state(%s) FAILED (%s)\n", filename_state, FRESULT_str(fr));
        goto finish;
    }

    DBG_INFO("I write_gb_emulator_state(%s) COMPLETED (%lu bytes)\n",filename, bw);

finish:
    fr = f_close(&fil);
    if(fr != FR_OK) {
        DBG_INFO("E f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    }
    f_unmount(sd->pcName);
}

/**
 * Load a .gb rom file in flash from the SD card
 */
void load_cart_rom_file(char *filename) {
    UINT br;
    uint8_t buffer[FLASH_SECTOR_SIZE];
    bool mismatch=false;
    sd_card_t *pSD=sd_get_by_num(0);
    FRESULT fr=f_mount(&pSD->fatfs,pSD->pcName,1);
    if (FR_OK!=fr) {
        DBG_INFO("E f_mount error: %s (%d)\n",FRESULT_str(fr),fr);
        return;
    }
    FIL fil;
    fr=f_open(&fil,filename,FA_READ);
    if (fr==FR_OK) {
        uint32_t flash_target_offset=FLASH_TARGET_OFFSET;
        for(;;) {
            f_read(&fil,buffer,sizeof buffer,&br);
            if(br==0) break; /* end of file */

            DBG_INFO("I Erasing target region...\n");
            flash_range_erase(flash_target_offset,FLASH_SECTOR_SIZE);
            DBG_INFO("I Programming target region...\n");
            flash_range_program(flash_target_offset,buffer,FLASH_SECTOR_SIZE);

            /* Read back target region and check programming */
            DBG_INFO("I Done. Reading back target region...\n");
            for(uint32_t i=0;i<FLASH_SECTOR_SIZE;i++) {
                if(rom[flash_target_offset+i]!=buffer[i]) {
                    mismatch=true;
                }
            }

            /* Next sector */
            flash_target_offset+=FLASH_SECTOR_SIZE;
        }
        if(mismatch) {
            DBG_INFO("I Programming successful!\n");
        } else {
            DBG_INFO("E Programming failed!\n");
        }
    } else {
        DBG_INFO("E f_open(%s) error: %s (%d)\n",filename,FRESULT_str(fr),fr);
    }

    fr=f_close(&fil);
    if(fr!=FR_OK) {
        DBG_INFO("E f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    }
    f_unmount(pSD->pcName);

    DBG_INFO("I load_cart_rom_file(%s) COMPLETE (%lu bytes)\n",filename,br);
}

/**
 * Function used by the rom file selector to display one page of .gb rom files
 */
uint16_t rom_file_selector_display_page(char filename[22][256],uint16_t num_page) {
    sd_card_t *pSD=sd_get_by_num(0);
    DIR dj;
    FILINFO fno;
    FRESULT fr;

    fr=f_mount(&pSD->fatfs,pSD->pcName,1);
    if (FR_OK!=fr) {
        DBG_INFO("E f_mount error: %s (%d)\n",FRESULT_str(fr),fr);
        return 0;
    }

    /* clear the filenames array */
    for(uint8_t ifile=0;ifile<22;ifile++) {
        strcpy(filename[ifile],"");
    }

    /* search *.gb files */
    uint16_t num_file=0;
    fr=f_findfirst(&dj, &fno, ".", "?*.gb");

    /* skip the first N pages */
    if(num_page>0) {
        while(num_file<num_page*22 && fr == FR_OK && fno.fname[0]) {
            num_file++;
            fr=f_findnext(&dj, &fno);
        }
    }

    /* store the filenames of this page */
    num_file=0;
    while(num_file<22 && fr == FR_OK && fno.fname[0]) {
        if(fno.fname[0] != '.') {
            /* Skip any file starting with dot. These are hidden files. */
            strcpy(filename[num_file],fno.fname);
            num_file++;
        }

        fr=f_findnext(&dj, &fno);
    }
    f_closedir(&dj);
    f_unmount(pSD->pcName);

    /* display *.gb rom files on screen */
    ili9225_fill(0x0000);
    for(uint8_t ifile=0;ifile<num_file;ifile++) {
        ili9225_text(filename[ifile],0,ifile*8,0xFFFF,0x0000);
    }
    return num_file;
}

/**
 * The ROM selector displays pages of up to 22 rom files
 * allowing the user to select which rom file to start
 * Copy your *.gb rom files to the root directory of the SD card
 */
void rom_file_selector() {
    uint16_t num_page;
    char filename[22][256];
    uint16_t num_file;

    /* display the first page with up to 22 rom files */
    num_file=rom_file_selector_display_page(filename,num_page);

    /* select the first rom */
    uint8_t selected=0;
    ili9225_text(filename[selected],0,selected*8,0xFFFF,0xF800);

    /* get user's input */
    bool up,down,left,right,a,b,select,start;
    while(true) {
        up=gpio_get(GPIO_UP);
        down=gpio_get(GPIO_DOWN);
        left=gpio_get(GPIO_LEFT);
        right=gpio_get(GPIO_RIGHT);
        a=gpio_get(GPIO_A);
        b=gpio_get(GPIO_B);
        select=gpio_get(GPIO_SELECT);
        start=gpio_get(GPIO_START);
        if(!start) {
            /* re-start the last game (no need to reprogram flash) */
            break;
        }
        if(!a | !b) {
            /* copy the rom from the SD card to flash and start the game */
            ili9225_fill(0x0000);
            ili9225_text("Loading game", 55, 80, 0xFFFF, 0x0000);
            load_cart_rom_file(filename[selected]);
            break;
        }
        if(!down) {
            /* select the next rom */
            ili9225_text(filename[selected],0,selected*8,0xFFFF,0x0000);
            selected++;
            if(selected>=num_file) selected=0;
            ili9225_text(filename[selected],0,selected*8,0xFFFF,0xF800);
            sleep_ms(150);
        }
        if(!up) {
            /* select the previous rom */
            ili9225_text(filename[selected],0,selected*8,0xFFFF,0x0000);
            if(selected==0) {
                selected=num_file-1;
            } else {
                selected--;
            }
            ili9225_text(filename[selected],0,selected*8,0xFFFF,0xF800);
            sleep_ms(150);
        }
        if(!right) {
            /* select the next page */
            num_page++;
            num_file=rom_file_selector_display_page(filename,num_page);
            if(num_file==0) {
                /* no files in this page, go to the previous page */
                num_page--;
                num_file=rom_file_selector_display_page(filename,num_page);
            }
            /* select the first file */
            selected=0;
            ili9225_text(filename[selected],0,selected*8,0xFFFF,0xF800);
            sleep_ms(150);
        }
        if((!left) && num_page>0) {
            /* select the previous page */
            num_page--;
            num_file=rom_file_selector_display_page(filename,num_page);
            /* select the first file */
            selected=0;
            ili9225_text(filename[selected],0,selected*8,0xFFFF,0xF800);
            sleep_ms(150);
        }
        tight_loop_contents();
    }
}

#endif

#if ENABLE_SOUND
void core1_audio(void) {
    /* Allocate memory for the stream buffer */
    stream = malloc(AUDIO_SAMPLES_TOTAL * sizeof(int16_t));
    assert(stream != NULL);
    memset(stream, 0, AUDIO_SAMPLES_TOTAL * sizeof(int16_t));

    /* Initialize I2S sound driver (using PIO0) */
    i2s_config_t i2s_config = i2s_get_default_config();
    i2s_config.sample_freq = AUDIO_SAMPLE_RATE;
    i2s_config.dma_trans_count = AUDIO_SAMPLES;
    i2s_volume(&i2s_config, 4);
    i2s_init(&i2s_config);

    /* Initialize audio emulation. */
    audio_init(&apu_ctx);

    DBG_INFO("I Audio ready on core1.\n");

    while(1) {
        audio_commands_e cmd = multicore_fifo_pop_blocking_inline();
        switch(cmd) {
        case AUDIO_CMD_PLAYBACK:
            audio_callback(&apu_ctx, stream);
            i2s_dma_write(&i2s_config, stream);
            break;

        case AUDIO_CMD_VOLUME_UP:
            i2s_increase_volume(&i2s_config);
            break;

        case AUDIO_CMD_VOLUME_DOWN:
            i2s_decrease_volume(&i2s_config);
            break;

        default:
            break;
        }
    }

    HEDLEY_UNREACHABLE();
}
#endif


int main(void)
{
    static struct gb_s gb;
    enum gb_init_error_e ret;

    /* Overclock to 266 MHZ. */
    {
        const unsigned vco = 1596 * 1000 * 1000;
        const unsigned div1 = 6, div2 = 1;

        vreg_set_voltage(VREG_VOLTAGE_1_15);
        sleep_ms(2);
        set_sys_clock_pll(vco, div1, div2);
        sleep_ms(2);
    }

    DBG_INIT();
    DBG_INFO("INIT: ");

    time_init();

    /* Initialise GPIO pins. */
    gpio_set_function(GPIO_UP, GPIO_FUNC_SIO);
    gpio_set_function(GPIO_DOWN, GPIO_FUNC_SIO);
    gpio_set_function(GPIO_LEFT, GPIO_FUNC_SIO);
    gpio_set_function(GPIO_RIGHT, GPIO_FUNC_SIO);
    gpio_set_function(GPIO_A, GPIO_FUNC_SIO);
    gpio_set_function(GPIO_B, GPIO_FUNC_SIO);
    gpio_set_function(GPIO_SELECT, GPIO_FUNC_SIO);
    gpio_set_function(GPIO_START, GPIO_FUNC_SIO);
    gpio_set_function(GPIO_CS, GPIO_FUNC_SIO);
    gpio_set_function(GPIO_CLK, GPIO_FUNC_SPI);
    gpio_set_function(GPIO_SDA, GPIO_FUNC_SPI);
    gpio_set_function(GPIO_RS, GPIO_FUNC_SIO);
    gpio_set_function(GPIO_RST, GPIO_FUNC_SIO);
    gpio_set_function(GPIO_LED, GPIO_FUNC_SIO);

    gpio_set_dir(GPIO_UP, false);
    gpio_set_dir(GPIO_DOWN, false);
    gpio_set_dir(GPIO_LEFT, false);
    gpio_set_dir(GPIO_RIGHT, false);
    gpio_set_dir(GPIO_A, false);
    gpio_set_dir(GPIO_B, false);
    gpio_set_dir(GPIO_SELECT, false);
    gpio_set_dir(GPIO_START, false);
    gpio_set_dir(GPIO_CS, true);
    gpio_set_dir(GPIO_RS, true);
    gpio_set_dir(GPIO_RST, true);
    gpio_set_dir(GPIO_LED, true);
    gpio_set_slew_rate(GPIO_CLK, GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(GPIO_SDA, GPIO_SLEW_RATE_FAST);

    gpio_pull_up(GPIO_UP);
    gpio_pull_up(GPIO_DOWN);
    gpio_pull_up(GPIO_LEFT);
    gpio_pull_up(GPIO_RIGHT);
    gpio_pull_up(GPIO_A);
    gpio_pull_up(GPIO_B);
    gpio_pull_up(GPIO_SELECT);
    gpio_pull_up(GPIO_START);

#if ENABLE_SOUND
    multicore_launch_core1(core1_audio);
#endif

#if ENABLE_LCD
    ili9225_init();
#endif

while(true) {
#if ENABLE_LCD
    ili9225_set_window(0, ILI9225_SCREEN_WIDTH, 0, ILI9225_SCREEN_HEIGHT);
#endif

#if ENABLE_SDCARD
    /* ROM File selector */
    rom_file_selector();
#endif

    /* Initialise GB context. */
    memcpy(rom_bank0, rom, sizeof(rom_bank0));
    ret = gb_init(&gb, &gb_rom_read, &gb_cart_ram_read,
              &gb_cart_ram_write, &gb_error, NULL);
    DBG_INFO("GB ");

    if(ret != GB_INIT_NO_ERROR)
    {
        DBG_INFO("Error: %d\n", ret);
        goto out;
    }

#if ENABLE_SDCARD
    /* Try to load last saved emulator state for this game. */
    read_gb_emulator_state(&gb);
#endif

    /* Automatically assign a colour palette to the game */
    char rom_title[16];
    auto_assign_palette(palette, gb_colour_hash(&gb),gb_get_rom_name(&gb,rom_title));

#if ENABLE_LCD
    gb_init_lcd(&gb, &lcd_draw_line);
    ili9225_fill(0x0000);
    ili9225_set_window(
        (ILI9225_SCREEN_WIDTH - LCD_WIDTH) / 2,
        LCD_WIDTH,
        (ILI9225_SCREEN_HEIGHT - LCD_HEIGHT) / 2,
        LCD_HEIGHT
    );
    DBG_INFO("LCD ");
#endif

#if ENABLE_SDCARD
    /* Load Save File. */
    read_cart_ram_file(&gb);
#endif

    DBG_INFO("\n> ");
    uint_fast32_t frames = 0;
    uint64_t start_time = time_us_64();
    while(1)
    {
        int input;

        /* Execute CPU cycles until the screen has to be redrawn. */
        gb_run_frame(&gb);

        frames++;
#if ENABLE_SOUND
        if(!gb.direct.frame_skip) {
            multicore_fifo_push_blocking_inline(AUDIO_CMD_PLAYBACK);
        }
#endif

        /* Update buttons state */
        prev_joypad_bits.up=gb.direct.joypad_bits.up;
        prev_joypad_bits.down=gb.direct.joypad_bits.down;
        prev_joypad_bits.left=gb.direct.joypad_bits.left;
        prev_joypad_bits.right=gb.direct.joypad_bits.right;
        prev_joypad_bits.a=gb.direct.joypad_bits.a;
        prev_joypad_bits.b=gb.direct.joypad_bits.b;
        prev_joypad_bits.select=gb.direct.joypad_bits.select;
        prev_joypad_bits.start=gb.direct.joypad_bits.start;
        gb.direct.joypad_bits.up=gpio_get(GPIO_UP);
        gb.direct.joypad_bits.down=gpio_get(GPIO_DOWN);
        gb.direct.joypad_bits.left=gpio_get(GPIO_LEFT);
        gb.direct.joypad_bits.right=gpio_get(GPIO_RIGHT);
        gb.direct.joypad_bits.a=gpio_get(GPIO_A);
        gb.direct.joypad_bits.b=gpio_get(GPIO_B);
        gb.direct.joypad_bits.select=gpio_get(GPIO_SELECT);
        gb.direct.joypad_bits.start=gpio_get(GPIO_START);

        /* hotkeys (select + * combo)*/
        if(!gb.direct.joypad_bits.select) {
#if ENABLE_SOUND
            if(!gb.direct.joypad_bits.up && prev_joypad_bits.up) {
                /* select + up: increase sound volume */
                multicore_fifo_push_blocking_inline(AUDIO_CMD_VOLUME_UP);
            }
            if(!gb.direct.joypad_bits.down && prev_joypad_bits.down) {
                /* select + down: decrease sound volume */
                multicore_fifo_push_blocking_inline(AUDIO_CMD_VOLUME_DOWN);
            }
#endif
            if(!gb.direct.joypad_bits.right && prev_joypad_bits.right) {
                /* select + right: select the next manual color palette */
                if(manual_palette_selected<12) {
                    manual_palette_selected++;
                    manual_assign_palette(palette,manual_palette_selected);
                }
            }
            if(!gb.direct.joypad_bits.left && prev_joypad_bits.left) {
                /* select + left: select the previous manual color palette */
                if(manual_palette_selected>0) {
                    manual_palette_selected--;
                    manual_assign_palette(palette,manual_palette_selected);
                }
            }
            if(!gb.direct.joypad_bits.start && prev_joypad_bits.start) {
                /* select + start: save ram and resets to the game selection menu */
#if ENABLE_SDCARD
                write_cart_ram_file(&gb);
                /* Try to save the emulator state for this game. */
                write_gb_emulator_state(&gb);
#endif
                goto out;
            }
            if(!gb.direct.joypad_bits.a && prev_joypad_bits.a) {
                /* select + A: enable/disable frame-skip => fast-forward */
                gb.direct.frame_skip=!gb.direct.frame_skip;
                DBG_INFO("I gb.direct.frame_skip = %d\n",gb.direct.frame_skip);
            }
        }

#if ENABLE_DEBUG
        /* Serial monitor commands */
        input = getchar_timeout_us(0);
        if(input == PICO_ERROR_TIMEOUT)
            continue;

        switch(input)
        {
#if 0
        static bool invert = false;
        static bool sleep = false;
        static uint8_t freq = 1;
        static ili9225_color_mode_e colour = ILI9225_COLOR_MODE_FULL;

        case 'i':
            invert = !invert;
            ili9225_display_control(invert, colour);
            break;

        case 'f':
            freq++;
            freq &= 0x0F;
            ili9225_set_drive_freq(freq);
            DBG_INFO("Freq %u\n", freq);
            break;
#endif
        case 'i':
            gb.direct.interlace = !gb.direct.interlace;
            break;

        case 'f':
            gb.direct.frame_skip = !gb.direct.frame_skip;
            break;

        case 'b':
        {
            uint64_t end_time;
            uint32_t diff;
            uint32_t fps;

            end_time = time_us_64();
            diff = end_time-start_time;
            fps = ((uint64_t)frames*1000*1000)/diff;
            DBG_INFO("Frames: %u\n"
                "Time: %lu us\n"
                "FPS: %lu\n",
                frames, diff, fps);
            stdio_flush();
            frames = 0;
            start_time = time_us_64();
            break;
        }

        case '\n':
        case '\r':
        {
            gb.direct.joypad_bits.start = 0;
            break;
        }

        case '\b':
        {
            gb.direct.joypad_bits.select = 0;
            break;
        }

        case '8':
        {
            gb.direct.joypad_bits.up = 0;
            break;
        }

        case '2':
        {
            gb.direct.joypad_bits.down = 0;
            break;
        }

        case '4':
        {
            gb.direct.joypad_bits.left= 0;
            break;
        }

        case '6':
        {
            gb.direct.joypad_bits.right = 0;
            break;
        }

        case 'z':
        case 'w':
        {
            gb.direct.joypad_bits.a = 0;
            break;
        }

        case 'x':
        {
            gb.direct.joypad_bits.b = 0;
            break;
        }

        case 'q':
            goto out;

        default:
            break;
        }
#endif /* ENABLE_DEBUG */
    }

out:
    DBG_INFO("\nEmulation Ended");

}

}
