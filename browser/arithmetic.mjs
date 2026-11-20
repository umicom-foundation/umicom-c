/*-----------------------------------------------------------------------------
 * Umicom C
 * File: browser/arithmetic.mjs
 *
 * PURPOSE:
 *   Execute bounded integer arithmetic, local state and forward branches in a standard browser.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

const instruction = Object.freeze({
    push: 1, add: 2, subtract: 3, multiply: 4, divide: 5, negate: 6, return: 7,
    less: 8, lessEqual: 9, greater: 10, greaterEqual: 11, equal: 12, notEqual: 13,
    loadLocal: 14, storeLocal: 15,
    jump: 16, jumpIfZero: 17,
    remainder: 18, bitwiseAnd: 19, bitwiseXor: 20, bitwiseOr: 21,
    shiftLeft: 22, shiftRight: 23,
});
const magic = 0x554d4342;
const headerBytes = 8;
const recordBytes = 9;
const programLimit = 256;
const stackLimit = 64;
const localLimit = 32;
const sourceLimit = 65536;
const minimumInteger = -2147483648n;
const maximumInteger = 2147483647n;

function refusal(error, sourceOffset = 0) {
    return { ok: false, error, sourceOffset };
}

/* Retain the former linear verifier for review. The current verifier shares
 * stack and local rules with the forward-branch profile and checks all joins. */
if (false) {
    function validateLinear(view, count, format) {
        let depth = 0;
        let requiredFormat = 1;
        const initialized = format === 3 ? new Uint8Array(localLimit) : null;
        for (let index = 0; index < count; ++index) {
            const record = headerBytes + index * recordBytes;
            const opcode = view.getUint8(record);
            const argument = view.getInt32(record + 1, true);
            const offset = view.getUint32(record + 5, true);
            const local = opcode === instruction.loadLocal || opcode === instruction.storeLocal;
            if (offset > sourceLimit || (opcode !== instruction.push && !local && argument !== 0) ||
                (local && (argument < 0 || argument >= localLimit))) {
                return refusal("bad-bytecode", offset);
            }
            switch (opcode) {
            case instruction.push:
                if (depth === stackLimit) return refusal("stack-overflow", offset);
                ++depth;
                break;
            case instruction.add:
            case instruction.subtract:
            case instruction.multiply:
            case instruction.divide:
                if (depth < 2) return refusal("bad-bytecode", offset);
                --depth;
                break;
            case instruction.less:
            case instruction.lessEqual:
            case instruction.greater:
            case instruction.greaterEqual:
            case instruction.equal:
            case instruction.notEqual:
                if (format < 2 || depth < 2) return refusal("bad-bytecode", offset);
                requiredFormat = Math.max(requiredFormat, 2);
                --depth;
                break;
            case instruction.loadLocal:
                // A slot must have a preceding store in the same linear program.
                if (initialized === null || initialized[argument] === 0) return refusal("bad-bytecode", offset);
                if (depth === stackLimit) return refusal("stack-overflow", offset);
                requiredFormat = 3;
                ++depth;
                break;
            case instruction.storeLocal:
                if (initialized === null || depth === 0) return refusal("bad-bytecode", offset);
                initialized[argument] = 1;
                requiredFormat = 3;
                --depth;
                break;
            case instruction.negate:
                if (depth === 0) return refusal("bad-bytecode", offset);
                break;
            case instruction.return:
                if (depth !== 1 || index + 1 !== count) return refusal("bad-bytecode", offset);
                // Native encoding always uses the minimum format required by the program.
                if (requiredFormat !== format) return refusal("bad-bytecode");
                return null;
            default:
                return refusal("bad-bytecode", offset);
            }
            if (index + 1 === count) return refusal("bad-bytecode", offset);
        }
        return refusal("bad-bytecode");
    }
}

function mergeState(depths, locals, target, depth, initialized) {
    if (depths[target] === 255) {
        depths[target] = depth;
        locals[target] = initialized;
    } else if (depths[target] !== depth) {
        return false;
    } else {
        // A local can be loaded after a join only when every edge stores it.
        locals[target] &= initialized;
    }
    return true;
}

