#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "cJSON.h"
#include "esp_err.h"

#include "jr_audio.h"
#include "jr_board.h"
#include "jr_brain.h"
#include "jr_config.h"
#include "jr_face.h"
#include "jr_runtime_api.h"
#include "jr_voice.h"
#include "jr_wifi.h"

#define JR_API_PREFIX "JR_API "
#define JR_API_ID_MAX 32
#define JR_API_FN_MAX 48
#define JR_API_PATH_MAX 64
#define JR_API_EXPRESSION_MAX 32

static bool valid_token(const char *value, size_t max_len) {
    if (!value || !value[0]) return false;
    size_t len = strlen(value);
    if (len > max_len) return false;
    for (size_t i = 0; i < len; ++i) {
        unsigned char c = (unsigned char)value[i];
        bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
        if (!ok) return false;
    }
    return true;
}

static cJSON *new_envelope(bool ok, const char *id) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;
    cJSON_AddNumberToObject(root, "v", JR_RUNTIME_API_MAJOR);
    cJSON_AddBoolToObject(root, "ok", ok);
    if (id && id[0]) cJSON_AddStringToObject(root, "id", id);
    return root;
}

static bool emit_json(cJSON *root, bool ok, char *response, size_t response_len) {
    if (!root || !response || response_len == 0) {
        if (root) cJSON_Delete(root);
        return false;
    }
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) {
        snprintf(response, response_len,
                 JR_API_PREFIX "{\"v\":1,\"ok\":false,\"error\":{\"code\":\"no_memory\",\"message\":\"json_encode_failed\"}}");
        return false;
    }
    size_t required = strlen(JR_API_PREFIX) + strlen(json) + 1;
    if (required > response_len) {
        cJSON_free(json);
        snprintf(response, response_len,
                 JR_API_PREFIX "{\"v\":1,\"ok\":false,\"error\":{\"code\":\"response_too_large\",\"message\":\"increase_response_buffer\"}}");
        return false;
    }
    snprintf(response, response_len, JR_API_PREFIX "%s", json);
    cJSON_free(json);
    return ok;
}

static bool emit_error(const char *id, const char *code, const char *message,
                       char *response, size_t response_len) {
    cJSON *root = new_envelope(false, id);
    if (!root) {
        snprintf(response, response_len,
                 JR_API_PREFIX "{\"v\":1,\"ok\":false,\"error\":{\"code\":\"no_memory\",\"message\":\"response_create_failed\"}}");
        return false;
    }
    cJSON *error = cJSON_CreateObject();
    if (!error) {
        cJSON_Delete(root);
        snprintf(response, response_len,
                 JR_API_PREFIX "{\"v\":1,\"ok\":false,\"error\":{\"code\":\"no_memory\",\"message\":\"response_create_failed\"}}");
        return false;
    }
    cJSON_AddStringToObject(error, "code", code ? code : "error");
    cJSON_AddStringToObject(error, "message", message ? message : "runtime_api_error");
    cJSON_AddItemToObject(root, "error", error);
    return emit_json(root, false, response, response_len);
}

static bool add_string_item(cJSON *array, const char *value) {
    cJSON *item = cJSON_CreateString(value);
    if (!item) return false;
    cJSON_AddItemToArray(array, item);
    return true;
}

static cJSON *capabilities_result(void) {
    cJSON *result = cJSON_CreateObject();
    if (!result) return NULL;

    cJSON_AddStringToObject(result, "api", JR_RUNTIME_API_VERSION);
    cJSON_AddNumberToObject(result, "protocol", JR_RUNTIME_API_MAJOR);
    cJSON_AddStringToObject(result, "firmware", JR_APP_VERSION);
    cJSON_AddStringToObject(result, "hardware", JR_PINMAP_REVISION);
    cJSON_AddStringToObject(result, "profile", JR_PROFILE_NAME);
    cJSON_AddBoolToObject(result, "persistent_config", false);
    cJSON_AddBoolToObject(result, "flow_engine", false);
    cJSON_AddBoolToObject(result, "playground", false);

    cJSON *functions = cJSON_AddArrayToObject(result, "functions");
    if (!functions) goto fail;
    cJSON *fn = cJSON_CreateObject();
    if (!fn) goto fail;
    cJSON_AddStringToObject(fn, "name", "capabilities");
    cJSON_AddStringToObject(fn, "kind", "query");
    cJSON_AddItemToArray(functions, fn);
    fn = cJSON_CreateObject();
    if (!fn) goto fail;
    cJSON_AddStringToObject(fn, "name", "get");
    cJSON_AddStringToObject(fn, "kind", "query");
    cJSON_AddItemToArray(functions, fn);
    fn = cJSON_CreateObject();
    if (!fn) goto fail;
    cJSON_AddStringToObject(fn, "name", "face");
    cJSON_AddStringToObject(fn, "kind", "action");
    cJSON_AddItemToArray(functions, fn);

    static const char *paths[] = {
        "api.version",
        "system.version",
        "system.hardware",
        "system.profile",
        "brain.status",
        "voice.status",
        "audio.volume",
        "face.current",
        "wifi.status",
    };
    cJSON *get_paths = cJSON_AddArrayToObject(result, "get_paths");
    if (!get_paths) goto fail;
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i)
        if (!add_string_item(get_paths, paths[i])) goto fail;

    cJSON *actions = cJSON_AddArrayToObject(result, "actions");
    cJSON *triggers = cJSON_AddArrayToObject(result, "triggers");
    cJSON *events = cJSON_AddArrayToObject(result, "events");
    if (!actions || !triggers || !events || !add_string_item(actions, "face")) goto fail;

    cJSON *transports = cJSON_AddArrayToObject(result, "transports");
    if (!transports || !add_string_item(transports, "serial.command") ||
        !add_string_item(transports, "http.post./cmd")) goto fail;
    return result;

