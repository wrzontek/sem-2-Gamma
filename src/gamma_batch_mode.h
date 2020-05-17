/** @file
 * Interfejs klasy obsługującej gre w trybie wsadowym.
 *
 * @author Adrian Matwiejuk <am418419@students.mimuw.edu.pl>
 * @copyright Uniwersytet Warszawski
 * @date 17.05.2020
 */

#ifndef GAMMA_GAMMA_BATCH_MODE_H
#define GAMMA_GAMMA_BATCH_MODE_H

#include "gamma.h"
#include "input.h"

/** @brief Główna funkcja obsługująca grę w trybie wsadowym.
 * Wczytuje polecenia z wejścia i je obsługuje.
 * @param[in] *g   – wskaźnik na strukturę przechowującą stan gry,
 * @param[in] current_line_count - liczba dotyczasowych linijek na wejściu
 */
void batch_read_input(gamma_t *g, long long current_line_count);

#endif /* GAMMA_GAMMA_BATCH_MODE_H */
