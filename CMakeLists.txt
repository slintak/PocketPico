cmake_minimum_required(VERSION 3.13...3.23)

set(PICO_SDK_PATH ../ext/pico-sdk)
include(pico_sdk_import.cmake)

project(PocketPico C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(ext/FatFs_SPI build)
add_subdirectory(ext/ili9225)

add_executable(PocketPico
        src/main.c
        ext/minigb_apu/minigb_apu.c
        ext/i2s/i2s.c
)

pico_generate_pio_header(PocketPico ${CMAKE_CURRENT_LIST_DIR}/ext/i2s/audio_i2s.pio)

target_include_directories(PocketPico PRIVATE inc ext/hedley ext/peanut-gb ext/minigb_apu ext/i2s)

target_link_libraries(PocketPico
        FatFs_SPI
        pico_stdlib pico_stdio pico_bootrom pico_multicore pico_multicore
        hardware_clocks hardware_pio hardware_vreg hardware_pio hardware_flash
        hardware_sync hardware_pll hardware_spi hardware_irq hardware_dma
        pico_binary_info ili9225_lcd)
target_compile_definitions(PocketPico PRIVATE
        PARAM_ASSERTIONS_DISABLE_ALL=1
        PICO_ENTER_USB_BOOT_ON_EXIT=1
        PICO_STDIO_ENABLE_CRLF_SUPPORT=0
        PICO_STDIO_DEFAULT_CRLF=0
        PICO_PRINTF_SUPPORT_FLOAT=0
        PICO_PRINTF_SUPPORT_EXPONENTIAL=0
        PICO_PRINTF_SUPPORT_LONG_LONG=1
        PICO_PRINTF_SUPPORT_PTRDIFF_T=0)

function(pico_add_verbose_dis_output TARGET)
	add_custom_command(TARGET ${TARGET} POST_BUILD
		COMMAND ${CMAKE_OBJDUMP} -h $<TARGET_FILE:${TARGET}> >$<IF:$<BOOL:$<TARGET_PROPERTY:${TARGET},OUTPUT_NAME>>,$<TARGET_PROPERTY:${TARGET},OUTPUT_NAME>,$<TARGET_PROPERTY:${TARGET},NAME>>.dis
		COMMAND ${CMAKE_OBJDUMP} -drwCSl $<TARGET_FILE:${TARGET}> >>$<IF:$<BOOL:$<TARGET_PROPERTY:${TARGET},OUTPUT_NAME>>,$<TARGET_PROPERTY:${TARGET},OUTPUT_NAME>,$<TARGET_PROPERTY:${TARGET},NAME>>.dis
		)
endfunction()

pico_set_binary_type(PocketPico copy_to_ram)
pico_enable_stdio_usb(PocketPico 1)
pico_enable_stdio_uart(PocketPico 0)
pico_add_verbose_dis_output(PocketPico)
pico_add_bin_output(PocketPico)
pico_add_uf2_output(PocketPico)
