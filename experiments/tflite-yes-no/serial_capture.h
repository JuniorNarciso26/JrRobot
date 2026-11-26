#pragma once
#include <cstddef>
#include <cstdint>

void JrCaptureStart();
void JrCaptureFeed(const int16_t* pcm, size_t samples);
bool JrCaptureActive();
