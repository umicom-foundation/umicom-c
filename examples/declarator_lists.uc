/*-----------------------------------------------------------------------------
 * Umicom C
 * File: examples/declarator_lists.uc
 *
 * PURPOSE:
 *   Use initialized local declarations in source order through the bounded integer compiler.
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
    int first = 6, second = first * 7;
    return second;
}
