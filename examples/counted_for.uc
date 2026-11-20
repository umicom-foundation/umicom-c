/*-----------------------------------------------------------------------------
 * Umicom C
 * File: examples/counted_for.uc
 *
 * PURPOSE:
 *   Calculate a total through a literal-bounded loop and its checked branches.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/
int main(void)
{
    int total = 0;
    /* The loop owns step. Its four literal values determine the four copies
     * of this body, which retain their ordinary conditional behavior. */
    for (int step = 0; step < 4; ++step) {
        if (step < 2) {
            total += 7;
        } else {
            total += 14;
        }
    }
    /* Two additions of seven and two additions of fourteen total 42. */
    return total;
}
