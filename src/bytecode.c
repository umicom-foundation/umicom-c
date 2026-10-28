/*-----------------------------------------------------------------------------
 * Umicom C
 * File: src/bytecode.c
 *
 * PURPOSE:
 *   Encode portable instruction bytes and reject malformed or noncanonical artifacts.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/
#include "internal.h"
#include <limits.h>

static void put_u32(uint8_t *output, uint32_t value)
{
    for (size_t i = 0U; i < 4U; ++i) {
        output[i] = (uint8_t)(value & UINT32_C(255));
        value >>= 8U;
    }
}

static uint32_t get_u32(const uint8_t *input)
{
    uint32_t value = 0U;
    for (size_t i = 0U; i < 4U; ++i) {
        value |= (uint32_t)input[i] << (unsigned int)(8U * i);
    }
    return value;
}

UmcStatus umc_encode(const UmcInstruction *program, size_t count,
    uint8_t *output, size_t capacity, size_t *size, UmcDiagnostic *diagnostic)
{
    if (size != NULL) *size = 0U;
    if (output == NULL || size == NULL) return umc_fail(diagnostic, UMC_INVALID_ARGUMENT, 0U);
    const UmcStatus status = umc_validate(program, count, diagnostic);
    if (status != UMC_OK) return status;
    const size_t needed = UMC_BYTECODE_HEADER_SIZE + count * UMC_BYTECODE_RECORD_SIZE;
    if (capacity < needed) return umc_fail(diagnostic, UMC_BUFFER_TOO_SMALL, 0U);
    output[0] = 'U'; output[1] = 'M'; output[2] = 'C'; output[3] = 'B';
    output[4] = 1U; /* Wire-format version, independent from C structure layout. */
    output[5] = 0U; /* Reserved flags must remain zero. */
    output[6] = (uint8_t)(count & 255U);
    output[7] = (uint8_t)(count >> 8U);
    for (size_t i = 0U; i < count; ++i) {
        uint8_t *record = output + UMC_BYTECODE_HEADER_SIZE + i * UMC_BYTECODE_RECORD_SIZE;
        record[0] = (uint8_t)program[i].opcode;
        put_u32(record + 1U, (uint32_t)program[i].operand);
        put_u32(record + 5U, (uint32_t)program[i].source_offset);
    }
    *size = needed;
    return UMC_OK;
}

UmcStatus umc_decode(const uint8_t *bytes, size_t length,
    UmcInstruction *output, size_t capacity, size_t *count, UmcDiagnostic *diagnostic)
{
    if (count != NULL) *count = 0U;
    if (bytes == NULL || output == NULL || count == NULL) {
        return umc_fail(diagnostic, UMC_INVALID_ARGUMENT, 0U);
    }
    if (length < UMC_BYTECODE_HEADER_SIZE || length > UMC_BYTECODE_LIMIT ||
        bytes[0] != 'U' || bytes[1] != 'M' || bytes[2] != 'C' || bytes[3] != 'B' ||
        bytes[4] != 1U || bytes[5] != 0U) {
        return umc_fail(diagnostic, UMC_BAD_BYTECODE, 0U);
    }
    const size_t records = (size_t)bytes[6] | ((size_t)bytes[7] << 8U);
    if (records == 0U || records > UMC_PROGRAM_LIMIT ||
        length != UMC_BYTECODE_HEADER_SIZE + records * UMC_BYTECODE_RECORD_SIZE) {
        return umc_fail(diagnostic, UMC_BAD_BYTECODE, 0U);
    }
    if (capacity < records) return umc_fail(diagnostic, UMC_BUFFER_TOO_SMALL, 0U);
    for (size_t i = 0U; i < records; ++i) {
        const uint8_t *record = bytes + UMC_BYTECODE_HEADER_SIZE + i * UMC_BYTECODE_RECORD_SIZE;
        const uint32_t bits = get_u32(record + 1U);
        output[i].opcode = (UmcOpcode)record[0];
        /* Avoid an implementation-defined unsigned-to-signed conversion when
         * the serialized two's-complement operand has its high bit set. */
        output[i].operand = bits <= INT32_MAX ? (int32_t)bits :
            -((int32_t)(UINT32_MAX - bits)) - 1;
        output[i].source_offset = (size_t)get_u32(record + 5U);
    }
    const UmcStatus status = umc_validate(output, records, diagnostic);
    if (status != UMC_OK) return status;
    *count = records;
    return UMC_OK;
}
