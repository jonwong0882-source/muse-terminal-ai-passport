idf_component_register(
    SRCS "muse_main.c" "muse_protocol.c" "muse_ui.c" "muse_network.c" "muse_avatar.c"
         "../assets/fonts/muse_font_16.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_driver_usb_serial_jtag esp_timer esp_system esp_wifi esp_event esp_netif esp-tls nvs_flash json lwip
)
target_add_binary_data(${COMPONENT_LIB}
    "${CMAKE_CURRENT_LIST_DIR}/../assets/images/muse-avatar-rgb565.bin"
    BINARY RENAME_TO muse_avatar)
