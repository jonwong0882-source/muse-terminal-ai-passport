#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define MUSE_MAX_PAYLOAD 2048
#define MUSE_HEADER 16
#define MUSE_MAX_FRAME (MUSE_HEADER + MUSE_MAX_PAYLOAD)
enum { M_HELLO=1, M_PING, M_START, M_AUDIO, M_END, M_TRANSCRIPT,
       M_CONFIRM, M_CANCEL, M_STATUS, M_REPLY, M_PLAY, M_ACK, M_DONE, M_ERROR };
typedef struct { uint8_t type; uint32_t session; uint16_t length; uint8_t data[MUSE_MAX_PAYLOAD]; } muse_frame_t;
typedef struct { uint8_t bytes[MUSE_MAX_FRAME]; size_t used; } muse_parser_t;
uint32_t muse_crc32(const uint8_t *data, size_t n, uint32_t crc);
size_t muse_encode(uint8_t *out, uint8_t type, uint32_t session, const void *data, size_t n);
bool muse_feed(muse_parser_t *p, uint8_t byte, muse_frame_t *out);
size_t muse_utf8_copy(char *out, size_t capacity, const uint8_t *src, size_t n);