fail:
    cJSON_Delete(result);
    return NULL;
}

static cJSON *get_result(const char *path) {
    cJSON *result = cJSON_CreateObject();
    if (!result) return NULL;
    cJSON_AddStringToObject(result, "path", path);

    if (!strcmp(path, "api.version")) {
        cJSON_AddStringToObject(result, "value", JR_RUNTIME_API_VERSION);
        return result;
    }
    if (!strcmp(path, "system.version")) {
        cJSON_AddStringToObject(result, "value", JR_APP_VERSION);
        cJSON_AddStringToObject(result, "build", JR_BUILD_STAMP_SP);
        return result;
    }
    if (!strcmp(path, "system.hardware")) {
        cJSON_AddStringToObject(result, "value", JR_PINMAP_REVISION);
        return result;
    }
    if (!strcmp(path, "system.profile")) {
        cJSON_AddStringToObject(result, "value", JR_PROFILE_NAME);
        return result;
    }
    if (!strcmp(path, "audio.volume")) {
        cJSON_AddNumberToObject(result, "value", jr_audio_volume());
        return result;
    }
    if (!strcmp(path, "face.current")) {
        cJSON_AddStringToObject(result, "value", jr_face_expression_name());
        return result;
    }
    if (!strcmp(path, "brain.status")) {
        jr_brain_status_t status = {0};
        jr_brain_get_status(&status);
        cJSON *value = cJSON_AddObjectToObject(result, "value");
        if (!value) goto fail;
        cJSON_AddStringToObject(value, "state", jr_brain_state_name(status.state));
        cJSON_AddBoolToObject(value, "enabled", status.enabled);
        cJSON_AddBoolToObject(value, "listening", status.listening);
        cJSON_AddBoolToObject(value, "mic_ready", status.mic_ready);
        cJSON_AddNumberToObject(value, "triggers", status.triggers);
        cJSON_AddNumberToObject(value, "frames", status.frames);
        cJSON_AddNumberToObject(value, "last_level", status.last_level);
        cJSON_AddNumberToObject(value, "last_probability", status.last_probability);
        cJSON_AddStringToObject(value, "engine", status.engine);
        cJSON_AddStringToObject(value, "last_event", status.last_event);
        cJSON_AddStringToObject(value, "last_error", esp_err_to_name(status.last_error));
        return result;
    }
    if (!strcmp(path, "voice.status")) {
        jr_voice_status_t status = {0};
        jr_voice_get_status(&status);
        cJSON *value = cJSON_AddObjectToObject(result, "value");
        if (!value) goto fail;
        cJSON_AddBoolToObject(value, "running", status.running);
        cJSON_AddBoolToObject(value, "mic_ready", status.mic_ready);
        cJSON_AddNumberToObject(value, "frames", status.frames);
        cJSON_AddNumberToObject(value, "detections", status.detections);
        cJSON_AddNumberToObject(value, "last_level", status.last_level);
        cJSON_AddNumberToObject(value, "sample_rate", status.sample_rate);
        cJSON_AddNumberToObject(value, "chunk_samples", status.chunk_samples);
        cJSON_AddStringToObject(value, "engine", status.engine);
        cJSON_AddStringToObject(value, "model", status.model);
        cJSON_AddStringToObject(value, "wakeword", status.wakeword);
        cJSON_AddStringToObject(value, "last_error", esp_err_to_name(status.last_error));
        return result;
    }
    if (!strcmp(path, "wifi.status")) {
        jr_wifi_status_t status = {0};
        jr_wifi_get_status(&status);
        cJSON *value = cJSON_AddObjectToObject(result, "value");
        if (!value) goto fail;
        cJSON_AddBoolToObject(value, "configured", status.configured);
        cJSON_AddBoolToObject(value, "connected", status.connected);
        cJSON_AddBoolToObject(value, "network_ready", status.network_ready);
        cJSON_AddBoolToObject(value, "pending_restart", status.pending_restart);
        cJSON_AddStringToObject(value, "ip", status.ip);
        cJSON_AddStringToObject(value, "last_error", esp_err_to_name(status.last_error));
        return result;
    }

fail:
    cJSON_Delete(result);
    return NULL;
}

