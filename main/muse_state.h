#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef enum { S_OFFLINE, S_IDLE, S_RECORDING, S_TRANSCRIBING, S_REVIEW, S_WAITING, S_REPLY, S_ERROR } muse_state_t;
static inline bool muse_accepts_reply(muse_state_t s) { return s==S_WAITING||s==S_REPLY; }
static inline bool muse_can_record(muse_state_t s) { return s==S_IDLE||s==S_REPLY||s==S_ERROR; }
