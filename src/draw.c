#include "draw.h"
#include "colors.h"
#include "model.h"

#include <notcurses/notcurses.h>

bool draw_input(model_t *state) {
  if (!state)
    return false;

  ncplane_erase(state->input_plane); // Clear input plane

  // Highlight focus
  if (state->focus == LIST)
    ncplane_set_bg_rgb(state->input_plane, DARK_BLUE);
  else
    ncplane_set_bg_default(state->input_plane);

  // Print prefix(>) and user input
  ncplane_putstr_yx(state->input_plane, 0, 0, "> ");
  ncplane_putstr_yx(state->input_plane, 0, 2, state->input_buffer);

  // Cursor
  if (state->focus == INPUT)
    ncplane_cursor_move_yx(state->input_plane, 0, 2 + (int)state->input_len);

  return true;
}

bool draw_info(model_t *state) {
  if (!state)
    return false;

  ncplane_erase(state->info_plane); // Clear info plate

  ncplane_set_fg_rgb(state->info_plane, MOUNTAIN_MEADOW);
  ncplane_putstr_yx(state->info_plane, 0, 1, "info");
  ncplane_set_fg_default(state->info_plane);

  // Print info
  if (state->filtered_count > 0 &&
      state->selected_idx < state->filtered_count) {
    const package_info_t *pkg =
        &state->packages
             ->packages[state->filtered_indices[state->selected_idx]];

    int y = 1;
    ncplane_printf_yx(state->info_plane, y++, 1, "Pkg: %s",
                      pkg->pkgver ? pkg->pkgver : "N/A");
    ncplane_printf_yx(state->info_plane, y++, 1, "Desc: %s",
                      pkg->short_desc ? pkg->short_desc : "N/A");
    ncplane_printf_yx(state->info_plane, y++, 1, "Homepage: %s",
                      pkg->homepage ? pkg->homepage : "N/A");
    ncplane_printf_yx(state->info_plane, y++, 1, "License: %s",
                      pkg->license ? pkg->license : "N/A");
    ncplane_printf_yx(state->info_plane, y++, 1, "Maintainer: %s",
                      pkg->maintainer ? pkg->maintainer : "N/A");
  } else {
    ncplane_set_fg_rgb(state->info_plane, RED);
    ncplane_putstr_yx(state->info_plane, 1, 1, "No Match");
    ncplane_set_fg_default(state->info_plane);
  }

  return true;
}

bool draw_list(model_t *state) {
  if (!state)
    return false;

  uint32_t rows, cols;
  ncplane_dim_yx(state->list_plane, &rows, &cols);
  ncplane_erase(state->list_plane);

  // Limit visible elements
  size_t max_visible = (size_t)rows;
  size_t start = state->visible_start;
  size_t end = (start + max_visible > state->filtered_count)
                   ? state->filtered_count
                   : start + max_visible;

  for (size_t i = start; i < end; i++) {
    int y = (int)(i - start);
    const package_info_t *pkg =
        &state->packages->packages[state->filtered_indices[i]];

    // Highlight selected element
    if (i == state->selected_idx && state->focus == LIST) {
      ncplane_set_fg_rgb(state->list_plane, WHITE); // Text
      ncplane_set_bg_rgb(state->list_plane, BLUE);  // Bg
    } else {
      ncplane_set_fg_default(state->list_plane);
      ncplane_set_bg_default(state->list_plane);
    }

    ncplane_putstr_yx(state->list_plane, y, 1, pkg->pkgver);
  }

  // Scroll
  if (state->filtered_count > max_visible) {
    ncplane_set_fg_rgb(state->list_plane, GREY);
    if (state->visible_start > 0)
      ncplane_putchar_yx(state->list_plane, 0, cols - 1, 'u');

    if (state->visible_start + max_visible < state->filtered_count)
      ncplane_putchar_yx(state->list_plane, rows - 1, cols - 1, 'd');

    ncplane_set_fg_default(state->list_plane);
  }

  return true;
}

bool init_ui(model_t *state) {
  if (!state)
    return false;

  // Get term sizes
  uint32_t term_rows, term_cols;
  ncplane_dim_yx(notcurses_stdplane(state->nc), &term_rows, &term_cols);

  // Calculate panels's size
  int list_rows = term_rows / 2;
  int input_rows = 1;
  int info_rows = term_rows - list_rows - input_rows - 2; // -2 for separator

  // Create planes
  state->list_plane =
      ncplane_create(notcurses_stdplane(state->nc),
                     &(struct ncplane_options){.y = 0,
                                               .x = 0,
                                               .rows = list_rows,
                                               .cols = term_cols,
                                               .name = "list",
                                               .flags = 0});
  state->input_plane =
      ncplane_create(notcurses_stdplane(state->nc),
                     &(struct ncplane_options){.y = list_rows,
                                               .x = 0,
                                               .rows = input_rows,
                                               .cols = term_cols,
                                               .name = "input",
                                               .flags = 0});
  state->info_plane =
      ncplane_create(notcurses_stdplane(state->nc),
                     &(struct ncplane_options){.y = list_rows + input_rows + 1,
                                               .x = 0,
                                               .rows = info_rows,
                                               .cols = term_cols,
                                               .name = "info",
                                               .flags = 0});

  if (!state->list_plane || !state->input_plane || !state->info_plane)
    return false;

  state->selected_idx = 0;
  state->visible_start = 0;
  state->input_buffer[0] = '\0';
  state->input_len = 0;
  state->focus = LIST;

  filter_elements(state);

  return true;
}
