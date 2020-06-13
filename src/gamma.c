/** @file
 * Implementacja modułu silnika gry gamma
 *
 * @author Adrian Matwiejuk <am418419@students.mimuw.edu.pl>
 * @copyright Uniwersytet Warszawski
 * @date 15.04.2020
 */

#include "gamma.h"

/**
 * Funckja pomocnicza zamieniająca wskaźniki na pola
 */
static void swap(field **a, field **b) {
    field *c = *a;
    *a = *b;
    *b = c;
}

gamma_t *gamma_new(uint32_t width, uint32_t height,
                   uint32_t players, uint32_t areas) {
    if (width < 1 || height < 1 || players < 1 || areas < 1) {
        return NULL;
    }

    gamma_t *g = malloc(sizeof(*g));
    if (g == NULL) {
        return NULL;
    }
    g->width = width;
    g->height = height;
    g->max_areas = areas;

    g->player_count = players;
    g->players = malloc(players * sizeof(player));
    if (g->players == NULL) {
        free(g);
        return NULL;
    }
    for (uint32_t i = 0; i < players; i++) {
        g->players[i].busy_fields = 0;
        g->players[i].free_fields = 0;
        g->players[i].areas = 0;
        g->players[i].golden_unused = true;
    }

    g->board = malloc(width * sizeof(field));
    if (g->board == NULL) {
        free(g->players);
        free(g);
        return NULL;
    }
    for (uint32_t i = 0; i < width; i++) {
        g->board[i] = malloc(height * sizeof(field));
        if (g->board[i] == NULL) {
            free(g->players);
            for (uint32_t j = 0; j < i; j++) {
                free(g->board[j]);
            }
            free(g->board);
            free(g);
            return NULL;
        }
    }

    for (uint32_t i = 0; i < width; i++) {
        for (uint32_t j = 0; j < height; j++) {
            g->board[i][j].owner = NONE;
            g->board[i][j].root = &g->board[i][j];
        }
    }

    return g;
}

void gamma_delete(gamma_t *g) {
    if (g != NULL) {
        free(g->players);
        for (uint32_t i = 0; i < g->width; i++) {
            free(g->board[i]);
        }
        free(g->board);
        free(g);
    }
}

/** @brief Podaje ilość pól należacych do danego gracza sąsiadujących z danym polem.
 * @param[in,out] g   – wskaźnik na strukturę przechowującą stan gry,
 * @param[in] player  – numer gracza, liczba dodatnia niewiększa od wartości
 *                      @p players z funkcji @ref gamma_new,
 * @param[in] x       – numer kolumny, liczba nieujemna mniejsza od wartości
 *                      @p width z funkcji @ref gamma_new,
 * @param[in] y       – numer wiersza, liczba nieujemna mniejsza od wartości
 *                      @p height z funkcji @ref gamma_new.
 */
static uint32_t owner_fields_neighbouring(gamma_t *g, uint32_t player, uint32_t x, uint32_t y) {
    uint32_t owner_fields_neighbouring = 0;
    int change[4] = {-1, 0, 1, 0};
    for (uint32_t i = 0; i < 4; i++) {
        if ((int64_t)x + change[i] >= 0 && (int64_t)x + change[i] < g->width &&
            (int64_t)y + change[3 - i] >= 0 && (int64_t)y + change[3 - i] < g->height) {
            if (g->board[x + change[i]][y + change[3 - i]].owner == player)
                owner_fields_neighbouring++;
        }
    }
    return owner_fields_neighbouring;
}

/** @brief Znajduje reprezentanta obszaru do którego należy pole.
 * Funkcja znajduje rekurencyjnie reprezentanta obszaru do którego należy dane pole,
 * kompresując również ścieżkę wskaźników do reprezentanta.
 */
static field *find_root(field *a) {
    assert(a != NULL);
    if (a->root != a) {
        a->root = find_root(a->root);
    }
    return a->root;
}

