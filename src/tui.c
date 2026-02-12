#include "tui.h"
#include "draw.h"
#include "input.h"
#include "model.h"

#include <notcurses/notcurses.h>

bool run_app(model_t *state) {
  if (!state)
    return false;

  if (!init_ui(state)) {
    fprintf(stderr, "Error creating interface\n");
    model_t_cleanup(state);
    return false;
  }

  notcurses_render(state->nc);

  ncinput ni = {0};

  // Main loop
  while (notcurses_get(state->nc, NULL, &ni) != (uint32_t)-1) {
    ACTION input = handle_input(state, &ni);
    if (input == EXIT || input == ERROR)
      break;
    else if (input == SWITCH_TAB)
      state->focus = 1 - state->focus;

    draw_list(state);
    draw_input(state);
    draw_info(state);

    notcurses_render(state->nc);
  }

  return true;
}
