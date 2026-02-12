#include "defer.h"
#include "model.h"
#include "tui.h"
#include <assert.h>

#include <notcurses/notcurses.h>

int main(void) {
  struct notcurses_options opts = {
      .flags = NCOPTION_NO_CLEAR_BITMAPS | NCOPTION_PRESERVE_CURSOR,
      .loglevel = NCLOGLEVEL_WARNING,
  };
  model_t state = model_t_init(opts);
  assert(state.nc);

  defer { model_t_cleanup(&state); };

  assert(run_app(&state) != 0);

  return 0;
}