/** @brief Dołącza pole do obszaru.
 * Funkcja ustawia root danego pola na reprezentanta
 * obszaru do którego pole jest przyłączane.
 * @param[in,out] g   – wskaźnik na strukturę przechowującą stan gry,
 * @param[in] player  – numer gracza, liczba dodatnia niewiększa od wartości
 *                      @p players z funkcji @ref gamma_new,
 * @param[in] x       – numer kolumny, liczba nieujemna mniejsza od wartości
 *                      @p width z funkcji @ref gamma_new,
 * @param[in] y       – numer wiersza, liczba nieujemna mniejsza od wartości
 *                      @p height z funkcji @ref gamma_new.
 */
static void unite_single(gamma_t *g, uint32_t player, uint32_t x, uint32_t y) {
    int change[4] = {-1, 0, 1, 0};
    for (uint32_t i = 0; i < 4; i++) {
        if ((int64_t)x + change[i] >= 0 && (int64_t)x + change[i] < g->width &&
            (int64_t)y + change[3 - i] >= 0 && (int64_t)y + change[3 - i] < g->height) {
            if (g->board[x + change[i]][y + change[3 - i]].owner == player)
                g->board[x][y].root = find_root(&g->board[x + change[i]][y + change[3 - i]]);
        }
    }
}

/** @brief Dołącza pole do obszaru, łączy obszary.
 * Funkcja ustawia root danego pola na reprezentanta
 * jednego z obszarów gracza z którymi sąsiaduje.
 *
 * W wypadku, w którym nowo zajęte pole
 * sąsiaduje z rozłącznymi obszarami gracza łączy je,
 * zmiejszając parametr areas gracza i ustawiając root
 * reprezentantów obszarów na reprezentanta
 * jednego z obszarów.
 * @param[in,out] g   – wskaźnik na strukturę przechowującą stan gry,
 * @param[in] player  – numer gracza, liczba dodatnia niewiększa od wartości
 *                      @p players z funkcji @ref gamma_new,
 * @param[in] x       – numer kolumny, liczba nieujemna mniejsza od wartości
 *                      @p width z funkcji @ref gamma_new,
 * @param[in] y       – numer wiersza, liczba nieujemna mniejsza od wartości
 *                      @p height z funkcji @ref gamma_new.
 */
static void unite_multiple(gamma_t *g, uint32_t player, uint32_t x, uint32_t y) {
    int change[4] = {-1, 0, 1, 0};
    for (uint32_t i = 0; i < 4; i++) {
        if ((int64_t)x + change[i] >= 0 && (int64_t)x + change[i] < g->width &&
            (int64_t)y + change[3 - i] >= 0 && (int64_t)y + change[3 - i] < g->height) {
            if (g->board[x + change[i]][y + change[3 - i]].owner == player) {
                g->board[x][y].root = find_root(&g->board[x + change[i]][y + change[3 - i]]);
                break;
            }
        }
    }

    for (uint32_t i = 0; i < 4; i++) {
        if ((int64_t)x + change[i] >= 0 && (int64_t)x + change[i] < g->width &&
            (int64_t)y + change[3 - i] >= 0 && (int64_t)y + change[3 - i] < g->height) {
            if (g->board[x + change[i]][y + change[3 - i]].owner == player) {
                field *root = find_root(&g->board[x + change[i]][y + change[3 - i]]);
                if (root != g->board[x][y].root) {
                    g->players[player - 1].areas--;
                    root->root = g->board[x][y].root;
                }
            }
        }
    }
}

/** @brief Podaje liczbe nowych sąsiednich wolnych pól.
 * Funkcja podaje liczbę pól sąsiadujących z nim, które są
 * niezajęte i nie sąsiadują z żadnym polem zajętym przez danego gracza.
 * @param[in,out] g   – wskaźnik na strukturę przechowującą stan gry,
 * @param[in] player  – numer gracza, liczba dodatnia niewiększa od wartości
 *                      @p players z funkcji @ref gamma_new,
 * @param[in] x       – numer kolumny, liczba nieujemna mniejsza od wartości
 *                      @p width z funkcji @ref gamma_new,
 * @param[in] y       – numer wiersza, liczba nieujemna mniejsza od wartości
 *                      @p height z funkcji @ref gamma_new.
 * @return liczba nowych sąsiednich wolnych pól
 */
