#-----------------------------------------------------------------------------
# Umicom C
# File: cmake/InputIdentity.cmake
#
# PURPOSE:
#   Inventory declared compiler inputs with bounded data-only receipts and explicit unknowns.
#
# AUTHOR AND ORGANISATION:
# Sammy Hegab
# Umicom Foundation
#
# LICENCE:
# MIT
#---------------------------------------------------------------------------

include_guard(GLOBAL)

# Hashes describe bytes, not authenticity. Names are encoded as JSON data and
# never included as CMake source. Refuse control bytes instead of accepting an
# ambiguous pathname or silently stripping characters during serialization.
function(umc_input_json_quote out value)
    foreach(code RANGE 1 31)
        string(ASCII ${code} control)
        string(FIND "${value}" "${control}" found)
        if(NOT found EQUAL -1)
            message(FATAL_ERROR "Control byte in configuration identity value")
        endif()
    endforeach()
    string(REPLACE "\\" "\\\\" value "${value}")
    string(REPLACE "\"" "\\\"" value "${value}")
    set(${out} "\"${value}\"" PARENT_SCOPE)
endfunction()

function(umc_input_regular_file file)
    if(NOT IS_ABSOLUTE "${file}" OR NOT EXISTS "${file}" OR IS_DIRECTORY "${file}")
        message(FATAL_ERROR "Identity input is not an existing absolute file: ${file}")
    endif()
    # A linked ancestor can hide an undeclared input tree. This check does not
    # claim protection against a concurrently replaced file or privileged edits.
    set(part "${file}")
    while(TRUE)
        if(IS_SYMLINK "${part}")
            message(FATAL_ERROR "Linked identity input is unsupported: ${part}")
        endif()
        cmake_path(GET part PARENT_PATH parent)
        if(parent STREQUAL part)
            break()
        endif()
        set(part "${parent}")
    endwhile()
endfunction()

function(umc_input_read_json path out)
    umc_input_regular_file("${path}")
    file(SIZE "${path}" size)
    if(size GREATER 16777216)
        message(FATAL_ERROR "Identity JSON exceeds 16 MiB")
    endif()
    file(READ "${path}" text)
    string(JSON kind ERROR_VARIABLE error TYPE "${text}")
    if(NOT error STREQUAL "NOTFOUND" OR NOT kind STREQUAL "OBJECT")
        message(FATAL_ERROR "Identity JSON must be one object: ${path}")
    endif()
    set(${out} "${text}" PARENT_SCOPE)
endfunction()


# Recipe and receipt contents are data, even when a flag or a path contains
# an @-delimited token. CMake file(CONFIGURE) would substitute that token.
# Only this explicit capture helper writes; the verifier does not call it.
function(umc_input_write_data path content)
    if(EXISTS "${path}")
        umc_input_regular_file("${path}")
        file(READ "${path}" existing)
        if(existing STREQUAL content)
            return()
        endif()
    endif()
    file(WRITE "${path}" "${content}")
endfunction()

