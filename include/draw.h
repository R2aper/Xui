#pragma once

#include <stdbool.h>

typedef struct model_t model_t;

/// @brief Functions for drawing each element of application
/// @param state Initialized model_t struct
///
/// @return true on success, false on error
bool draw_input(model_t *state);
bool draw_list(model_t *state);
bool draw_info(model_t *state);
bool init_ui(model_t *state);