static uint64_t new_free_fields(gamma_t *g, uint32_t player, uint32_t x, uint32_t y) {
    uint64_t new_free_fields = 0;
    int change[4] = {-1, 0, 1, 0};
    for (uint32_t i = 0; i < 4; i++) {
        if ((int64_t)x + change[i] >= 0 && (int64_t)x + change[i] < g->width &&
            (int64_t)y + change[3 - i] >= 0 && (int64_t)y + change[3 - i] < g->height) {
            if (g->board[x + change[i]][y + change[3 - i]].owner == NONE)
                if (owner_fields_neighbouring(g, player, x + change[i], y + change[3 - i]) == 0)
                    new_free_fields++;
        }
    }

    return new_free_fields;
}

/** @brief Ustawia pole jako zajęte
 * Funkcja sprawia, że dany gracz staje się właścicielem danego pola,
 * jego liczba zajętych pól zwiększa się o jeden, a liczba sąsiednich wolnych pól
 * innych graczy, którzy sąsiadują z tym polem zmniejsza się o jeden.
 * @param[in,out] g   – wskaźnik na strukturę przechowującą stan gry,
 * @param[in] player  – numer gracza, liczba dodatnia niewiększa od wartości
 *                      @p players z funkcji @ref gamma_new,
 * @param[in] x       – numer kolumny, liczba nieujemna mniejsza od wartości
 *                      @p width z funkcji @ref gamma_new,
 * @param[in] y       – numer wiersza, liczba nieujemna mniejsza od wartości
 *                      @p height z funkcji @ref gamma_new.
 */
static void make_field_busy(gamma_t *g, uint32_t player, uint32_t x, uint32_t y) {
    g->board[x][y].owner = player;
    g->players[player - 1].busy_fields++;
    uint32_t owner[4] = {NONE, NONE, NONE, NONE};
    int change[4] = {-1, 0, 1, 0};
    for (uint32_t i = 0; i < 4; i++) {
        if ((int64_t)x + change[i] >= 0 && (int64_t)x + change[i] < g->width &&
            (int64_t)y + change[3 - i] >= 0 && (int64_t)y + change[3 - i] < g->height) {

            if (g->board[x + change[i]][y + change[3 - i]].owner != player &&
                g->board[x + change[i]][y + change[3 - i]].owner != NONE) {
                owner[i] = g->board[x + change[i]][y + change[3 - i]].owner;
                bool unique = true;
                /*
                 * nie zabieramy wolnego pola danemu graczowi
                 * więcej niż raz
                 */
                for (uint32_t j = 0; j < i; j++) {
                    if (owner[i] == owner[j])
                        unique = false;
                }
                if (unique)
                    g->players[owner[i] - 1].free_fields--;
            }
        }
    }
}

bool gamma_move(gamma_t *g, uint32_t player, uint32_t x, uint32_t y) {
    if (player < 1 || player > g->player_count || x >= g->width || y >= g->height) {
        return false;
    }
    if (g->board[x][y].owner != NONE) {
        return false;
    }
    int own_fields_neighbouring = owner_fields_neighbouring(g, player, x, y);
    if (own_fields_neighbouring > 0) {
        g->players[player - 1].free_fields += new_free_fields(g, player, x, y) - 1;
        make_field_busy(g, player, x, y);
        if (own_fields_neighbouring == 1)
            unite_single(g, player, x, y);
        else
            unite_multiple(g, player, x, y);
    } else {
        if (g->players[player - 1].areas >= g->max_areas) {
            return false;
        } else {
            g->players[player - 1].areas++;
            g->players[player - 1].free_fields += new_free_fields(g, player, x, y);
            make_field_busy(g, player, x, y);

            //pole staje się reprezentanem nowego obszaru
            g->board[x][y].root = &g->board[x][y];
        }
    }
    return true;
}

/** @brief Podaje liczbę nowo powstałych obszarów
 * Funkcja podaje liczbe nowych obszarów, które moga powstać
 * na skutek złotego ruchu innego gracza, dzielącego obszar gracza
 * na 1-4 części (czyli 0-3 nowe).
 * @param[in,out] g   – wskaźnik na strukturę przechowującą stan gry,
 * @param[in] player  – numer gracza, liczba dodatnia niewiększa od wartości
 *                      @p players z funkcji @ref gamma_new,
 * @param[in] x       – numer kolumny, liczba nieujemna mniejsza od wartości
 *                      @p width z funkcji @ref gamma_new,
 * @param[in] y       – numer wiersza, liczba nieujemna mniejsza od wartości
 *                      @p height z funkcji @ref gamma_new.
 * @return liczba nowo powstałych obszarów
 */
