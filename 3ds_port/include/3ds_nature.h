#ifndef CTR_NATURE_H
#define CTR_NATURE_H

/*
 * What a nature does to the stats, the third generation's rule: of the 25 natures, number n raises stat n / 5 and lowers
 * stat n % 5, counted in the order ATTACK, DEFENSE, SPEED, SP. ATK, SP. DEF; when the two are the same stat the nature
 * is neutral. The answers are in the Summary's order instead (ATTACK, DEFENSE, SP. ATK, SP. DEF, SPEED: 0-4), worked
 * out, not looked up; -1 for a neutral nature or a number past the 25.
 */

/* The rule's stat k (0-4) as the Summary lists it: SPEED moves to the end, SP. ATK and SP. DEF one up. */
static inline int CtrNature_Listed(int k)
{
    return k == 2 ? 4 : k > 2 ? k - 1 : k;
}

/* The stat the nature raises, in the Summary's order, or -1. */
static inline int CtrNature_Up(int nature)
{
    if (nature < 0 || nature >= 25 || nature / 5 == nature % 5)
        return -1;
    return CtrNature_Listed(nature / 5);
}

/* The stat the nature lowers, in the Summary's order, or -1. */
static inline int CtrNature_Down(int nature)
{
    if (nature < 0 || nature >= 25 || nature / 5 == nature % 5)
        return -1;
    return CtrNature_Listed(nature % 5);
}

#endif
