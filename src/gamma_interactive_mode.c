/** @file
 * Implementacja klasy obsługującej gre interaktywną.
 *
 * @author Adrian Matwiejuk <am418419@students.mimuw.edu.pl>
 * @copyright Uniwersytet Warszawski
 * @date 17.05.2020
 */

#include "gamma_interactive_mode.h"

/** @brief Funkcja wyłączająca wypisywanie wczytywanych znaków do terminala.
 * Funkcja podesłana w komentarzu do części drugiej (dziękuję bardzo za pomocne wyjaśnienia :) )
 */
static struct termios disable_terminal_echo()
{
    struct termios newattr, oldattr;
    tcgetattr(STDIN_FILENO, &oldattr);
    newattr = oldattr;
    newattr.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newattr);

    return oldattr;
}

/** @brief Funkcja włączająca wypisywanie wczytywanych znaków do terminala.
 */
static void enable_terminal_echo(struct termios oldattr)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
}


/** @brief Wypisuje informacje o graczu.
 * @param[in] *g        – wskaźnik na strukturę przechowującą stan gry,
 * @param[in] player    – numer gracza, liczba dodatnia
 */
static void print_player_info(gamma_t *g, uint32_t player) {
    uint64_t busy_fields = gamma_busy_fields(g, player);
    uint64_t free_fields = gamma_free_fields(g, player);

    printf("PLAYER %d BUSY: %ld FREE: %ld", player, busy_fields, free_fields);
    if (gamma_golden_possible(g,player)) {
        printf(" G");
    }
    printf("\n");
}

/** @brief Obsługuje polecenia gracza.
 * Wczytuje znaki z wejścia bez buforowania i odpowiednio porusza kursorem lub wykonuje ruch.
 * @param[in] *g        – wskaźnik na strukturę przechowującą stan gry,
 * @param[in] player    – numer gracza, liczba dodatnia,
 * @param[in, out] x, y - aktualna pozycja kursora, liczby nieujemne
 */
static void interactive_move(gamma_t *g, uint32_t player, uint32_t *x, uint32_t *y, bool *game_shut_down) {
    bool move_made = false;
    while (!move_made) {
        int ch = getchar();
        if (ch == '\033') { // strzałka daje nam trzy znaki : '\033', '[' i jeden z 'A', 'B', 'C', 'D'
            getchar(); // omijamy '['
            ch = getchar();

            if (!((ch == 'A' && g->height - *y == g->height - 1) || (ch == 'B' && g->height - *y == 0)
                  || (ch == 'C' && *x == g->width) || (ch == 'D' && *x - 1 == 0))) {

                printf("\033[1%c", ch); // poruszamy kursorem odpowiednio

                if (ch == 'A') {
                    (*y)--;
                } else if (ch == 'B') {
                    (*y)++;
                } else if (ch == 'C') {
                    (*x)++;
                } else if (ch == 'D') {
                    (*x)--;
                }
            }

        } else if (ch == ' ') {
            if (gamma_move(g, player, *x - 1, g->height - *y)) {
                move_made = true;
            }
        } else if (ch == 'G' || ch == 'g') {
            if (gamma_golden_move(g, player, *x - 1, g->height - *y)) {
                move_made = true;
            }
        } else if (ch == 'C' || ch == 'c') { // rezygancja z ruchu
            move_made = true;
        } else if (ch == EOF) { // wyłączenie gry
            move_made = true;
            *game_shut_down = true;
        }

    }
}

void move_cursor_to_center(uint32_t x, uint32_t y) {
    printf("\033[%d;%dH", y, x);
}

void interactive_play(gamma_t *g) {
    struct termios terminal_settings = disable_terminal_echo();

    uint32_t x = (uint64_t)(g->width + 1) / 2;
    uint32_t y = (uint64_t)(g->height + 1) / 2;

    printf(CLEAR_CONSOLE);
    bool end = false;
    bool game_shut_down = false;
    char *board_string = NULL;

    while (!end && !game_shut_down) {
        end = true;
        for (uint32_t player = 0; player < g->player_count; player++) {
            if (g->players[player].free_fields > 0 || g->players[player].golden_unused) {
                printf(CLEAR_CONSOLE);
                board_string = gamma_board(g);
                if (board_string != NULL) {
                    printf(board_string);
                    free(board_string);
                } else {
                    gamma_delete(g);
                    exit(1); // nie starczyło pamięci na zapisanie stanu gry, nie można kontynuuować
                }
                print_player_info(g, player + 1);

                move_cursor_to_center(x,y);

                end = false; // jeśli któryś z graczy może wykonać ruch to nie kończymy

                interactive_move(g, player + 1, &x, &y, &game_shut_down);
                if (game_shut_down) {
                    break;
                }
            }
        }
    }

    printf(CLEAR_CONSOLE);
    board_string = gamma_board(g);
    printf(board_string);
    free(board_string);
    for (uint32_t player = 0; player < g->player_count; player++) {
        print_player_info(g, player + 1);
    }

    enable_terminal_echo(terminal_settings);
}
