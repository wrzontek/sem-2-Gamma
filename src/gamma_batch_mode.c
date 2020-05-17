/** @file
 * Implementacja klasy obsługującej gre w trybie wsadowym.
 *
 * @author Adrian Matwiejuk <am418419@students.mimuw.edu.pl>
 * @copyright Uniwersytet Warszawski
 * @date 17.05.2020
 */
#include "gamma_batch_mode.h"

/** @brief Interpretuje polecenie i zleca jego wykonanie silnikowi.
 * @param[in] *g   – wskaźnik na strukturę przechowującą stan gry,
 * @param[in] *command - wskaźnik na tablicę reprezentującą aktualne polecenie.
 */
static void execute_command(gamma_t *g, int *command) {

    if (command[0] == 'm') {
        printf("%d\n", gamma_move(g, command[1], command[2], command[3]));
    } else if (command[0] == 'g') {
        printf("%d\n", gamma_golden_move(g, command[1], command[2], command[3]));
    } else if (command[0] == 'b') {
        printf("%ld\n", gamma_busy_fields(g, command[1]));
    } else if (command[0] == 'f') {
        printf("%ld\n", gamma_free_fields(g, command[1]));
    } else if (command[0] == 'q') {
        printf("%d\n", gamma_golden_possible(g, command[1]));
    } else {
        char *board_string = gamma_board(g);
        if (board_string != NULL) {
            printf(board_string);
            free(board_string);
        } else {
            printf("0");
        }
    }
}

void batch_read_input(gamma_t *g, long long current_line_count) {
    bool end_of_input = false;
    while (!end_of_input) {
        bool correct_command = true;

        int *command = get_command(g, &correct_command, &end_of_input);
        current_line_count++;

        if (!(correct_command && command == NULL)) { // komentarz lub pusta
            if (!correct_command) {
                fprintf(stderr, "ERROR %lld\n", current_line_count);
            } else {
                execute_command(g, command);
            }
        }

        if (command != NULL) {
            free(command);
        }
    }

}