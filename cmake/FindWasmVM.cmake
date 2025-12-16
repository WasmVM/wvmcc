find_library(WASMVM_LIBRARY REQUIRED
    NAMES WasmVM
)
get_filename_component(WASMVM_LIBRARY_DIR ${WASMVM_LIBRARY} DIRECTORY)
find_path(WASMVM_INCLUDE_DIR REQUIRED
    NAMES WasmVM.hpp
    PATH_SUFFIXES WasmVM
)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WasmVM REQUIRED_VARS WASMVM_INCLUDE_DIR WASMVM_LIBRARY_DIR WASMVM_LIBRARY)