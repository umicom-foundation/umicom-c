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
#include "umicom_c/int_input.h"
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

/* Format 1 remains byte-for-byte stable for earlier arithmetic programs.
 * Format 2 identifies the comparison extension so older runtimes refuse it
 * before interpreting records. Layout and reserved flags stay unchanged. */
/* Local operands use format 3. Inspect the whole program so an earlier comparison cannot hide a later local operation; legacy formats keep their bytes. The predecessor remains disabled for review. */
#if 0
static uint8_t program_wire_version(const UmcInstruction *program, size_t count)
{
    for (size_t i = 0U; i < count; ++i)
        if (program[i].opcode >= UMC_LESS && program[i].opcode <= UMC_NOT_EQUAL) return 2U;
    return 1U;
}

#endif

/* Select format 4 for branches regardless of earlier local or comparison records, while preserving each older minimum profile. The predecessor remains disabled for review. */
#if 0
static uint8_t program_wire_version(const UmcInstruction *program, size_t count)
{
    uint8_t version = 1U;
    for (size_t i = 0U; i < count; ++i) {
        if (program[i].opcode == UMC_LOAD_LOCAL || program[i].opcode == UMC_STORE_LOCAL)
            return 3U;
        if (program[i].opcode >= UMC_LESS && program[i].opcode <= UMC_NOT_EQUAL)
            version = 2U;
    }
    return version;
}

#endif

/* Scan all records for the highest required profile; an earlier branch cannot hide a later remainder. The previous implementation is retained for engineering review. */
#if 0
static uint8_t program_wire_version(const UmcInstruction *program, size_t count)
{
    uint8_t version = 1U;
    for (size_t i = 0U; i < count; ++i) {
        if (program[i].opcode == UMC_JUMP || program[i].opcode == UMC_JUMP_IF_ZERO)
            return 4U;
        if (program[i].opcode == UMC_LOAD_LOCAL || program[i].opcode == UMC_STORE_LOCAL)
            version = 3U;
        if (version < 2U && program[i].opcode >= UMC_LESS && program[i].opcode <= UMC_NOT_EQUAL)
            version = 2U;
    }
    return version;
}
#endif
/* Select format 6 only for the new operation family, scanning past lower requirements without changing older canonical encodings. The previous implementation is retained for engineering review. */
#if 0
static uint8_t program_wire_version(const UmcInstruction *program, size_t count)
{
    uint8_t version = 1U;
    for (size_t i = 0U; i < count; ++i) {
        if (program[i].opcode == UMC_REMAINDER) return 5U;
        if (program[i].opcode == UMC_JUMP || program[i].opcode == UMC_JUMP_IF_ZERO)
            version = 4U;
        else if (version < 3U &&
            (program[i].opcode == UMC_LOAD_LOCAL || program[i].opcode == UMC_STORE_LOCAL))
            version = 3U;
        else if (version < 2U &&
            program[i].opcode >= UMC_LESS && program[i].opcode <= UMC_NOT_EQUAL)
            version = 2U;
    }
    return version;
}
#endif
/* Select the maximum required format across all records; older bitwise records must not hide later shifts. The former implementation is retained for owner review. */
#if 0
static uint8_t program_wire_version(const UmcInstruction *program, size_t count)
{
    uint8_t version = 1U;
    for (size_t i = 0U; i < count; ++i) {
        if (program[i].opcode == UMC_BITWISE_AND ||
            program[i].opcode == UMC_BITWISE_XOR || program[i].opcode == UMC_BITWISE_OR)
            return 6U;
        if (program[i].opcode == UMC_REMAINDER) version = 5U;
        else if (version < 4U &&
            (program[i].opcode == UMC_JUMP || program[i].opcode == UMC_JUMP_IF_ZERO))
            version = 4U;
        else if (version < 3U &&
            (program[i].opcode == UMC_LOAD_LOCAL || program[i].opcode == UMC_STORE_LOCAL))
            version = 3U;
        else if (version < 2U &&
            program[i].opcode >= UMC_LESS && program[i].opcode <= UMC_NOT_EQUAL)
            version = 2U;
    }
    return version;
}

#endif
static uint8_t program_wire_version(const UmcInstruction *program, size_t count)
{
    uint8_t version = 1U;
    for (size_t i = 0U; i < count; ++i) {
        if (program[i].opcode == UMC_SHIFT_LEFT || program[i].opcode == UMC_SHIFT_RIGHT)
            return 7U;
        if (program[i].opcode == UMC_BITWISE_AND ||
            program[i].opcode == UMC_BITWISE_XOR || program[i].opcode == UMC_BITWISE_OR)
            version = 6U;
        if (version < 5U && program[i].opcode == UMC_REMAINDER) version = 5U;
        else if (version < 4U &&
            (program[i].opcode == UMC_JUMP || program[i].opcode == UMC_JUMP_IF_ZERO))
            version = 4U;
        else if (version < 3U &&
            (program[i].opcode == UMC_LOAD_LOCAL || program[i].opcode == UMC_STORE_LOCAL))
            version = 3U;
        else if (version < 2U &&
            program[i].opcode >= UMC_LESS && program[i].opcode <= UMC_NOT_EQUAL)
            version = 2U;
    }
    return version;
}

