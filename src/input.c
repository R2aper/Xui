#include "input.h"
#include "model.h"

#include <notcurses/notcurses.h>

#define IS_DOWN_KEY(ch) (ch == 'j' || ch == NCKEY_DOWN)
#define IS_UP_KEY(ch) (ch == 'k' || ch == NCKEY_UP)

ACTION handle_input(model_t *state, const ncinput *ni) {
  if (!state || !ni)
    return ERROR;

  // Ignoring key release
  if (ni->evtype == NCTYPE_RELEASE)
    return SKIP;

  if (ni->id == NCKEY_ESC)
    return EXIT;

  if (ni->id == NCKEY_TAB)
    return SWITCH_TAB;

  // Hahdle user input
  if (state->focus == INPUT) {
    if (ni->id == NCKEY_ENTER) {
      state->focus = LIST;
    } else if (ni->id == NCKEY_BACKSPACE && state->input_len > 0) {
      state->input_buffer[--state->input_len] = '\0';
      filter_elements(state);
    } else if (ni->id >= 32 && ni->id <= 126 &&
               state->input_len < sizeof(state->input_buffer) - 1) {
      state->input_buffer[state->input_len++] = (char)ni->id;
      state->input_buffer[state->input_len] = '\0';
      filter_elements(state);
    }

    return SKIP;
  }

  // Navigate the list
  if (state->focus == LIST) {
    unsigned int rows, cols;
    ncplane_dim_yx(state->list_plane, &rows, &cols);

    if (IS_DOWN_KEY(ni->id) &&
        state->selected_idx + 1 < state->filtered_count) {
      state->selected_idx++;
      if (state->selected_idx >= state->visible_start + (size_t)rows) {
        state->visible_start++;
      }
    } else if (IS_UP_KEY(ni->id) && state->selected_idx > 0) {
      state->selected_idx--;
      if (state->selected_idx < state->visible_start) {
        if (state->visible_start > 0)
          state->visible_start--;
      }
    } else if (ni->id == NCKEY_PGUP) { // Page Up
      if (state->selected_idx > 0) {
        state->selected_idx = (state->selected_idx > (size_t)rows)
                                  ? state->selected_idx - rows
                                  : 0;
      }
      if (state->visible_start > state->selected_idx) {
        state->visible_start = state->selected_idx;
      }
    } else if (ni->id == NCKEY_PGDOWN) { // Page Down
      if (state->selected_idx < state->filtered_count - 1) {
        state->selected_idx =
            (state->selected_idx + rows < state->filtered_count)
                ? state->selected_idx + rows
                : state->filtered_count - 1;
      }
      if (state->selected_idx >= state->visible_start + (size_t)rows) {
        state->visible_start = state->selected_idx - rows + 1;
      }
    }
  }

  return SKIP;
}
