#include "input.h"
#include "gamma_batch_mode.h"
#include "gamma_interactive_mode.h"

int main() {
    gamma_t *g = NULL;
    long long current_line_count = 0;
    int game_type = determine_game_type(&g, &current_line_count);
    if (game_type == BATCH) {
        batch_read_input(g, current_line_count);
    } else if (game_type == INTERACTIVE) {
        interactive_play(g);
    }
    gamma_delete(g);
    return 0;
}