static uint32_t separated_areas(gamma_t *g, uint32_t player, uint32_t x, uint32_t y) {
    uint32_t separated_areas = 0;
    field *root[4] = {NULL, NULL, NULL, NULL};
    int change[4] = {-1, 0, 1, 0};
    for (uint32_t i = 0; i < 4; i++) {
        if ((int64_t)x + change[i] >= 0 && (int64_t)x + change[i] < g->width &&
            (int64_t)y + change[3 - i] >= 0 && (int64_t)y + change[3 - i] < g->height) {
            if (g->board[x + change[i]][y + change[3 - i]].owner == player) {
                root[i] = g->board[x + change[i]][y + change[3 - i]].root;
            }
        }
    }

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3 - i; j++) {
            if (root[j] > root[j + 1]) {
                swap(&root[j], &root[j + 1]);
            }
        }
    }

    for (int i = 1; i < 4; i++) {
        if (root[i] && root[i - 1] && root[i] != root[i - 1])
            separated_areas++;
    }

    return separated_areas;
}

/** @brief Funkcja pomocnicza do update_roots
 * Rekurencyjnie przechodzi osiągalne pola gracza
 * ustawiając ich reprezentanta na sąsiada
 * pola przekazanego do update_roots, z którym funkcja
 * set_accessible_root została wywowałana
 * @param[in,out] g   – wskaźnik na strukturę przechowującą stan gry,
 * @param[in] player  – numer gracza, liczba dodatnia niewiększa od wartości
 *                      @p players z funkcji @ref gamma_new,
 * @param[in] x       – numer kolumny, liczba nieujemna mniejsza od wartości
 *                      @p width z funkcji @ref gamma_new,
 * @param[in] y       – numer wiersza, liczba nieujemna mniejsza od wartości
 *                      @p height z funkcji @ref gamma_new.
 */
static void set_accessible_root(gamma_t *g, uint32_t player, uint32_t x, uint32_t y) {
    field *new_root = g->board[x][y].root;
    int change[4] = {-1, 0, 1, 0};
    for (uint32_t i = 0; i < 4; i++) {
        if ((int64_t)x + change[i] >= 0 && (int64_t)x + change[i] < g->width &&
            (int64_t)y + change[3 - i] >= 0 && (int64_t)y + change[3 - i] < g->height) {
            if (g->board[x + change[i]][y + change[3 - i]].owner == player &&
                g->board[x + change[i]][y + change[3 - i]].root != new_root) {

                g->board[x + change[i]][y + change[3 - i]].root = new_root;
                set_accessible_root(g, player, x + change[i], y + change[3 - i]);

            }
        }
    }
}

/** @brief Uaktualnia reprezantantów obszarów
 * Funkcja ustawia swoim sasiadom reprezentantów, którzy
 * na pewno są osiągalni. Jest to konieczne, bo przy złotym
 * ruchu może dojść do rozdzielenia obszarów i część pól
 * może wskazywać na nieosiągalnego reprezentanta.
 * @param[in,out] g   – wskaźnik na strukturę przechowującą stan gry,
 * @param[in] player  – numer gracza, liczba dodatnia niewiększa od wartości
 *                      @p players z funkcji @ref gamma_new,
 * @param[in] x       – numer kolumny, liczba nieujemna mniejsza od wartości
 *                      @p width z funkcji @ref gamma_new,
 * @param[in] y       – numer wiersza, liczba nieujemna mniejsza od wartości
 *                      @p height z funkcji @ref gamma_new.
 */