function validate(view, count, format) {
    const depths = new Uint8Array(count).fill(255);
    const localStates = new Uint32Array(count);
    depths[0] = 0;
    let requiredFormat = 1;
    for (let index = 0; index < count; ++index) {
        const record = headerBytes + index * recordBytes;
        const opcode = view.getUint8(record);
        const argument = view.getInt32(record + 1, true);
        const offset = view.getUint32(record + 5, true);
        const local = opcode === instruction.loadLocal || opcode === instruction.storeLocal;
        const branch = opcode === instruction.jump || opcode === instruction.jumpIfZero;
        let depth = depths[index];
        let initialized = localStates[index];
        if (offset > sourceLimit || depth === 255 ||
            (opcode !== instruction.push && !local && !branch && argument !== 0) ||
            (local && (format < 3 || argument < 0 || argument >= localLimit)) ||
            (branch && (format < 4 || argument <= index || argument >= count))) {
            return refusal("bad-bytecode", offset);
        }
        if (local) requiredFormat = Math.max(requiredFormat, 3);
        if (branch) requiredFormat = Math.max(requiredFormat, 4);
        switch (opcode) {
        case instruction.loadLocal:
            if ((initialized & (1 << argument)) === 0) return refusal("bad-bytecode", offset);
            if (depth === stackLimit) return refusal("stack-overflow", offset);
            ++depth;
            break;
        case instruction.storeLocal:
            if (depth === 0) return refusal("bad-bytecode", offset);
            initialized |= 1 << argument;
            --depth;
            break;
        case instruction.push:
            if (depth === stackLimit) return refusal("stack-overflow", offset);
            ++depth;
            break;
        case instruction.less:
        case instruction.lessEqual:
        case instruction.greater:
        case instruction.greaterEqual:
        case instruction.equal:
        case instruction.notEqual:
            if (format < 2 || depth < 2) return refusal("bad-bytecode", offset);
            requiredFormat = Math.max(requiredFormat, 2);
            --depth;
            break;
        case instruction.add:
        case instruction.subtract:
        case instruction.multiply:
        case instruction.divide:
            if (depth < 2) return refusal("bad-bytecode", offset);
            --depth;
            break;
        case instruction.remainder:
        case instruction.bitwiseAnd:
        case instruction.bitwiseXor:
        case instruction.bitwiseOr:
        case instruction.shiftLeft:
        case instruction.shiftRight: {
            // The native encoder uses the greatest requirement across the
            // whole program, including the operations on untaken paths.
            const needed = opcode === instruction.remainder ? 5 :
                opcode >= instruction.shiftLeft ? 7 : 6;
            if (format < needed || depth < 2) return refusal("bad-bytecode", offset);
            requiredFormat = Math.max(requiredFormat, needed);
            --depth;
            break;
        }
        case instruction.negate:
            if (depth === 0) return refusal("bad-bytecode", offset);
            break;
        case instruction.jumpIfZero:
            if (depth === 0) return refusal("bad-bytecode", offset);
            --depth;
            break;
        case instruction.jump:
            break;
        case instruction.return:
            if (depth !== 1 || index + 1 !== count) return refusal("bad-bytecode", offset);
            // Native encoding always uses the minimum format required by the program.
            return requiredFormat === format ? null : refusal("bad-bytecode");
        default:
            return refusal("bad-bytecode", offset);
        }
        if (branch && !mergeState(depths, localStates, argument, depth, initialized))
            return refusal("bad-bytecode", offset);
        if (opcode !== instruction.jump &&
            (index + 1 === count || !mergeState(depths, localStates, index + 1, depth, initialized)))
            return refusal("bad-bytecode", offset);
    }
    return refusal("bad-bytecode");
}

/** Verify one complete artifact before evaluating any instruction. BigInt
 * preserves intermediate and signed comparison values across native bounds. */
