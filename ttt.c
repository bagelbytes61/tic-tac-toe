#include <limits.h>
#include <stdbool.h>
#include <stdio.h>

/* 
    First revision size: 877200 bytes
    Second revision size: 872216 -- replace rand() with much smaller xorshift rand
*/


static unsigned int rand_that_doesnt_take_up_5kb(void) {
    static unsigned int seed = 0xfa57;

    seed ^= seed << 7;
    seed ^= seed >> 9;
    seed ^= seed << 8;

    return seed;
}

static const unsigned short int wstates[8] = { 7, 56, 73, 84, 146, 273, 292, 448 }; 
static const unsigned short int *wstates_end = wstates + sizeof(wstates) / sizeof(wstates[0]);

static bool move(unsigned short int *bstate, unsigned short int *pstate, int pos) {
   if (*bstate & (1 << pos)) {
       return false;
   }

   *pstate |= (1 << pos);
   *bstate |= *pstate;

   return true;
}

static bool cell_taken(unsigned short int state, int pos) {
    return state & (1 << pos);
}

static void move_ai(unsigned short int *bstate, unsigned short int *pstate) {
    int x, y, z;

    for (x = 0; x < 9; ++x) {
        if (cell_taken(*bstate, x)) {
            if (!cell_taken(*pstate, x)) {
                printf("cell %d is taken by the player...\n", x);
                continue;
            }
        }

        for (y = 0; y < 9; ++y) {
            if (cell_taken(*bstate, y)) {
                if (!cell_taken(*pstate, y)) {
                    continue;
                }
            }
            for (z = 0; z < 9; ++z) {
                if (cell_taken(*bstate, z)) {
                    if (!cell_taken(*pstate, z)) {
                        continue;
                    }
                }
                const unsigned short int *s = wstates;
                for (; s != wstates_end; ++s) {
                    if ((1 << x) + (1 << y) + (1 << z) == *s) {
                        goto solved;
                    }
                }
            }
        }
    }

    /* there is no winning solution -- yolo */
    while (true) {
        const int mask = 1 << rand_that_doesnt_take_up_5kb() % 9;

        if (!(*pstate & mask)) {
            *pstate |= mask;
            goto board_state;
        }
    }

solved:
    {
        const int seq[] = { x, y, z };
        const int *p = seq;

        for (; p != seq + 4; ++p) {
            if (!cell_taken(*pstate, *p)) {
                *pstate |= (1 << *p);
                break;
            }
        }
    }

board_state:
    *bstate |= *pstate;
}

static bool check_win(unsigned short int pstate) {
    const unsigned short int *s = wstates;
    for (; s != wstates_end; ++s) {
        if ((pstate & *s) == *s) {
            return true;
        }
    }

    return false;
}

#if defined(ENABLE_DRAW_GAME)
static void draw_game(unsigned short int x, unsigned short int o) {
    puts("\n\n\n\n");
    for (int j = 0; j < 3; ++j) {
        for (int k = 0; k < 3; ++k) {
            const int i = j * 3 + k;
            char c = '-';
            if (x & (1 << i)) c = 'x';
            if (o & (1 << i)) c = 'o';
            printf("%c ", c);
        }
        puts("");
    }
}
#endif

int main(void) {
    unsigned short int x = 0;
    unsigned short int o = 0;
    unsigned short int board = 0;

    const char *status = NULL;

    while (true) {
        const int p = getc(stdin);
        if (p == 10 || p == EOF) { /* skip linefeed && EOF */
            continue;
        }

        if (!move(&board, &x, p - 0x30)) {
           puts("Invalid move");
           continue;
        }

#if defined(ENABLE_DRAW_GAME)
        draw_game(x, o);
#endif

        if (check_win(x)) {
            status = "x wins";
            break;
        }
        
        if (board == USHRT_MAX) {
            status = "stallmate";
            break;
        }

        move_ai(&board, &o);

#if defined(ENABLE_DRAW_GAME)
        draw_game(x, o);
#endif
        if (check_win(o)) {
            status = "o wins";
            break;
        }
    }

    if (status != NULL) {
        puts(status);
    }

  return 0;
}