function(umc_input_snapshot source binary recipe out)
    foreach(path IN ITEMS "${source}" "${binary}")
        if(NOT IS_ABSOLUTE "${path}" OR NOT IS_DIRECTORY "${path}")
            message(FATAL_ERROR "Identity roots must be existing absolute directories")
        endif()
    endforeach()
    if(source STREQUAL binary)
        message(FATAL_ERROR "Use an out-of-source build for input identity")
    endif()
    umc_input_read_json("${recipe}" recipe_json)
    file(SHA256 "${recipe}" recipe_hash)
    set(files "${source}/CMakeLists.txt")
    if(EXISTS "${source}/CMakePresets.json")
        list(APPEND files "${source}/CMakePresets.json")
    endif()
    # These are the compiler's declared source, fixture and artwork roots.
    # Documentation prose and generated build directories are not source inputs.
    foreach(folder IN ITEMS src include tools cmake tests examples docs/assets/branding)
        if(IS_SYMLINK "${source}/${folder}")
            message(FATAL_ERROR "Linked input root is unsupported: ${folder}")
        endif()
        file(GLOB_RECURSE entries LIST_DIRECTORIES TRUE "${source}/${folder}/*")
        foreach(entry IN LISTS entries)
            if(IS_SYMLINK "${entry}")
                message(FATAL_ERROR "Linked input is unsupported: ${entry}")
            endif()
            if(NOT IS_DIRECTORY "${entry}")
                list(APPEND files "${entry}")
            endif()
        endforeach()
    endforeach()
    list(REMOVE_DUPLICATES files)
    list(SORT files)
    list(LENGTH files file_count)
    if(file_count GREATER 4096)
        message(FATAL_ERROR "Declared compiler inventory exceeds 4096 files")
    endif()
    set(inventory "{}")
    foreach(file IN LISTS files)
        umc_input_regular_file("${file}")
        file(RELATIVE_PATH relative "${source}" "${file}")
        umc_input_json_quote(name "${relative}")
        file(SHA256 "${file}" hash)
        string(JSON inventory SET "${inventory}" "${relative}" "\"${hash}\"")
    endforeach()
    set(generated "{}")
    foreach(name IN ITEMS umicom-c.rc UmicomCConfig.cmake UmicomCConfigVersion.cmake)
        set(file "${binary}/${name}")
        if(EXISTS "${file}")
            umc_input_regular_file("${file}")
            file(SHA256 "${file}" hash)
            string(JSON generated SET "${generated}" "${name}" "\"${hash}\"")
        else()
            string(JSON generated SET "${generated}" "${name}" null)
        endif()
    endforeach()
    set(tool_hashes "{}")
    foreach(tool IN ITEMS CMAKE_COMMAND CMAKE_C_COMPILER CMAKE_MAKE_PROGRAM CMAKE_LINKER CMAKE_AR
        CMAKE_RANLIB CMAKE_RC_COMPILER CMAKE_TOOLCHAIN_FILE)
        string(JSON file ERROR_VARIABLE error GET "${recipe_json}" "${tool}")
        if(error STREQUAL "NOTFOUND" AND NOT file STREQUAL "")
            umc_input_regular_file("${file}")
            file(SHA256 "${file}" hash)
            string(JSON tool_hashes SET "${tool_hashes}" "${tool}" "\"${hash}\"")
        else()
            string(JSON tool_hashes SET "${tool_hashes}" "${tool}" null)
        endif()
    endforeach()
    set(snapshot "{}")
    foreach(field IN ITEMS source binary)
        umc_input_json_quote(value "${${field}}")
        string(JSON snapshot SET "${snapshot}" "${field}" "${value}")
    endforeach()
    string(JSON snapshot SET "${snapshot}" schema "\"umicom.c.configuration-inputs\"")
    string(JSON snapshot SET "${snapshot}" version 1)
    string(JSON snapshot SET "${snapshot}" recipeSha256 "\"${recipe_hash}\"")
    string(JSON snapshot SET "${snapshot}" recipe "${recipe_json}")
    string(JSON snapshot SET "${snapshot}" sourceFiles "${inventory}")
    string(JSON snapshot SET "${snapshot}" generatedFiles "${generated}")
    string(JSON snapshot SET "${snapshot}" toolFiles "${tool_hashes}")
    string(JSON snapshot SET "${snapshot}" compilationVerified false)
    string(JSON snapshot SET "${snapshot}" sourceStableDuringBuildVerified false)
    string(JSON snapshot SET "${snapshot}" unknownInputs
        "[\"Compiler-loaded libraries, plugins and implicit SDK headers/libraries\",\"Environment and launcher side effects\",\"Generated build rules, object files and final binaries\",\"Concurrent input changes between observations; later cache edits require reconfiguration\"]")
    set(${out} "${snapshot}" PARENT_SCOPE)
endfunction()

function(umc_input_identity mode source binary recipe receipt)
    if(NOT mode STREQUAL "capture" AND NOT mode STREQUAL "verify")
        message(FATAL_ERROR "Unknown input identity operation")
    endif()
    if(NOT IS_ABSOLUTE "${receipt}")
        message(FATAL_ERROR "Receipt path must be absolute")
    endif()
    if(mode STREQUAL "verify")
        # Read first: missing and malformed receipts are refusals, never an
        # invitation to overwrite evidence. Verification has no write operation.
        umc_input_read_json("${receipt}" recorded)
    endif()
    umc_input_snapshot("${source}" "${binary}" "${recipe}" current)
    if(mode STREQUAL "capture")
        if(IS_SYMLINK "${receipt}")
            message(FATAL_ERROR "Refusing a linked receipt output")
        endif()
        # Retain the substituting writer for review; literal writing preserves
        # data such as an @NAME@ token in a compiler argument.
        if(FALSE)
            file(CONFIGURE OUTPUT "${receipt}" CONTENT "${current}\n" @ONLY)
        endif()
        umc_input_write_data("${receipt}" "${current}\n")
    else()
        # The capture format is canonical. Byte equality rejects trailing
        # content and duplicate JSON keys that a permissive parser might ignore.
        set(expected "${current}\n")
        if(NOT recorded STREQUAL expected)
            message(FATAL_ERROR "Configured inputs changed or receipt is invalid. Inspect differences; reconfigure and rebuild before qualification.")
        endif()
    endif()
endfunction()
