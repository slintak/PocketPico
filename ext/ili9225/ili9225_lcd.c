#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"

#include "ili9225_lcd.h"
#include "ili9225_lcd.pio.h"

/**
 * TODO: Make these definitions part of configuration.
 */
#define ILI9225_PIN_CS 17
#define ILI9225_PIN_CLK 18
#define ILI9225_PIN_DIN 19
#define ILI9225_PIN_RS 20
#define ILI9225_PIN_RESET 21
#define ILI9225_PIN_BL 22

/**
 * Clock division for LCD PIO.
 * PocketPico MCU is overclocked to 266 MHz.
 * Maximum clock frequency for ILI9225 LCD is 50-60 MHz.
 * PIO state machine is capable of producing clock period every
 * 2 clock cycles -> 266 / 2 = 133 MHz. So we need to slow PIO down
 * by a factor of 2 -> 266 / 2 / 2 = 62.5 MHz.
 *
 * TODO: Calculate this value from CPU and LCD frequencies.
 */
#define SERIAL_CLK_DIV 2.f

static PIO pio = pio1;
static uint sm = 0;
static int dma_chan;


struct reg_dat_pair {
    uint16_t reg;
    uint16_t dat;
};


void ili9225_set_rc_cs(bool rc, bool cs) {
    sleep_us(1);
    gpio_put_masked(
        (1u << ILI9225_PIN_RS) | (1u << ILI9225_PIN_CS),
        !!rc << ILI9225_PIN_RS | !!cs << ILI9225_PIN_CS
    );
    sleep_us(1);
}

void ili9225_write_cmd(const uint16_t cmd) {
    ili9225_lcd_wait_idle(pio, sm);
    ili9225_set_rc_cs(0, 0);
    ili9225_lcd_put(pio, sm, cmd);
    ili9225_lcd_wait_idle(pio, sm);
    ili9225_set_rc_cs(1, 1);
}

void ili9225_write_data(const uint16_t data) {
    ili9225_lcd_wait_idle(pio, sm);
    ili9225_set_rc_cs(1, 0);
    ili9225_lcd_put(pio, sm, data);
    ili9225_lcd_wait_idle(pio, sm);
    ili9225_set_rc_cs(1, 1);
}

void ili9225_set_register(uint16_t reg, uint16_t data) {
    ili9225_write_cmd(reg);
    ili9225_write_data(data);
}

