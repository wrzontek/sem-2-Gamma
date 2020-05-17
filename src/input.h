/** @file
 * Interfejs klasy pomocniczej do wczytywania wejścia.
 *
 * @author Adrian Matwiejuk <am418419@students.mimuw.edu.pl>
 * @copyright Uniwersytet Warszawski
 * @date 17.05.2020
 */

#include "gamma.h"
#include <stdio.h>

#ifndef GAMMA_INPUT_H
#define GAMMA_INPUT_H

#define BATCH 1
#define INTERACTIVE 2

/** @brief Wczytuje pojedyncze polecenie z wejścia.
 * @param[in] *g   – wskaźnik na strukturę przechowującą stan gry,
 * @param[in] *correct_command  – wskaźnik na bool mówiący czy cała aktualna komenda jest do tej pory poprawna,
 * @param[in] *end_of_input - wskaźnik na bool mówiący czy natrafiono na EOF,
 * @return <=5 elementowa tablica int reprezentująca wczytaną komendę lub NULL
 */
int *get_command(gamma_t *g, bool *correct_command, bool *end_of_input);


/** @brief Wczytuje wejście i determinuje jaki typ rozgrywki wybrał użytkownik.
 * @param[in, out] **g   – wskaźnik na wskaźnik na strukturę przechowującą stan gry,
 * @param[in, out] *current_line_count - wskaźnik na liczbę dotychczasowo wczytanych linijek wejścia,
 * @return int = 1 lub 2, odpowiednio zdefiniowane jako tryby BATCH lub INTERACTIVE
 */
int determine_game_type(gamma_t **g, long long *current_line_count);

#endif //GAMMA_INPUT_H