function runProfile(artifact, fuel, highestFormat, input) {
    if (!(artifact instanceof ArrayBuffer) || !Number.isInteger(fuel) || fuel < 0 || fuel > programLimit) {
        return refusal("invalid-request");
    }
    let view;
    try {
        view = new DataView(artifact);
    } catch {
        return refusal("bad-bytecode");
    }
    const length = view.byteLength;
    if (length < headerBytes || length > headerBytes + programLimit * recordBytes) {
        return refusal("bad-bytecode");
    }
    const hasInput = input !== undefined;
    // Old profiles still require zero flags. Only the explicit input entry
    // accepts the flag that marks the native compiler's unbound first push.
    if (view.getUint32(0) !== magic || view.getUint8(5) !== (hasInput ? 1 : 0)) {
        return refusal("bad-bytecode");
    }
    const format = view.getUint8(4);
    if (format < 1 || format > highestFormat) {
        return refusal("unsupported-profile");
    }
    const count = view.getUint16(6, true);
    if (count === 0 || count > programLimit || length !== headerBytes + count * recordBytes) {
        return refusal("bad-bytecode");
    }
    const invalid = validate(view, count, format);
    if (invalid !== null) return invalid;
    if (hasInput && (count < 3 || view.getUint8(headerBytes) !== instruction.push ||
        view.getInt32(headerBytes + 1, true) !== 0 ||
        view.getUint8(headerBytes + recordBytes) !== instruction.storeLocal ||
        view.getInt32(headerBytes + recordBytes + 1, true) !== 0)) {
        return refusal("bad-bytecode");
    }

    const stack = [];
    // Every request has separate, typed local slots. Validation above admits
    // their bounds and initialization before the first instruction executes.
    const locals = format >= 3 ? new Array(localLimit) : null;
    let executed = 0;
    for (let index = 0; index < count; ++index) {
        const record = headerBytes + index * recordBytes;
        const offset = view.getUint32(record + 5, true);
        if (executed >= fuel) return refusal("fuel-exhausted", offset);
        ++executed;
        const opcode = view.getUint8(record);
        if (opcode === instruction.return) {
            return { ok: true, value: Number(stack[0]), instructions: executed };
        }
        if (opcode === instruction.push) {
            const value = hasInput && index === 0 ? input : view.getInt32(record + 1, true);
            stack.push(BigInt(value));
            continue;
        }
        if (opcode === instruction.loadLocal) {
            stack.push(locals[view.getInt32(record + 1, true)]);
            continue;
        }
        if (opcode === instruction.storeLocal) {
            locals[view.getInt32(record + 1, true)] = stack.pop();
            continue;
        }
        if (opcode === instruction.jump || opcode === instruction.jumpIfZero) {
            // The admission pass already proved every destination moves forward.
            if (opcode === instruction.jump || stack.pop() === 0n)
                index = view.getInt32(record + 1, true) - 1;
            continue;
        }
        const right = stack.pop();
        let value;
        if (opcode === instruction.negate) {
            value = -right;
        } else {
            const left = stack.pop();
            switch (opcode) {
            case instruction.add: value = left + right; break;
            case instruction.subtract: value = left - right; break;
            case instruction.multiply: value = left * right; break;
            case instruction.divide:
                if (right === 0n) return refusal("arithmetic-error", offset);
                value = left / right;
                break;
            case instruction.remainder:
                if (right === 0n || (left === minimumInteger && right === -1n))
                    return refusal("arithmetic-error", offset);
                value = left % right;
                break;
            // Match the native interpretation of the low 32 bits as signed.
            case instruction.bitwiseAnd: value = BigInt.asIntN(32, left & right); break;
            case instruction.bitwiseXor: value = BigInt.asIntN(32, left ^ right); break;
            case instruction.bitwiseOr: value = BigInt.asIntN(32, left | right); break;
            case instruction.shiftLeft:
            case instruction.shiftRight:
                if (left < 0n || right < 0n || right > 31n)
                    return refusal("arithmetic-error", offset);
                // BigInt keeps left shifts exact until the common signed
                // result bound is checked below.
                value = opcode === instruction.shiftLeft ? left << right : left >> right;
                break;
            case instruction.less: value = left < right ? 1n : 0n; break;
            case instruction.lessEqual: value = left <= right ? 1n : 0n; break;
            case instruction.greater: value = left > right ? 1n : 0n; break;
            case instruction.greaterEqual: value = left >= right ? 1n : 0n; break;
            case instruction.equal: value = left === right ? 1n : 0n; break;
            case instruction.notEqual: value = left !== right ? 1n : 0n; break;
            }
        }
        if (value < minimumInteger || value > maximumInteger) {
            return refusal("arithmetic-error", offset);
        }
        stack.push(value);
    }
    return refusal("bad-bytecode");
}

/** Preserve the format-1 entry point for arithmetic-only hosts. */
export function runUmicomArithmetic(artifact, fuel = programLimit) {
    return runProfile(artifact, fuel, 1);
}

/** Admit native formats 1 and 2. Comparisons produce the integer zero or one. */
export function runUmicomComparison(artifact, fuel = programLimit) {
    return runProfile(artifact, fuel, 2);
}

/** Admit native formats 1 through 3, including initialized signed integer locals. */
export function runUmicomLocals(artifact, fuel = programLimit) {
    return runProfile(artifact, fuel, 3);
}

/** Admit native formats 1 through 4, including verified forward branches. */
export function runUmicomBranches(artifact, fuel = programLimit) {
    return runProfile(artifact, fuel, 4);
}

/** Admit all current native wire formats, including checked integer operators. */
export function runUmicomIntegers(artifact, fuel = programLimit) {
    return runProfile(artifact, fuel, 7);
}

/** Bind one signed integer to an input-aware artifact without editing it. */
export function runUmicomIntInput(artifact, input, fuel = programLimit) {
    if (!Number.isInteger(input) || input < Number(minimumInteger) || input > Number(maximumInteger)) {
        return refusal("invalid-request");
    }
    return runProfile(artifact, fuel, 7, input);
}