/* Select the minimum required wire profile while retaining old arithmetic artifact bytes. The former implementation is retained for review. */
#if 0
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
#endif

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
    output[4] = program_wire_version(program, count);
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

/* Admit both explicit profiles and reject opcode/version mismatches without publishing a count. The former implementation is retained for review. */
#if 0
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
#endif

/* Accept the additive local-slot wire profile, then retain whole-program validation and minimum-profile checks before publishing a count. The predecessor remains disabled for review. */
#if 0
UmcStatus umc_decode(const uint8_t *bytes, size_t length,
    UmcInstruction *output, size_t capacity, size_t *count, UmcDiagnostic *diagnostic)
{
    if (count != NULL) *count = 0U;
    if (bytes == NULL || output == NULL || count == NULL) {
        return umc_fail(diagnostic, UMC_INVALID_ARGUMENT, 0U);
    }
    if (length < UMC_BYTECODE_HEADER_SIZE || length > UMC_BYTECODE_LIMIT ||
        bytes[0] != 'U' || bytes[1] != 'M' || bytes[2] != 'C' || bytes[3] != 'B' ||
        (bytes[4] != 1U && bytes[4] != 2U) || bytes[5] != 0U) {
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
    /* Require the canonical profile even after complete instruction admission.
     * A comparison mislabeled as format 1 must never reach an old consumer. */
    if (bytes[4] != program_wire_version(output, records))
        return umc_fail(diagnostic, UMC_BAD_BYTECODE, 0U);
    *count = records;
    return UMC_OK;
}

#endif

/* Admit the forward-branch wire profile only after complete graph validation and canonical minimum-version checking. The predecessor remains disabled for review. */
#if 0
UmcStatus umc_decode(const uint8_t *bytes, size_t length,
    UmcInstruction *output, size_t capacity, size_t *count, UmcDiagnostic *diagnostic)
{
    if (count != NULL) *count = 0U;
    if (bytes == NULL || output == NULL || count == NULL) {
        return umc_fail(diagnostic, UMC_INVALID_ARGUMENT, 0U);
    }
    if (length < UMC_BYTECODE_HEADER_SIZE || length > UMC_BYTECODE_LIMIT ||
        bytes[0] != 'U' || bytes[1] != 'M' || bytes[2] != 'C' || bytes[3] != 'B' ||
        (bytes[4] != 1U && bytes[4] != 2U && bytes[4] != 3U) || bytes[5] != 0U) {
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
    /* Require the canonical profile even after complete instruction admission.
     * A comparison mislabeled as format 1 must never reach an old consumer. */
    if (bytes[4] != program_wire_version(output, records))
        return umc_fail(diagnostic, UMC_BAD_BYTECODE, 0U);
    *count = records;
    return UMC_OK;
}

#endif

/* Admit format 5 only after complete structural validation and the canonical minimum-format check. The predecessor remains disabled for review. */
#if 0
UmcStatus umc_decode(const uint8_t *bytes, size_t length,
    UmcInstruction *output, size_t capacity, size_t *count, UmcDiagnostic *diagnostic)
{
    if (count != NULL) *count = 0U;
    if (bytes == NULL || output == NULL || count == NULL) {
        return umc_fail(diagnostic, UMC_INVALID_ARGUMENT, 0U);
    }
    if (length < UMC_BYTECODE_HEADER_SIZE || length > UMC_BYTECODE_LIMIT ||
        bytes[0] != 'U' || bytes[1] != 'M' || bytes[2] != 'C' || bytes[3] != 'B' ||
        (bytes[4] != 1U && bytes[4] != 2U && bytes[4] != 3U && bytes[4] != 4U) || bytes[5] != 0U) {
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
    /* Require the canonical profile even after complete instruction admission.
     * A comparison mislabeled as format 1 must never reach an old consumer. */
    if (bytes[4] != program_wire_version(output, records))
        return umc_fail(diagnostic, UMC_BAD_BYTECODE, 0U);
    *count = records;
    return UMC_OK;
}
#endif

/* Admit the additive format-6 envelope while retaining whole-program validation and exact minimum-version checks. The previous implementation remains for review. */
#if 0
UmcStatus umc_decode(const uint8_t *bytes, size_t length,
    UmcInstruction *output, size_t capacity, size_t *count, UmcDiagnostic *diagnostic)
{
    if (count != NULL) *count = 0U;
    if (bytes == NULL || output == NULL || count == NULL) {
        return umc_fail(diagnostic, UMC_INVALID_ARGUMENT, 0U);
    }
    if (length < UMC_BYTECODE_HEADER_SIZE || length > UMC_BYTECODE_LIMIT ||
        bytes[0] != 'U' || bytes[1] != 'M' || bytes[2] != 'C' || bytes[3] != 'B' ||
        (bytes[4] != 1U && bytes[4] != 2U && bytes[4] != 3U && bytes[4] != 4U && bytes[4] != 5U) || bytes[5] != 0U) {
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
    /* Require the canonical profile even after complete instruction admission.
     * A comparison mislabeled as format 1 must never reach an old consumer. */
    if (bytes[4] != program_wire_version(output, records))
        return umc_fail(diagnostic, UMC_BAD_BYTECODE, 0U);
    *count = records;
    return UMC_OK;
}
#endif
static UmcStatus decode_bytecode(const uint8_t *bytes, size_t length,
    UmcInstruction *output, size_t capacity, size_t *count, UmcDiagnostic *diagnostic,
    uint8_t expected_flags)
{
    if (count != NULL) *count = 0U;
    if (bytes == NULL || output == NULL || count == NULL) {
        return umc_fail(diagnostic, UMC_INVALID_ARGUMENT, 0U);
    }
/* Admit format 7 while retaining full structural and canonical-format validation. The former implementation is retained for owner review. */
#if 0
    if (length < UMC_BYTECODE_HEADER_SIZE || length > UMC_BYTECODE_LIMIT ||
        bytes[0] != 'U' || bytes[1] != 'M' || bytes[2] != 'C' || bytes[3] != 'B' ||
        (bytes[4] != 1U && bytes[4] != 2U && bytes[4] != 3U && bytes[4] != 4U && bytes[4] != 5U && bytes[4] != 6U) || bytes[5] != 0U) {
        return umc_fail(diagnostic, UMC_BAD_BYTECODE, 0U);
    }

#endif
    /* The ordinary decoder still requires zero. The input-aware boundary
     * explicitly asks for the one required-input flag before binding. */
    if (length < UMC_BYTECODE_HEADER_SIZE || length > UMC_BYTECODE_LIMIT ||
        bytes[0] != 'U' || bytes[1] != 'M' || bytes[2] != 'C' || bytes[3] != 'B' ||
        (bytes[4] != 1U && bytes[4] != 2U && bytes[4] != 3U && bytes[4] != 4U && bytes[4] != 5U && bytes[4] != 6U && bytes[4] != 7U) || bytes[5] != expected_flags) {
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
    /* Require the canonical profile even after complete instruction admission.
     * A comparison mislabeled as format 1 must never reach an old consumer. */
    if (bytes[4] != program_wire_version(output, records))
        return umc_fail(diagnostic, UMC_BAD_BYTECODE, 0U);
    *count = records;
    return UMC_OK;
}

UmcStatus umc_decode(const uint8_t *bytes, size_t length,
    UmcInstruction *output, size_t capacity, size_t *count, UmcDiagnostic *diagnostic)
{
    return decode_bytecode(bytes, length, output, capacity, count, diagnostic, 0U);
}

UmcStatus umc_bind_int_input_bytecode(const uint8_t *bytes, size_t length, int32_t value,
    UmcInstruction *output, size_t capacity, size_t *count, UmcDiagnostic *diagnostic)
{
    if (count == NULL) return umc_fail(diagnostic, UMC_INVALID_ARGUMENT, 0U);
    *count = 0U;
    if (output == NULL) return umc_fail(diagnostic, UMC_INVALID_ARGUMENT, 0U);
    UmcInstruction staged[UMC_PROGRAM_LIMIT];
    size_t decoded = 0U;
    const UmcStatus status = decode_bytecode(bytes, length, staged, UMC_PROGRAM_LIMIT,
        &decoded, diagnostic, UMC_INT_INPUT_BYTECODE_FLAG);
    if (status != UMC_OK) return status;
    if (decoded < 3U || staged[0].opcode != UMC_PUSH || staged[0].operand != 0 ||
        staged[1].opcode != UMC_STORE_LOCAL || staged[1].operand != 0)
        return umc_fail(diagnostic, UMC_BAD_BYTECODE, 0U);
    if (capacity < decoded) return umc_fail(diagnostic, UMC_BUFFER_TOO_SMALL, 0U);

    /* Changing only the int32 operand of a validated PUSH preserves stack,
     * local and control-flow admission. The template is never edited. */
    staged[0].operand = value;
    for (size_t i = 0U; i < decoded; ++i) {
        output[i].opcode = staged[i].opcode;
        output[i].operand = staged[i].operand;
        output[i].source_offset = staged[i].source_offset;
    }
    *count = decoded;
    return UMC_OK;
}
