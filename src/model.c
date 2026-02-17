#include "model.h"

#include "utils.h"
#include <notcurses/notcurses.h>

const element_t elements[] = {
    {"Element1", "First element"},  {"Element2", "Second element"},
    {"Element3", "Third element"},  {"Element11", "Third element"},
    {"Element22", "Third element"}, {"Element33", " element"},
    {"Element44", "1 element"},     {"Element55", "2 element"},
    {"Element66", "2 element"},     {"Element67", "3 element"},

};

const size_t elements_count = sizeof(elements) / sizeof(elements[0]);

model_t model_t_init(struct notcurses_options opts) {
  model_t state = {0};

  state.nc = notcurses_init(&opts, NULL);
  if (!state.nc) {
    fprintf(stderr, "Ошибка инициализации notcurses\n");
    return (model_t){0};
  }

  ncplane_set_scrolling(notcurses_stdplane(state.nc), false);

  return state;
}

void model_t_cleanup(model_t *state) {
  if (!state)
    return;

  if (state->info_plane)
    ncplane_destroy(state->info_plane);
  if (state->input_plane)
    ncplane_destroy(state->input_plane);
  if (state->list_plane)
    ncplane_destroy(state->list_plane);
  if (state->nc)
    notcurses_stop(state->nc);
}

void filter_elements(model_t *state) {
  if (state->input_len == 0) {
    state->filtered_count = elements_count;
    for (size_t i = 0; i < elements_count && i < 64; i++) {
      state->filtered_indices[i] = i;
    }
    state->selected_idx = 0;
    state->visible_start = 0;
    return;
  }

  // Filtering by substring
  state->filtered_count = 0;
  for (size_t i = 0; i < elements_count && state->filtered_count < 64; i++) {
    if (strcasestr_portable(elements[i].name, state->input_buffer)) {
      state->filtered_indices[state->filtered_count++] = i;
    }
  }

  if (state->filtered_count == 0) {
    state->selected_idx = 0;
  } else if (state->selected_idx >= state->filtered_count) {
    state->selected_idx = state->filtered_count - 1;
  }

  state->visible_start = 0;
}
