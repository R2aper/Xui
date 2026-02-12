#pragma once

#include <stdbool.h>

typedef struct model_t model_t;

/// @brief Start application
/// @param state Initialized model_t struct
///
/// @return true on success, false on error
bool run_app(model_t *state);
