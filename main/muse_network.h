#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
bool muse_network_init(void);
typedef enum {
    MUSE_SEND_OK,
    MUSE_SEND_DISCONNECTED,
    MUSE_SEND_QUEUE_FULL,
    MUSE_SEND_INVALID,
} muse_send_result_t;
muse_send_result_t muse_network_send_result(const uint8_t *data,size_t n);
bool muse_network_send(const uint8_t *data,size_t n);
size_t muse_network_read(uint8_t *data,size_t n);
bool muse_network_configure(const uint8_t *json,size_t n);
const char *muse_network_token(void);
