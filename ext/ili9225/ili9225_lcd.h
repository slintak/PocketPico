#pragma once


#define ILI9225_SCREEN_WIDTH 220
#define ILI9225_SCREEN_HEIGHT 176


/* ILI9225 Registers. */
/**
 * [Register Name] ([RO,WO, or WR])
 * R ([Read mask]): [Description of what can be read at mask]
 * W ([Write mask]): [Description of what can be written at mask]
 */
/**
 * Driver Code Read (RW)
 * R (0xFFFF): Reads the device code integer 0x9225.
 * W: (0xXXX1): Starts (0x1) or stops (0x0) internal oscillator.
 * At least 10ms must pass after starting the internal oscillator to allow it
 * to stabilise.
 */
#define ILI9225_REG_DRIVER_CODE_READ     0x00
#define ILI9225_REG_START_OSCILLATION    0x00
/**
 * Driver Output Control (WO)
 * W (0x8XXX): VSPL: Inverts the polarity of VSYNC pin. "1" is high active.
 * W (0x4XXX): HSPL: Inverts the polarity of HSYNC pin. "1" is high active.
 * W (0x2XXX): DPL: Same as above for DOTCLK.
 * W (0x1XXX): EPL: Same as above for ENABLE.
 * W (0xX4XX): SM: Scan order by gate driver.
 * W (0xX2XX): GS: Shift direction of gate driver.
 * W (0xX1XX): SS: Shift direction from source driver.
 * W (0xXX1F): NL: Set active gate driver line. Default: 0b11011.
 */
#define ILI9225_REG_DRIVER_OUTPUT_CTRL   0x01
/**
 * LCD Driving Waveform Control (WO)
 * Set LCD inversion method.
 * W (0xX3XX): INV: Sets inversion method.
 * W (0xXXX1): FLD: 3-field interlaced scanning function.
 */
#define ILI9225_REG_LCD_AC_DRIVING_CTRL  0x02
/**
 * Entry Mode (WO)
 * W (0x1XXX): BGR: Swap Red with Blue.
 * W (0xX3XX): MDT: Data transmission format for 80-mode.
 * W (0xXX2X): ID1: Control horizontal address counter increment (1) or
 *      decrement (0).
 * W (0xXX1X): ID0: Control vertical address counter increment (1) or
 *      decrement (0).
 * W (0xXXX8): AM: GRAM update direction. "1" is horizontal.
 */
#define ILI9225_REG_ENTRY_MODE       0x03
/**
 * Display Control 1 (WO)
 * W (0x1XXX): TEMON: Sets the frame flag output signal FLM.
 * W (0xXX1X): GON: Set output level of gate driver. VGL = 0, Normal = 1.
 * W (0xXXX8): CL: Selects 8-colour display mode when 1. Else 2^18 colours.
 * W (0xXXX4): REV: Reverses greyscale levels if set.
 * W (0xXXX3): D: Control display output. 0b11 to turn on display, else display
 *      is off.
 */
#define ILI9225_REG_DISPLAY_CTRL     0x07
/**
 * Display Control 2 (WO)
 * Used for I80, M68, and RGB modes only.
 * W (0xXFXX): FP: Specify number of front porch lines.
 * W (0xXXXF): BP: Specify number of back porch lines.
 */
#define ILI9225_REG_BLANK_PERIOD_CTRL    0x08
/**
 * Frame Cycle Control (WO)
 * Used for RGB mode only.
 * W (0xFXXX): NO: Set amount of non-overlay for gate output.
 * W (0xXFXX): SDT: Set delay from gate edge to source output.
 * W (0xXXXF): RTN: Set number of clock cycles for one display line.
 */
#define ILI9225_REG_FRAME_CYCLE_CTRL     0x0B
/**
 * RGB Input Interface Control (W)
 * Used for RGB mode only.
 */
#define ILI9225_REG_INTERFACE_CTRL       0x0C
/**
 * Oscillator Control (W)
 * W (0xXFXX): FOSC: Oscillation frequency.
 * W (0xXXX1): OSC_EN: Starts oscillation from halt state. 10ms wait required.
 */
#define ILI9225_REG_OSC_CTRL         0x0F
/**
 * Power Control 1 (W)
 * W (0xXFXX): SAP: Set current level of driver.
 * W (0xXXX2): DSTB: Enable deep standby mode. Writing to GRAM is prohibited.
 * W (0xXXX1): STB: Enable sleep mode. Stops display and internal oscillator.
 */
#define ILI9225_REG_PWR_CTRL1        0x10
/**
 * Power Control 2 (W)
 * W (0x1XXX): APON: Use automatic boosting operation.
 * W (0xXFXX): PON: Set boosting operation for specific circuits.
 * W (0xXX2X): AON: Set operation of amplifier.
 * W (0xXX1X): VCL1EN: Set operation of generation amplifier.
 * W (0xXXXF): VC: Set VCl1 voltage output.
 */
#define ILI9225_REG_PWR_CTRL2        0x11
/**
 * Power Control 3 (W)
 * W (0x7XXX): BT: Set output factor of boost converter.
 * W (0xX7XX): DC1: Operation frequency of circuit 1 boost converter.
 * W (0xXX7X): DC2: Operation frequency of circuit 2 boost converter.
 * W (0XXXX7): DC3: Operation frequency of circuit 3 boost converter.
 */
#define ILI9225_REG_PWR_CTRL3        0x12
/**
 * Power Control 4 (W)
 * W (0xXX7F): GVD: Set gamma voltage (GVDD) from 2.66V to 5.5V.
 */
