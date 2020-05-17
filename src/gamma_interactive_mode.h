/** @file
 * Interfejs klasy obsługującej gre interaktywną.
 * Gra interaktywna działa poprawnie jedynie dla < 10 graczy.
 * @author Adrian Matwiejuk <am418419@students.mimuw.edu.pl>
 * @copyright Uniwersytet Warszawski
 * @date 17.05.2020
 */

#ifndef GAMMA_GAMMA_INTERACTIVE_MODE_H
#define GAMMA_GAMMA_INTERACTIVE_MODE_H

#include "gamma.h"
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#define CLEAR_CONSOLE "\e[1;1H\e[2J"

/** @brief Główna funkcja obsługująca grę interaktywną.
 * Wypisuje aktualny stan planszy i umożliwia kolejnym graczom ruch aż do momentu gdy żaden nie jest możliwy.
 * @param[in] *g   – wskaźnik na strukturę przechowującą stan gry,
 */
void interactive_play(gamma_t *g);

#endif /* GAMMA_GAMMA_INTERACTIVE_MODE_H */
