# M2-18: build the libc runtime — compile each runtime/src/*.c via wvmcc -c,
# bundle the .wasm objects into libc.a via wasmvm-ar.
#
# Source list grows as M2-7..M2-17 land. Today only errno.c is wired.

find_program(WASMVM_AR wasmvm-ar
    DOC "Path to the wasmvm-ar archiver (from the WasmVM project)")
if(NOT WASMVM_AR)
    message(FATAL_ERROR
        "wasmvm-ar not found on PATH. Install WasmVM, or pass\n"
        "  cmake -DWASMVM_AR=/path/to/wasmvm-ar ...")
endif()
message(STATUS "Using wasmvm-ar: ${WASMVM_AR}")

set(RUNTIME_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(RUNTIME_BUILD_DIR   ${CMAKE_CURRENT_BINARY_DIR})
set(RUNTIME_LIB_DIR     ${RUNTIME_BUILD_DIR}/lib)
file(MAKE_DIRECTORY ${RUNTIME_LIB_DIR})

# Symlink (or copy) the source include/ into the build tree so the
# resulting layout matches what `--sysroot=<dir>` expects: <dir>/include/
# and <dir>/lib/.
if(NOT EXISTS ${RUNTIME_BUILD_DIR}/include)
    file(CREATE_LINK ${RUNTIME_INCLUDE_DIR} ${RUNTIME_BUILD_DIR}/include
         SYMBOLIC RESULT _link_rc)
    if(NOT _link_rc EQUAL 0)
        # Symlink failed (Windows w/o admin, exotic FS) — fall back to copy.
        file(COPY ${RUNTIME_INCLUDE_DIR} DESTINATION ${RUNTIME_BUILD_DIR})
    endif()
endif()

# TUs that have actually been written. Add new entries as M2-7..M2-17 land.
set(RUNTIME_SOURCES
    src/errno.c
    src/string.c
    src/unistd_fcntl.c
    src/stdlib_core.c
    src/malloc.c
)

set(RUNTIME_OBJECTS)
foreach(src ${RUNTIME_SOURCES})
    get_filename_component(name ${src} NAME_WE)
    set(obj ${RUNTIME_LIB_DIR}/${name}.wasm)
    add_custom_command(
        OUTPUT ${obj}
        COMMAND $<TARGET_FILE:wvmcc>
                -c
                -isystem ${RUNTIME_INCLUDE_DIR}
                ${CMAKE_CURRENT_SOURCE_DIR}/${src}
                -o ${obj}
        DEPENDS wvmcc ${CMAKE_CURRENT_SOURCE_DIR}/${src}
        COMMENT "Compiling libc TU ${src}"
        VERBATIM)
    list(APPEND RUNTIME_OBJECTS ${obj})
endforeach()

# Bundle all object files into libc.a via wasmvm-ar.
add_custom_command(
    OUTPUT ${RUNTIME_LIB_DIR}/libc.a
    COMMAND ${WASMVM_AR} create ${RUNTIME_LIB_DIR}/libc.a ${RUNTIME_OBJECTS}
    DEPENDS ${RUNTIME_OBJECTS}
    COMMENT "Archiving libc.a"
    VERBATIM)

add_custom_target(runtime ALL DEPENDS ${RUNTIME_LIB_DIR}/libc.a)

# Install rules — flat layout under share/wvmcc/.
install(DIRECTORY include/ DESTINATION share/wvmcc/include)
install(FILES ${RUNTIME_LIB_DIR}/libc.a DESTINATION share/wvmcc/lib)
