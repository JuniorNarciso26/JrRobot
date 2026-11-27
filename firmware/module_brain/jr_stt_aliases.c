#include <ctype.h>
#include <stddef.h>
#include <string.h>
#include "jr_stt_aliases.h"

/* Keep observed substitutions explicit. Never match fragments inside other words.
 * Short aliases such as se/said are experimental and can match unintended speech
 * inside the post-wake capture window; the full raw transcript remains in the log.
 */
static const jr_stt_face_match_t aliases[] = {
    {"happy", "happy"},
    {"haing", "happy"},
    {"habe", "happy"},
    {"change face to happy", "happy"},
    {"change the face to happy", "happy"},
    {"sad", "sad"},
    {"se", "sad"},
    {"said", "sad"},
    {"shady face foresid", "sad"},
    {"shange the face far se", "sad"},
    {"change face to sad", "sad"},
    {"change the face to sad", "sad"},
};

const jr_stt_face_match_t *jr_stt_match_face(const char *text) {
    if (!text) return NULL;
    char normalized[160];
    size_t used=0;
    int space=0;
    for (const unsigned char *p=(const unsigned char *)text; *p; ++p) {
        /* Decoder vocabulary is English ASCII. Reject unsupported bytes rather
         * than silently deleting them and accidentally producing a command. */
        if (*p>=128) return NULL;
        if (isalpha(*p)) {
            if (used+(space?1U:0U)+1>=sizeof(normalized)) return NULL;
            if (space) normalized[used++]=' ';
            normalized[used++]=(char)tolower(*p);
            space=0;
        } else if (isspace(*p) || strchr(".,!?;:",*p)) {
            space=used!=0;
        } else return NULL;
    }
    normalized[used]='\0';
    for (size_t i=0;i<sizeof(aliases)/sizeof(aliases[0]);++i)
        if (!strcmp(normalized,aliases[i].alias)) return &aliases[i];
    return NULL;
}
