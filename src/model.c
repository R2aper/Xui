#include "model.h"

#include "pkg_search.h"
#include "utils.h"
#include <notcurses/notcurses.h>
#include <stdlib.h>
#include <xbps.h>

model_t model_t_init(struct notcurses_options opts) {
  model_t state = {0};

  state.nc = notcurses_init(&opts, NULL);
  if (!state.nc) {
    fprintf(stderr, "Initialization error: notcurses\n");
    return (model_t){0};
  }
  ncplane_set_scrolling(notcurses_stdplane(state.nc), false);

  if (xbps_init(&state.xhp) != 0) {
    fprintf(stderr, "Initialization error: libxbps\n");
    notcurses_stop(state.nc);
    return (model_t){0};
  }

  state.packages = search_packages(&state.xhp, "", LOCAL, false);
  if (!state.packages) {
    xbps_end(&state.xhp);
    notcurses_stop(state.nc);
    return (model_t){0};
  }

  state.filtered_indices_cap = state.packages->count;
  state.filtered_indices = malloc(state.filtered_indices_cap * sizeof(size_t));
  if (!state.filtered_indices) {
    search_result_cleanup(state.packages);
    xbps_end(&state.xhp);
    notcurses_stop(state.nc);
    return (model_t){0};
  }

  return state;
}

void model_t_cleanup(model_t *state) {
  if (!state)
    return;

  if (state->packages)
    search_result_cleanup(state->packages);

  if (state->filtered_indices)
    free(state->filtered_indices);

  xbps_end(&state->xhp);

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
    state->filtered_count = state->packages->count;
    if (state->filtered_indices_cap < state->packages->count) {
      state->filtered_indices = realloc(
          state->filtered_indices, state->packages->count * sizeof(size_t));
      state->filtered_indices_cap = state->packages->count;
    }
    for (size_t i = 0; i < state->packages->count; i++) {
      state->filtered_indices[i] = i;
    }
    state->selected_idx = 0;
    state->visible_start = 0;
    return;
  }

  // Filtering by substring
  state->filtered_count = 0;
  for (size_t i = 0; i < state->packages->count; i++) {
    package_info_t *pkg = &state->packages->packages[i];
    if ((pkg->pkgver &&
         strcasestr_portable(pkg->pkgver, state->input_buffer)) ||
        (pkg->short_desc &&
         strcasestr_portable(pkg->short_desc, state->input_buffer))) {

      if (state->filtered_count >= state->filtered_indices_cap) {
        state->filtered_indices_cap = state->filtered_indices_cap > 0
                                          ? state->filtered_indices_cap * 2
                                          : 128;
        state->filtered_indices =
            realloc(state->filtered_indices,
                    state->filtered_indices_cap * sizeof(size_t));
      }
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
