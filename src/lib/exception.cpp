// Copyright 2025 Luis Hsu. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <exception.hpp>

using namespace WasmVM;

std::optional<std::function<void(std::string)>> Exception::Warning::handler = std::nullopt;