void ili9225_init(void) {
    /* Setup all the GPIO directions for our LCD. */
    gpio_init(ILI9225_PIN_CS);
    gpio_init(ILI9225_PIN_RS);
    gpio_init(ILI9225_PIN_RESET);
    gpio_init(ILI9225_PIN_BL);
    gpio_set_dir(ILI9225_PIN_CS, GPIO_OUT);
    gpio_set_dir(ILI9225_PIN_RS, GPIO_OUT);
    gpio_set_dir(ILI9225_PIN_RESET, GPIO_OUT);
    gpio_set_dir(ILI9225_PIN_BL, GPIO_OUT);
    gpio_put(ILI9225_PIN_BL, 0);

    /* Reset the LCD. */
    gpio_put(ILI9225_PIN_RESET, 1);
    ili9225_set_rc_cs(0, 1);
    sleep_ms(1);
    gpio_put(ILI9225_PIN_RESET, 0);
    sleep_ms(10);
    gpio_put(ILI9225_PIN_RESET, 1);
    sleep_ms(50);

    /* Setup PIO with ILI9225 program. */
    uint offset = pio_add_program(pio, &ili9225_lcd_program);
    ili9225_lcd_program_init(pio, sm, offset, ILI9225_PIN_DIN, ILI9225_PIN_CLK, SERIAL_CLK_DIV);

    /* Setup DMA.
     * We will transfer 16bits data from the buffer given by user to
     * the PIO program. */
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, DREQ_PIO1_TX0);
    dma_channel_configure(
        dma_chan,
        &c,
        &pio1_hw->txf[0], // Write address.
        NULL,             // Don't provide a read address yet.
        0,                // Don't provide a read length yet.
        false             // Don't start yet.
    );

    /* Switch on power control. */
    {
        /* VCI set to 2.58V. */
        /* FIXME: CTRL3 VGH is set to 2.58*6=15.48, but it could be
         * FIXME: lower ( 9 < VGH < 16.5).
         * FIXME: CTRL3 VGL is set to 2.58*-4=-10.32, but it could be
         * FIXME: higher ( -4 < VGL < -16.5).
         * FIXME: BT could be set to 0b000 for VGH=10, VGL=-7.5. */
        /* CTRL4: GVDD is set to 4.68V. */
        /* CTRL5: Set VCM to 0.8030V. Set VML to 1.104V. */
        /* CTRL1: Set driving capability to "Medium Fast 1". */
        const uint8_t regs[] = {
            ILI9225_REG_PWR_CTRL2, ILI9225_REG_PWR_CTRL3,
            ILI9225_REG_PWR_CTRL4, ILI9225_REG_PWR_CTRL5,
            ILI9225_REG_PWR_CTRL1
        };
        const uint16_t dat[] = {
            0x0018, 0x6121, 0x006F, 0x495F, 0x0800
        };

        for(uint8_t i = 0; i < sizeof(regs); i++)
            ili9225_set_register(regs[i], dat[i]);
    }
    sleep_ms(10);

    /* Enable automatic booster operation, and amplifiers.
     * Set VCI1 to 2.76V.
     * FIXME: why is VCI1 changed from 2.58 to 2.76V? */
    ili9225_set_register(ILI9225_REG_PWR_CTRL2, 0x103B);
    sleep_ms(50);

    {
        const struct reg_dat_pair cmds[] = {
            /* GRAM address of 0x0000 should be in top-left corner
             * when looking at the display in the "landscape" mode.
             * Set active lines NL to 528 * 220 dots. */
            { ILI9225_REG_DRIVER_OUTPUT_CTRL,    0x031C },
            /* Set LCD inversion to disabled. */
            { ILI9225_REG_LCD_AC_DRIVING_CTRL,   0x0100 },
            /* Increment both vertical and horizontal address.
             * Use horizontal image. */
            { ILI9225_REG_ENTRY_MODE,        0x1038 },
            /* Turn off all display outputs. */
            { ILI9225_REG_DISPLAY_CTRL,      0x0000 },
            /* Set porches to 8 lines. */
            { ILI9225_REG_BLANK_PERIOD_CTRL,     0x0808 },
            /* Use 1-clock delay to gate output and edge. */
            { ILI9225_REG_FRAME_CYCLE_CTRL,      0x1100 },
            /* Ignore RGB interface settings. */
            { ILI9225_REG_INTERFACE_CTRL,        0x0000 },
            /* Set oscillation frequency to 266.6 kHz. */
            { ILI9225_REG_OSC_CTRL,          0x0701 },
            /* Set VCI recycling to 2 clocks. */
            { ILI9225_REG_VCI_RECYCLING,     0x0020 },
            /* Initialise RAM Address to 0x0 px. */
            { ILI9225_REG_RAM_ADDR_SET1,     0x0000 },
            { ILI9225_REG_RAM_ADDR_SET2,     0x0000 },

            /* Set scanning position to full screen. */
            { ILI9225_REG_GATE_SCAN_CTRL,        0x0000 },
            /* Set end scan position to 219 + 1 px (0xDB). */
            { ILI9225_REG_VERT_SCROLL_CTRL1, 0x00DB },
            /* Set start scan position to 0 px. */
            { ILI9225_REG_VERT_SCROLL_CTRL2, 0x0000 },
            /* Set scroll step to 0 px (no scrolling). */
            { ILI9225_REG_VERT_SCROLL_CTRL3, 0x0000 },
            /* Set partial screen driving end to 219 + 1 px
             * (0xDB). */
            { ILI9225_REG_PART_DRIVING_POS1, 0x00DB },
            /* Set partial screen driving start to 0 px. */
            { ILI9225_REG_PART_DRIVING_POS2, 0x0000 },
            /* Set window to 176 x 220 px (full screen). */
            { ILI9225_REG_HORI_WIN_ADDR1,        0x00AF },
            { ILI9225_REG_HORI_WIN_ADDR2,        0x0000 },
            { ILI9225_REG_VERT_WIN_ADDR1,        0x00DB },
            { ILI9225_REG_VERT_WIN_ADDR2,        0x0000 },

            /* Gamma curve data. */
            { ILI9225_REG_GAMMA_CTRL1,       0x0000 },
            { ILI9225_REG_GAMMA_CTRL2,       0x0808 },
            { ILI9225_REG_GAMMA_CTRL3,       0x080A },
            { ILI9225_REG_GAMMA_CTRL4,       0x000A },
            { ILI9225_REG_GAMMA_CTRL5,       0x0A08 },
            { ILI9225_REG_GAMMA_CTRL6,       0x0808 },
            { ILI9225_REG_GAMMA_CTRL7,       0x0000 },
            { ILI9225_REG_GAMMA_CTRL8,       0x0A00 },
            { ILI9225_REG_GAMMA_CTRL9,       0x0710 },
            { ILI9225_REG_GAMMA_CTRL10,      0x0710 },

            /* Enable full colour display. */
            { ILI9225_REG_DISPLAY_CTRL,      0x0012 }
        };

        for(uint_fast8_t i = 0; i < (uint_fast8_t)ARRAYSIZE(cmds); i++)
            ili9225_set_register(cmds[i].reg, cmds[i].dat);
    }
    sleep_ms(50);

    /**
     * TEMON: disabled, we don't have access to the FLM signal.
     * GON: Enable display.
     * CL: Use full colour.
     * REV: reverse greyscale levels.
     * D: Switch on display.
     */
    ili9225_set_register(ILI9225_REG_DISPLAY_CTRL, 0x0017);
    sleep_ms(50);


    gpio_put(ILI9225_PIN_BL, 0);
}

