/*-----------------------------------------------------------------------------
 * Umicom C
 * File: examples/checked_shifts.uc
 *
 * PURPOSE:
 *   Demonstrate checked integer shift statements without host side effects.
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
    int value = 84;
    value >>= 1; /* Divide by two: 84 becomes 42. */
    value <<= 0; /* A zero count preserves the value. */
    return value;
}
