# CPack packaging for wvmcc (modeled on WasmVM's cmake/pack.cmake).
#
# Generators used by .github/workflows/pack.yml:
#   Linux:  DEB + TGZ        macOS:  productbuild + TGZ
# Windows (NSIS) is configured below for LOCAL `cpack` use only — wvmcc has no
# Windows CI yet, so no packaging job publishes a Windows installer until a
# Windows build is verified (see the discussion in RELEASING.md).
#
# Packages carry bin/wvmcc plus the share/wvmcc sysroot (headers + libc.a).
# wvmcc links the WasmVM *shared* library, and running its output needs the
# `wasmvm` interpreter — hence the Debian dependency on the wasmvm package
# (installed from WasmVM's GitHub releases).

set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR "WasmVM")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "A freestanding C17-to-WebAssembly compiler toolchain targeting WasmVM")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/WasmVM/wvmcc")
set(CPACK_VERBATIM_VARIABLES True)
set(CPACK_OUTPUT_FILE_PREFIX ${PROJECT_ROOT}/packages)
set(CPACK_THREADS 0)

if(WIN32 AND NOT UNIX)
    # NSIS installer: staged install dir, uninstall-first, PATH entry so
    # `wvmcc` resolves its sysroot via dirname(argv[0])/../share/wvmcc.
    set(CPACK_SET_DESTDIR OFF)
    set(CPACK_NSIS_PACKAGE_NAME wvmcc)
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_PACKAGE_INSTALL_DIRECTORY wvmcc)
else()
    # Unix: install into CMAKE_INSTALL_PREFIX (e.g. /usr/local) rather than a
    # staged per-package prefix, matching WasmVM's packages.
    set(CPACK_SET_DESTDIR ON)
endif()

# Debian
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Luis Hsu")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "wasmvm")
set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

# Productbuild (macOS)
set(CPACK_PRODUCTBUILD_IDENTIFIER org.WasmVM.wvmcc)

include(CPack)
