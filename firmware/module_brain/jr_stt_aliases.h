#pragma once
/* Exact normalized utterances; this table does not retrain the speech model. */
typedef struct {
    const char *alias;
    const char *expression;
} jr_stt_face_match_t;
const jr_stt_face_match_t *jr_stt_match_face(const char *text);
