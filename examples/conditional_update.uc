/*-----------------------------------------------------------------------------
 * Umicom C
 * File: examples/conditional_update.uc
 *
 * PURPOSE:
 *   Select one checked local update while retaining the existing final return and branch wire format.
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
    int value = 1;
    if (0) {
        value = 7;
    } else {
        value = 42;
    }
    return value;
}