#define ILI9225_REG_PWR_CTRL4        0x13
/**
 * Power Control 5 (W)
 * W (0x8XXX): VCOMG: Sets amplitude of VCOM signal.
 * W (0x7FXX): VCM: Sets VCOMH voltage, from 0.4015 to 1.1000 * GVDD.
 * W (0xXX7F): VML: Sets alternate amplitudes of VCOM from 0.534 to 1.200*GVDD.
 */
#define ILI9225_REG_PWR_CTRL5        0x14
/**
 * VCI Recycling (W)
 * W (0xXX7X): VCIR: Number of clock cycles in VCI recycling period.
 */
#define ILI9225_REG_VCI_RECYCLING        0x15
/**
 * RAM Address Set 1 (W)
 * W (0xXXFF): AD[7:0]: Set the initial value of the address counter.
 */
#define ILI9225_REG_RAM_ADDR_SET1        0x20
/**
 * RAM Address Set 2 (W)
 * W (0xXXFF): AD[15:8]: Set the initial value of the address counter.
 */
#define ILI9225_REG_RAM_ADDR_SET2        0x21
/**
 * Read/Write GRAM Data (RW)
 * W (0x3FFFF): Write data to GRAM.
 * R (0x3FFFF): Read data from GRAM.
 */
#define ILI9225_REG_GRAM_RW          0x22
/**
 * Software Reset (W)
 * W (0xFFFF): Perform software reset when 0x00CE is written.
 */
#define ILI9225_REG_SOFT_RESET       0x28
/**
 * Gate Scan Control (W)
 * W (0xXX1F): SCN: Line to start gate scan from.
 */
#define ILI9225_REG_GATE_SCAN_CTRL       0x30
/**
 * Vertical Scroll Control 1 (W)
 * W (0xXXFF): SEA: Scroll end address.
 */
#define ILI9225_REG_VERT_SCROLL_CTRL1    0x31
/**
 * Vertical Scroll Control 2 (W)
 * W (0xXXFF): SSA: Scroll start address.
 */
#define ILI9225_REG_VERT_SCROLL_CTRL2    0x32
/**
 * Vertical Scroll Control 3 (W)
 * W (0xXXFF): SST: Scroll step.
 */
#define ILI9225_REG_VERT_SCROLL_CTRL3    0x33
/**
 * Partial Screen Driving Position (W)
 * W (0xXXFF): SE: High byte of screen end position.
 */
#define ILI9225_REG_PART_DRIVING_POS1    0x34
/**
 * Partial Screen Driving Position (W)
 * W (0xXXFF): SS: High byte of screen start position.
 */
#define ILI9225_REG_PART_DRIVING_POS2    0x35
/**
 * Horizontal RAM Address Position (W)
 * W (0xXXFF): HEA: End horizontal position.
 */
#define ILI9225_REG_HORI_WIN_ADDR1       0x36
/**
 * Horizontal RAM Address Position (W)
 * W (0xXXFF): HSA: Start horizontal position.
 */
#define ILI9225_REG_HORI_WIN_ADDR2       0x37
/**
 * Vertical RAM Address Position (W)
 * W (0xXXFF): VEA: End vertical position.
 */
#define ILI9225_REG_VERT_WIN_ADDR1       0x38
/**
 * Vertical RAM Address Position (W)
 * W (0xXXFF): VSA: Start vertical position.
 */
#define ILI9225_REG_VERT_WIN_ADDR2       0x39
/**
 * Gamma Control (W)
 */
#define ILI9225_REG_GAMMA_CTRL1      0x50
#define ILI9225_REG_GAMMA_CTRL2      0x51
#define ILI9225_REG_GAMMA_CTRL3      0x52
#define ILI9225_REG_GAMMA_CTRL4      0x53
#define ILI9225_REG_GAMMA_CTRL5      0x54
#define ILI9225_REG_GAMMA_CTRL6      0x55
#define ILI9225_REG_GAMMA_CTRL7      0x56
#define ILI9225_REG_GAMMA_CTRL8      0x57
#define ILI9225_REG_GAMMA_CTRL9      0x58
#define ILI9225_REG_GAMMA_CTRL10     0x59

#define ILI9225_NV_MEM_DATA_PROG     0x60
#define ILI9225_NV_MEM_CTRL          0x61
#define ILI9225_NV_MEM_STAT          0x62
#define ILI9225_NV_MEM_PROTECTION_KEY    0x63
#define ILI9225_NV_MEM_ID_CODE       0x65

#define ILI9225_REG_MTP_TEST_KEY     0x80
#define ILI9225_REG_MTP_CTRL_REG     0x81
#define ILI9225_REG_MTP_DATA_READ        0x82


#define ARRAYSIZE(array)    (sizeof(array)/sizeof(array[0]))

void ili9225_set_rc_cs(bool rc, bool cs);

void ili9225_write_cmd(const uint16_t cmd);

void ili9225_write_data(const uint16_t data);

void ili9225_set_register(uint16_t reg, uint16_t data);

void ili9225_init(void);

void ili9225_set_xy(uint8_t x, uint8_t y);

void ili9225_fill(uint16_t color);

void ili9225_write_pixels_start(uint8_t x, uint8_t y);

void ili9225_write_pixels_chunk(uint16_t *data, uint16_t length);

void ili9225_write_pixels_end(void);

void ili9225_write_pixels_wait(void);

void ili9225_write_pixel(uint16_t color);

void ili9225_set_window(uint16_t x_start, uint16_t x_length, uint16_t y_start, uint16_t y_length);
