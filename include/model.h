#pragma once

#include <stddef.h>

#define INPUT_BUFFER_SIZE 256
#define FILTERED_INDICES_SIZE 64

struct notcurses_options;

// TODO:
typedef struct element_t {
  const char *name;
  const char *desc;

} element_t;

extern const element_t elements[];
extern const size_t elements_count;

// -------

/// @brief Enumeration for managing focus between different tabs or components
typedef enum FOCUS_TAB {
  INPUT = 0,
  LIST = 1,

} FOCUS_TAB;

///
typedef struct model_t {
  struct notcurses *nc;        // notcurses context
  struct ncplane *list_plane;  // Plane for displaying the list of items
  struct ncplane *input_plane; // Plane for user input
  struct ncplane *info_plane;  // Informational plane

  size_t selected_idx;  // Index of selected item
  size_t visible_start; // Starting index of the portion of the list that is currently visible

  char input_buffer[INPUT_BUFFER_SIZE];
  size_t input_len;

  size_t filtered_indices[FILTERED_INDICES_SIZE]; // Array storing indices of items that pass a
                                                  // filter criteria
  size_t filtered_count;

  FOCUS_TAB focus; // Current focus

} model_t;

/// @brief Filter elements of list based on user input, updating `filtered_indices` and
/// `filtered_count`
void filter_elements(model_t *state);

/// @brief Initializes a new instance of model_t
/// @param opts Notcurses options
///
/// @return valid model_t on success and (model_t){0} on error
model_t model_t_init(struct notcurses_options opts);

/// @brief Cleans up resources associated with the model_t
void model_t_cleanup(model_t *state);
