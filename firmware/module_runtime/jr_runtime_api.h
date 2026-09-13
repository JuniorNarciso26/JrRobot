#pragma once

#include <stdbool.h>
#include <stddef.h>

#define JR_RUNTIME_API_MAJOR 1
#define JR_RUNTIME_API_VERSION "1.3"

/*
 * Processa somente funcoes registradas da Runtime API.
 * request_json deve conter um objeto JSON com v, fn e args opcional.
 * A resposta e sempre prefixada por "JR_API " e contem JSON estruturado.
 */
bool jr_runtime_api_handle(const char *request_json, char *response, size_t response_len);