static void update_roots(gamma_t *g, uint32_t player, uint32_t x, uint32_t y) {
    int change[4] = {-1, 0, 1, 0};
    for (uint32_t i = 0; i < 4; i++) {
        if ((int64_t)x + change[i] >= 0 && (int64_t)x + change[i] < g->width &&
            (int64_t) y + change[3 - i] >= 0 && (int64_t)y + change[3 - i] < g->height) {
            if (g->board[x + change[i]][y + change[3 - i]].owner == player) {
                g->board[x + change[i]][y + change[3 - i]].root = NULL;
            }
        }
    }

    for (uint32_t i = 0; i < 4; i++) {
        if ((int64_t)x + change[i] >= 0 && (int64_t)x + change[i] < g->width &&
        (int64_t)y + change[3 - i] >= 0 && (int64_t)y + change[3 - i] < g->height) {
            if (g->board[x + change[i]][y + change[3 - i]].owner == player &&
                g->board[x + change[i]][y + change[3 - i]].root == NULL) {
                g->board[x + change[i]][y + change[3 - i]].root = &g->board[x + change[i]][y + change[3 - i]];
                set_accessible_root(g, player, x + change[i], y + change[3 - i]);
            }
        }
    }
}

bool gamma_golden_move(gamma_t *g, uint32_t player, uint32_t x, uint32_t y) {
    if (g->players[player - 1].golden_unused == false) {
        return false;
    }
    if (player < 1 || player > g->player_count || x >= g->width || y >= g->height) {
        return false;
    }
    if (g->board[x][y].owner == NONE || g->board[x][y].owner == player) {
        return false;
    }
    uint32_t victim = g->board[x][y].owner;
    int own_fields_neighbouring = owner_fields_neighbouring(g, player, x, y);
    int victims_fields_neighbouring = owner_fields_neighbouring(g, victim, x, y);

    if (own_fields_neighbouring > 0) {
        g->players[player - 1].free_fields += new_free_fields(g, player, x, y);
        g->players[player - 1].busy_fields++;

        g->board[x][y].owner = player;

        g->players[victim - 1].free_fields -= new_free_fields(g, victim, x, y);
        g->players[victim - 1].busy_fields--;


        if (own_fields_neighbouring == 1)
            unite_single(g, player, x, y);
        else
            unite_multiple(g, player, x, y);
    } else {
        if (g->players[player - 1].areas >= g->max_areas) {
            return false;
        } else {
            g->players[player - 1].areas++;
            g->players[player - 1].free_fields += new_free_fields(g, player, x, y);
            g->players[player - 1].busy_fields++;

            g->board[x][y].owner = player;

            g->players[victim - 1].free_fields -= new_free_fields(g, victim, x, y);
            g->players[victim - 1].busy_fields--;

            //pole staje się reprezentanem nowego obszaru
            g->board[x][y].root = &g->board[x][y];
        }
    }

    if (victims_fields_neighbouring == 0) {
        g->players[victim - 1].areas--;
    } else if (victims_fields_neighbouring > 1) {
        update_roots(g, victim, x, y);
        int new_areas = separated_areas(g, victim, x, y);

        if (new_areas > 0) {
            g->players[victim - 1].areas += new_areas;
            if (g->players[victim - 1].areas > g->max_areas) {
                // cofamy ruch
                if (own_fields_neighbouring == 0) {
                    g->players[player - 1].areas--;
                }
                g->players[player - 1].busy_fields--;

                g->players[victim - 1].free_fields += new_free_fields(g, victim, x, y);
                g->players[victim - 1].busy_fields++;
                g->board[x][y].owner = victim;

                g->players[player - 1].free_fields -= new_free_fields(g, player, x, y);
                unite_multiple(g, victim, x, y);
                return false;
            }
        }
    }

    g->players[player - 1].golden_unused = false;
    return true;
}


uint64_t gamma_busy_fields(gamma_t *g, uint32_t player) {
    if (player > g->player_count) {
        return 0;
    }
    return g->players[player - 1].busy_fields;
}

uint64_t gamma_free_fields(gamma_t *g, uint32_t player) {
    if (player > g->player_count) {
        return 0;
    }
    if (g->players[player - 1].areas < g->max_areas) {
        uint64_t free_fields = (uint64_t)g->width * g->height;
        for (uint32_t i = 0; i < g->player_count; i++) {
            free_fields -= g->players[i].busy_fields;
        }
        return (free_fields);
    } else {
        return g->players[player - 1].free_fields;
    }
}

