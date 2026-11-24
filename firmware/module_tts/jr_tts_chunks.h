#pragma once
#include <stddef.h>

/* Return a prefix ending at an ASCII word boundary. Keep UTF-8 and long words
 * intact; the caller bounds the complete input to JR_TTS_MAX_TEXT_BYTES. */
static inline size_t jr_tts_chunk_bytes(const char *text, size_t target)
{
    if (!text || !text[0] || !target) return 0;
    size_t i = 0, boundary = 0;
    for (; text[i] && i < target; ++i) {
        if (text[i] == ' ' || text[i] == '\n' || text[i] == '\t') boundary = i + 1;
    }
    if (!text[i]) return i;
    if (boundary) return boundary;
    /* A single word exceeds the target: do not cut a UTF-8 code point or word. */
    while (text[i] && text[i] != ' ' && text[i] != '\n' && text[i] != '\t') ++i;
    return i;
}
