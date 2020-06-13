/** @file
 * Implementacja klasy pomocniczej do wczytywania wejścia.
 *
 * @author Adrian Matwiejuk <am418419@students.mimuw.edu.pl>
 * @copyright Uniwersytet Warszawski
 * @date 17.05.2020
 */
#include "input.h"

static bool is_whitespace(int c) {
    return (c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r');
}

static void ignore_line(int *c, bool *end_of_input) {
    while (*c != EOF && *c != '\n') {
        *c = getchar();
        if (*c == EOF)
            *end_of_input = true;
    }
}

/** @brief Wczytuje ciąg cyfr (tworzący liczbę) z wejścia, przekształca ją w liczbę
 *  Funkcja alokuje dynamicznie pamięć na wczytywany ciąg cyfr jednocześnie sprawdzając jego poprawność.
 *  Przekształca stworzoną tablicę znaków w unsigned long long po czym zwalnia pamięć.
 * @param[in] *c   – wskaźnik na aktualny znak na wejściu,
 * @param[in] *correct_command  – wskaźnik na bool mówiący czy cała aktualna komenda jest do tej pory poprawna,
 * @param[in] *end_of_input - wskaźnik na bool mówiący czy natrafiono na EOF,
 * @return Wczytany argument polecenia w postaci unsigned long long
 */
static unsigned long long get_argument(int *c, bool *correct_command, bool *end_of_input) {
    char *argument_string = calloc(3, sizeof(char));
    int i = 0;
    int size = 3;
    while (*c != EOF && *c != '\n' && !is_whitespace(*c) && !(*c < '0' || *c > '9')) {
        if (i == size - 2) {
            size = size * 3 / 2;
            argument_string = realloc(argument_string, size);
        }
        argument_string[i] = *c;
        i++;
        *c = getchar();
        if (*c == EOF) {
            *end_of_input = true;
        }
        if (!is_whitespace(*c) && *c != '\n' && (*c < '0' || *c > '9')) {
            *correct_command = false;
            free(argument_string);
            return 0;
        }
    }
    argument_string[i] = '\0';
    unsigned long long argument = strtoull(argument_string, NULL, 10);
    free(argument_string);
    return argument;
}

int *get_command(gamma_t *g, bool *correct_command, bool *end_of_input) {
    int *command = NULL;
    int c = getchar();
    if (c == EOF) {
        *end_of_input = true;
    } else if (c == '#' || c == '\n') {
        ignore_line(&c, end_of_input);
    } else if ((c == 'B' || c == 'I') && g != NULL) { // gra już zainicjalizowana, drugi raz nie można
        *correct_command = false;
        ignore_line(&c, end_of_input);
    } else if (c != 'B' && c != 'I' && c != 'm' && c != 'g' && c != 'b' && c != 'f' && c != 'q' && c != 'p') {
        *correct_command = false;
        while (c != EOF && c != '\n') {
            c = getchar();
            if (c == EOF) {
                *end_of_input = true;
            }
        }
    } else {
        int arg_count;
        if (c == 'B' || c == 'I') {
            arg_count = 5;
        } else if (c == 'm' || c == 'g') {
            arg_count = 4;
        } else if (c == 'b' || c == 'f' || c == 'q') {
            arg_count = 2;
        } else {
            arg_count = 1;
        }
        command = calloc(arg_count, sizeof(int));
        command[0] = c;
        int i = 1;
        c = getchar();

        if (arg_count > 1 && !is_whitespace(c)) { // musi być whitespace po pierwszym znaku
            *correct_command = false;
            ignore_line(&c, end_of_input);
            free(command);
            return NULL;
        }

        while (c != EOF && c != '\n') {
            while (is_whitespace(c)) {
                c = getchar();
            }
            if (c != '\n') {
                if (!*correct_command || (c < '0' || c > '9') || i >= arg_count) {
                    *correct_command = false;
                    ignore_line(&c, end_of_input);
                    free(command);
                    command = NULL;
                    break;
                } else {
                    unsigned long long arg = get_argument(&c, correct_command, end_of_input);
                    if (arg <= UINT32_MAX) { // wartość mieści się w dopuszczalnym zakresie
                        command[i] = (int) arg;
                        i++;
                        if (c != EOF && c != '\n')
                            c = getchar();
                    } else {
                        *correct_command = false;
                    }
                }
            }
        }
        if (*correct_command && i != arg_count) {  // za mało argumentów
            *correct_command = false;
            free(command);
            command = NULL;
        }
    }
    return command;
}

int determine_game_type(gamma_t **g, long long *current_line_count) {
    bool end_of_input = false;
    while (!end_of_input) {
        bool correct_command = true;

        int *command = get_command(*g, &correct_command, &end_of_input);
        (*current_line_count)++;

        if (!(correct_command && command == NULL)) { // jeżeli prawda to komentarz/wiersz pusty
            if (!correct_command || (command[0] != 'B' && command[0] != 'I')) {
                fprintf(stderr, "ERROR %lld\n", *current_line_count);
            } else {
                for (int i = 1; i <= 4; i++) {
                    if (command[i] <= 0)
                        correct_command = false;
                }
                if (correct_command) {
                    *g = gamma_new(command[1], command[2], command[3], command[4]);
                    if (*g != NULL) {
                        if (command[0] == 'B') {
                            printf("OK %lld\n", *current_line_count);
                            free(command);
                            return BATCH;
                        } else if (command[0] == 'I') {
                            free(command);
                            return INTERACTIVE;
                        }
                    }
                } else {
                    fprintf(stderr, "ERROR %lld\n", *current_line_count);
                }
            }
        }

        if (command != NULL) {
            free(command);
        }
    }
    return 0;
}