bool jr_runtime_api_handle(const char *request_json, char *response, size_t response_len) {
    if (!response || response_len == 0) return false;
    response[0] = '\0';
    if (!request_json || !request_json[0])
        return emit_error(NULL, "invalid_request", "empty_json", response, response_len);

    cJSON *request = cJSON_Parse(request_json);
    if (!request || !cJSON_IsObject(request)) {
        if (request) cJSON_Delete(request);
        return emit_error(NULL, "invalid_json", "expected_json_object", response, response_len);
    }

    char id_buffer[JR_API_ID_MAX + 1] = {0};
    const char *id = NULL;
    const cJSON *id_item = cJSON_GetObjectItemCaseSensitive(request, "id");
    if (id_item) {
        if (!cJSON_IsString(id_item) || !valid_token(id_item->valuestring, JR_API_ID_MAX)) {
            cJSON_Delete(request);
            return emit_error(NULL, "invalid_request", "invalid_id", response, response_len);
        }
        snprintf(id_buffer, sizeof(id_buffer), "%s", id_item->valuestring);
        id = id_buffer;
    }

    const cJSON *version = cJSON_GetObjectItemCaseSensitive(request, "v");
    if (!cJSON_IsNumber(version) || version->valuedouble != JR_RUNTIME_API_MAJOR) {
        cJSON_Delete(request);
        return emit_error(id, "unsupported_version", "expected_v_1", response, response_len);
    }

    const cJSON *fn_item = cJSON_GetObjectItemCaseSensitive(request, "fn");
    if (!cJSON_IsString(fn_item) || !valid_token(fn_item->valuestring, JR_API_FN_MAX)) {
        cJSON_Delete(request);
        return emit_error(id, "invalid_request", "invalid_fn", response, response_len);
    }
    const char *fn = fn_item->valuestring;

    const cJSON *args = cJSON_GetObjectItemCaseSensitive(request, "args");
    if (args && !cJSON_IsObject(args)) {
        cJSON_Delete(request);
        return emit_error(id, "invalid_request", "args_must_be_object", response, response_len);
    }

    cJSON *root = NULL;
    cJSON *result = NULL;

    if (!strcmp(fn, "capabilities")) {
        result = capabilities_result();
        if (!result) {
            cJSON_Delete(request);
            return emit_error(id, "no_memory", "capabilities_create_failed", response, response_len);
        }
    } else if (!strcmp(fn, "get")) {
        if (!args) {
            cJSON_Delete(request);
            return emit_error(id, "invalid_args", "get_requires_args", response, response_len);
        }
        const cJSON *path_item = cJSON_GetObjectItemCaseSensitive(args, "path");
        if (!cJSON_IsString(path_item) || !valid_token(path_item->valuestring, JR_API_PATH_MAX)) {
            cJSON_Delete(request);
            return emit_error(id, "invalid_args", "invalid_get_path", response, response_len);
        }
        result = get_result(path_item->valuestring);
        if (!result) {
            cJSON_Delete(request);
            return emit_error(id, "not_found", "get_path_not_supported", response, response_len);
        }
    } else if (!strcmp(fn, "face")) {
        if (!args) {
            cJSON_Delete(request);
            return emit_error(id, "invalid_args", "face_requires_args", response, response_len);
        }
        const cJSON *expression_item = cJSON_GetObjectItemCaseSensitive(args, "expression");
        if (!cJSON_IsString(expression_item) ||
            !valid_token(expression_item->valuestring, JR_API_EXPRESSION_MAX)) {
            cJSON_Delete(request);
            return emit_error(id, "invalid_args", "invalid_face_expression", response, response_len);
        }
        result = cJSON_CreateObject();
        if (!result) {
            cJSON_Delete(request);
            return emit_error(id, "no_memory", "face_response_create_failed", response, response_len);
        }
        if (!jr_face_set_expression(expression_item->valuestring)) {
            cJSON_Delete(result);
            cJSON_Delete(request);
            return emit_error(id, "invalid_args", "face_expression_not_supported", response, response_len);
        }
        jr_face_increment_command_count();
        if (!cJSON_AddStringToObject(result, "expression", jr_face_expression_name())) {
            cJSON_Delete(result);
            cJSON_Delete(request);
            return emit_error(id, "no_memory", "face_response_create_failed", response, response_len);
        }
    } else {
        cJSON_Delete(request);
        return emit_error(id, "not_found", "function_not_supported", response, response_len);
    }

    root = new_envelope(true, id);
    if (!root) {
        cJSON_Delete(result);
        cJSON_Delete(request);
        return emit_error(id, "no_memory", "response_create_failed", response, response_len);
    }
    cJSON_AddItemToObject(root, "result", result);
    cJSON_Delete(request);
    return emit_json(root, true, response, response_len);
}
