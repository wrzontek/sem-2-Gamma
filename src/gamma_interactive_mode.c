/** @file
 * Implementacja klasy obsługującej gre interaktywną.
 *
 * @author Adrian Matwiejuk <am418419@students.mimuw.edu.pl>
 * @copyright Uniwersytet Warszawski
 * @date 17.05.2020
 */

#include "gamma_interactive_mode.h"

/** @brief Funkcja zapożyczona ze stackoverflow, wczytuje pojedynczy znak z wejścia bez buforowania.
    Czarna magia jest to dla mnie, funkcję znalazłem tu:
    https://stackoverflow.com/questions/558009/ansi-c-no-echo-keyboard-input?fbclid=IwAR2otMnZJHtIIrgbRAwkhZfJkAFoTAYb7QgWVJameduGxyRIF37FQTt-eHQ
 */
static int getche(void) {
    struct termios oldattr, newattr;
    int ch;
    tcgetattr(STDIN_FILENO, &oldattr);
    newattr = oldattr;
    newattr.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
    system("stty -echo");
    ch = getchar();
    system("stty echo");
    tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
    return ch;
}

/** @brief Wypisuje informacje o graczu.
 * @param[in] *g        – wskaźnik na strukturę przechowującą stan gry,
 * @param[in] player    – numer gracza, liczba dodatnia
 */
static void print_player_info(gamma_t *g, uint32_t player) {
    uint64_t busy_fields = gamma_busy_fields(g, player);
    uint64_t free_fields = gamma_free_fields(g, player);

    printf("PLAYER %d BUSY: %ld FREE: %ld", player, busy_fields, free_fields);
    if (g->players[player - 1].golden_unused) {
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
static void interactive_move(gamma_t *g, uint32_t player, uint32_t *x, uint32_t *y) {
    bool move_made = false;
    while (!move_made) {
        int ch = getche();
        if (ch == '\033') { // strzałka daje nam trzy znaki : '\033', '[' i jeden z 'A', 'B', 'C', 'D'
            getche(); // omijamy '['
            ch = getche();

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
        }

    }
}

void interactive_play(gamma_t *g) {
    uint32_t x = (g->width + 1) / 2;
    uint32_t y = (g->height + 1) / 2;

    printf(CLEAR_CONSOLE);
    bool end = false;
    char *board_string = NULL;

    while (!end) {
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

                printf("\033[%d;%dH", y, x);

                end = false; // jeśli któryś z graczy może wykonać ruch to nie kończymy

                interactive_move(g, player + 1, &x, &y);

            }
        }
    }

    printf(CLEAR_CONSOLE);
    printf(gamma_board(g));
    for (uint32_t player = 0; player < g->player_count; player++) {
        print_player_info(g, player + 1);
    }
}
