// Copyright (C) 2023 Luis Hsu
// 
// wvmcc is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// wvmcc is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with wvmcc. If not, see <http://www.gnu.org/licenses/>.

#include <harness.hpp>
#include <sstream>
#include <SourceStream.hpp>

using namespace WasmVM;
using namespace Testing;

Suite source_stream {
    Test("normal", {
        SourceTextStream source("test", std::filesystem::path("normal.c"));
        Expect(source.get() == 't');
        Expect(source.pos.line() == 1 && source.pos.col() == 1);
        Expect(source.get() == 'e');
        Expect(source.pos.line() == 1 && source.pos.col() == 2);
        Expect(source.get() == 's');
        Expect(source.pos.line() == 1 && source.pos.col() == 3);
        Expect(source.get() == 't');
        Expect(source.pos.line() == 1 && source.pos.col() == 4);
    })
    Test("newline", {
        SourceTextStream source("te\nst\n\na", std::filesystem::path("newline.c") );
        Expect(source.get() == 't');
        Expect(source.pos.line() == 1 && source.pos.col() == 1);
        Expect(source.get() == 'e');
        Expect(source.pos.line() == 1 && source.pos.col() == 2);
        Expect(source.get() == '\n');
        Expect(source.get() == 's');
        Expect(source.pos.line() == 2 && source.pos.col() == 1);
        Expect(source.get() == 't');
        Expect(source.pos.line() == 2 && source.pos.col() == 2);
        Expect(source.get() == '\n');
        Expect(source.get() == '\n');
        Expect(source.get() == 'a');
        Expect(source.pos.line() == 4 && source.pos.col() == 1);
    })
    Test("concat", {
        SourceTextStream source("te\\\nst", std::filesystem::path("concat.c"));
        Expect(source.get() == 't');
        Expect(source.pos.line() == 1 && source.pos.col() == 1);
        Expect(source.get() == 'e');
        Expect(source.pos.line() == 1 && source.pos.col() == 2);
        Expect(source.get() == 's');
        Expect(source.pos.line() == 2 && source.pos.col() == 1);
        Expect(source.get() == 't');
        Expect(source.pos.line() == 2 && source.pos.col() == 2);
    })
    Test("trigraph", {
        SourceFileStream source(std::filesystem::path("trigraph.txt"));
        Expect(source.get() == '#');
        Expect(source.pos.line() == 1 && source.pos.col() == 3);
        Expect(source.get() == '[');
        Expect(source.pos.line() == 1 && source.pos.col() == 6);
        Expect(source.get() == '\\');
        Expect(source.pos.line() == 1 && source.pos.col() == 9);
        Expect(source.get() == ']');
        Expect(source.pos.line() == 1 && source.pos.col() == 12);
        Expect(source.get() == '^');
        Expect(source.pos.line() == 1 && source.pos.col() == 15);
        Expect(source.get() == '{');
        Expect(source.pos.line() == 1 && source.pos.col() == 18);
        Expect(source.get() == '|');
        Expect(source.pos.line() == 1 && source.pos.col() == 21);
        Expect(source.get() == '}');
        Expect(source.pos.line() == 1 && source.pos.col() == 24);
        Expect(source.get() == '~');
        Expect(source.pos.line() == 1 && source.pos.col() == 27);
    })
};