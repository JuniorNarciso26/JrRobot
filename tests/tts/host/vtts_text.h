#pragma once
#include "vtts_lex.h"
#include <stdint.h>
#define VTTS_TEXT_MAX_IDS 320
#define VTTS_LANG_PT 10
typedef struct {int unused;} vtts_text_ws;
typedef struct {int lang; const vtts_lex *lex;} vtts_text_opts;
int vtts_text_to_ids_ex(const char *, vtts_text_ws *, const vtts_text_opts *, int (*)(const int32_t *, int, void *), void *);
