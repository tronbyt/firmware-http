#pragma once

#include <stdint.h>

// Retrieves url via HTTP GET. Caller is responsible for freeing buf
// on success.
int remote_get(const char* url, uint8_t** buf, size_t* len, int32_t* brightness, int32_t* dwell_secs);