bool golden_target_avalible(gamma_t *g, uint32_t player) {

    for (uint32_t x = 0; x < g->width; x++) {
        for (uint32_t y = 0; y < g->height; y++) {

            field current_field = g->board[x][y];
            if (current_field.owner != player && current_field.owner != NONE) {
                if (owner_fields_neighbouring(g, player, x, y) > 0) {

                    uint32_t victim_areas_under_limit = g->max_areas - g->players[current_field.owner - 1].areas;
                    if (victim_areas_under_limit >= 2) {// w tej sytuacji ofiarze przybędą maksymalnie dwa nowe obszary
                        return true;
                    }
                    uint32_t victim = current_field.owner;
                    current_field.owner = player;
                    update_roots(g, victim, x, y);
                    uint32_t new_areas = separated_areas(g, victim, x ,y);
                    current_field.owner = victim;

                    if (new_areas <= victim_areas_under_limit)
                        return true;

                }
            }

        }
    }

    return false;
}

bool gamma_golden_possible(gamma_t *g, uint32_t player) {
    if (player > g->player_count || g->players[player - 1].golden_unused == false) {
        return false;
    }
    bool other_players_have_fields = false;
    for (uint32_t i = 0; i < g->player_count; i++) {
        if (i != player - 1 && g->players[i].busy_fields > 0) {
            other_players_have_fields = true;
            break;
        }
    }
    if (!other_players_have_fields) {
        return false;
    }
    if (g->players[player - 1].areas < g->max_areas) {
        return true;
    }

    return golden_target_avalible(g, player);
}

char *gamma_board(gamma_t *g) {
    if (g == NULL) {
        return NULL;
    }
    char *board = NULL;
    if (g->player_count < 10) {
        board = malloc(1 + (uint64_t)(g->width + 1) * g->height * sizeof(char));
        if (board == NULL) {
            return NULL;
        }
        for (uint32_t y = 0; y < g->height; y++) {

            for (uint32_t x = 0; x < g->width; x++) {
                if (g->board[x][y].owner == NONE) {
                    board[(g->height - y - 1) * (g->width + 1) + x] = '.';
                } else {
                    board[(g->height - y - 1) * (g->width + 1) + x] = g->board[x][y].owner + '0';
                }
            }

            board[(y + 1) * g->width + y] = '\n';
        }
        board[(g->width + 1) * g->height] = '\0';
    } else {
        uint64_t bonus_space = 0;
        for (uint32_t y = 0; y < g->height; y++) {
            for (uint32_t x = 0; x < g->width; x++) {
                if (g->board[x][y].owner >= 10) {
                    bonus_space += 3;
                }
            }
        }
        board = malloc(1 + bonus_space + (uint64_t)(g->width + 1) * g->height * sizeof(char));
        if (board == NULL) {
            return NULL;
        }
        for (uint64_t i = 0; i < bonus_space + (g->width + 1) * g->height; i++) {
            board[i] = '\n';
        }
        uint64_t current_bonus = bonus_space;
        for (uint32_t y = 0; y < g->height; y++) {
            bool stop = false;
            for (uint32_t x = g->width - 1; !stop; x--) {
                if (g->board[x][y].owner == NONE) {
                    board[(g->height - y - 1) * (g->width + 1) + x + current_bonus] = '.';
                } else if (g->board[x][y].owner < 10) {
                    board[(g->height - y - 1) * (g->width + 1) + x + current_bonus] =
                            g->board[x][y].owner + '0';
                } else {
                    int current_space = (g->height - y - 1) * (g->width + 1) + x + current_bonus;
                    current_space -= 3;
                    board[current_space] = '[';
                    int first_digit = g->board[x][y].owner / 10;
                    board[current_space + 1] = first_digit + '0';
                    int second_digit = g->board[x][y].owner - first_digit * 10;
                    board[current_space + 2] = second_digit + '0';
                    board[current_space + 3] = ']';
                    current_bonus -= 3;
                }
                if (x == 0)
                    stop = true;
            }

        }
        board[(g->width + 1) * g->height + bonus_space] = '\0';
    }
    return board;
}
