add_library(ili9225_lcd)

target_sources(ili9225_lcd
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ili9225_lcd.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ili9225_font.c
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/ili9225_lcd.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ili9225_font.c
)

pico_generate_pio_header(ili9225_lcd ${CMAKE_CURRENT_LIST_DIR}/ili9225_lcd.pio)

target_include_directories(ili9225_lcd PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ili9225_lcd PUBLIC pico_stdlib hardware_pio hardware_dma)
