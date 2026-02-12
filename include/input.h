#pragma once

typedef struct model_t model_t;
typedef struct ncinput ncinput;

/// @brief App's response on user input
typedef enum ACTION {
  EXIT = 0,
  SWITCH_TAB = 1,
  SKIP = 2,
  ERROR = 3,

} ACTION;

/// @brief Decide App's action based on user input
/// @param state Initialized model_t struct
///
/// @return ACTION enum
ACTION handle_input(model_t *state, const ncinput *ni);
