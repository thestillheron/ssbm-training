#ifdef TRAINING_BUILD

#include "drop_height.h"

#include <melee/ft/forward.h>

typedef struct DropHeightRow {
    int kind; /* FighterKind */
    float height;
} DropHeightRow;

/* The starting values come from each character's max fall speed (terminal
 * velocity, from the fighter data on the disc), on a straight line from
 * Jigglypuff (1.3, drops from 21) to Falco (3.1, drops from 40, the old fixed
 * height):
 *
 *     height = 21 + (max fall speed - 1.3) * 19 / 1.8
 *
 * rounded to tenths. The values have been tuned by hand since, so edit a row
 * freely; the max fall speed beside it only says where it started. Zelda and
 * Sheik (Seak) have their own rows; Ice Climbers use Popo's. */
static const DropHeightRow drop_heights[] = {
    { Ft_Kind_Falco, 40.0F }, /* 3.1 */
    { Ft_Kind_Captain, 37.9F }, /* 2.9 */
    { Ft_Kind_Fox, 36.8F }, /* 2.8 */
    { Ft_Kind_Donkey, 32.6F }, /* 2.4 */
    { Ft_Kind_Mars, 32.6F }, /* 2.4 (Roy) */
    { Ft_Kind_Emblem, 30.5F }, /* 2.2 (Marth) */
    { Ft_Kind_Link, 29.8F }, /* 2.13 */
    { Ft_Kind_CLink, 29.8F }, /* 2.13 */
    { Ft_Kind_Seak, 29.8F }, /* 2.13 (Sheik) */
    { Ft_Kind_Ganon, 28.4F }, /* 2.0 */
    { Ft_Kind_Yoshi, 27.6F }, /* 1.93 */
    { Ft_Kind_Koopa, 27.3F }, /* 1.9 (Bowser) */
    { Ft_Kind_Pikachu, 27.3F }, /* 1.9 */
    { Ft_Kind_Pichu, 27.3F }, /* 1.9 */
    { Ft_Kind_Ness, 26.6F }, /* 1.83 */
    { Ft_Kind_Mario, 25.2F }, /* 1.7 */
    { Ft_Kind_DrMario, 25.2F }, /* 1.7 */
    { Ft_Kind_GameWatch, 25.2F }, /* 1.7 */
    { Ft_Kind_Kirby, 24.2F }, /* 1.6 */
    { Ft_Kind_Luigi, 24.2F }, /* 1.6 */
    { Ft_Kind_Popo, 24.2F }, /* 1.6 (Ice Climbers) */
    { Ft_Kind_Peach, 23.1F }, /* 1.5 */
    { Ft_Kind_Mewtwo, 23.1F }, /* 1.5 */
    { Ft_Kind_Samus, 22.1F }, /* 1.4 */
    { Ft_Kind_Zelda, 22.1F }, /* 1.4 */
    { Ft_Kind_Purin, 21.0F }, /* 1.3 (Jigglypuff) */
};

float training_drop_height(int fighter_kind)
{
    int i;

    for (i = 0; i < (int) (sizeof(drop_heights) / sizeof(drop_heights[0])); i++) {
        if (drop_heights[i].kind == fighter_kind) {
            return drop_heights[i].height;
        }
    }
    return DROP_HEIGHT_FALLBACK;
}

#endif
