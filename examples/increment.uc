/*-----------------------------------------------------------------------------
 * Umicom C
 * File: examples/increment.uc
 *
 * PURPOSE:
 *   Update initialized integer locals with standalone prefix and postfix operations.
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
    int value = 40;
    ++value;
    value++;
    --value;
    value--;
    ++value;
    value++;
    return value;
}