void ili9225_fill(uint16_t color) {
    ili9225_set_window(0, ILI9225_SCREEN_WIDTH, 0, ILI9225_SCREEN_HEIGHT);
    ili9225_write_pixels_start(0, 0);
    for(uint16_t i = 0; i < (ILI9225_SCREEN_WIDTH * ILI9225_SCREEN_HEIGHT); i++) {
        ili9225_lcd_put(pio, sm, color);
        ili9225_lcd_wait_idle(pio, sm);
    }
    ili9225_set_rc_cs(1, 1);
}

void ili9225_set_xy(uint8_t x, uint8_t y) {
    ili9225_set_register(ILI9225_REG_RAM_ADDR_SET1, y);
    ili9225_set_register(ILI9225_REG_RAM_ADDR_SET2, x);
}

void ili9225_write_pixels_start(uint8_t x, uint8_t y) {
    ili9225_set_xy(x, y);
    ili9225_write_cmd(ILI9225_REG_GRAM_RW);
    ili9225_lcd_wait_idle(pio, sm);
    ili9225_set_rc_cs(1, 0);
}

void ili9225_write_pixels_chunk(uint16_t *data, uint16_t length) {
    dma_channel_wait_for_finish_blocking(dma_chan);
    dma_channel_transfer_from_buffer_now(dma_chan, data, length);
}

void ili9225_write_pixels_end(void) {
    dma_channel_wait_for_finish_blocking(dma_chan);
    ili9225_set_rc_cs(0, 1);
}

void ili9225_write_pixels_wait(void) {
    dma_channel_wait_for_finish_blocking(dma_chan);
}

void ili9225_write_pixel(uint16_t color) {
    ili9225_set_register(ILI9225_REG_GRAM_RW, color);
}

void ili9225_set_window(uint16_t x_start, uint16_t x_length, uint16_t y_start, uint16_t y_length) {
    ili9225_set_register(ILI9225_REG_HORI_WIN_ADDR1, y_start + y_length - 1);
    ili9225_set_register(ILI9225_REG_HORI_WIN_ADDR2, y_start);
    ili9225_set_register(ILI9225_REG_VERT_WIN_ADDR1, x_start + x_length - 1);
    ili9225_set_register(ILI9225_REG_VERT_WIN_ADDR2, x_start);
    ili9225_set_register(ILI9225_REG_RAM_ADDR_SET1, y_start);
    ili9225_set_register(ILI9225_REG_RAM_ADDR_SET2, x_start);
}
