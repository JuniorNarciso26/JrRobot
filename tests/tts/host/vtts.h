#pragma once
#include <stdint.h>
#include <stddef.h>
typedef struct {int sample_rate;} vtts_model;
int vtts_init(vtts_model *, const void *, size_t);
void vtts_set_expressiveness(vtts_model *, float);
size_t vtts_stream_arena_bytes(const vtts_model *, int, int);
long vtts_synthesize_stream_ex(const vtts_model *, const int32_t *, int, float, int, int, int (*)(const float *, long, void *), void *, void *, size_t);
