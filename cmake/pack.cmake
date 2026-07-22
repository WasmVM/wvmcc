# CPack packaging for wvmcc (modeled on WasmVM's cmake/pack.cmake).
#
# Generators used by .github/workflows/pack.yml:
#   Linux:  DEB + TGZ        macOS:  productbuild + TGZ
# wvmcc is not built on Windows, so there is no NSIS branch.
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
# Unix-only: install into CMAKE_INSTALL_PREFIX (e.g. /usr/local) rather than a
# staged per-package prefix, matching WasmVM's packages.
set(CPACK_SET_DESTDIR ON)
set(CPACK_THREADS 0)

# Debian
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Luis Hsu")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "wasmvm")
set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

# Productbuild (macOS)
set(CPACK_PRODUCTBUILD_IDENTIFIER org.WasmVM.wvmcc)

include(CPack)
