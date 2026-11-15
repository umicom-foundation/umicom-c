/*-----------------------------------------------------------------------------
 * Umicom C
 * File: examples/remainder.uc
 *
 * PURPOSE:
 *   Keep the remainder of a bounded local integer update; this program returns forty-two.
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
    int value = 137;
    value %= 95;
    return value;
}
