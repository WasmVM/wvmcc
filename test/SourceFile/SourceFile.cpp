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
#include <SourceFile.hpp>

using namespace WasmVM;
using namespace Testing;

Suite source_file {
    Test("normal", {
        SourceFile source(std::filesystem::path("normal.c"), "test");
        Expect(source.get() == 't');
        Expect(source.position().line() == 1 && source.position().col() == 1);
        Expect(source.get() == 'e');
        Expect(source.position().line() == 1 && source.position().col() == 2);
        Expect(source.get() == 's');
        Expect(source.position().line() == 1 && source.position().col() == 3);
        Expect(source.get() == 't');
        Expect(source.position().line() == 1 && source.position().col() == 4);
    })
    Test("newline", {
        SourceFile source(std::filesystem::path("newline.c"), "te\nst\n\na");
        Expect(source.get() == 't');
        Expect(source.position().line() == 1 && source.position().col() == 1);
        Expect(source.get() == 'e');
        Expect(source.position().line() == 1 && source.position().col() == 2);
        Expect(source.get() == '\n');
        Expect(source.get() == 's');
        Expect(source.position().line() == 2 && source.position().col() == 1);
        Expect(source.get() == 't');
        Expect(source.position().line() == 2 && source.position().col() == 2);
        Expect(source.get() == '\n');
        Expect(source.get() == '\n');
        Expect(source.get() == 'a');
        Expect(source.position().line() == 4 && source.position().col() == 1);
    })
    Test("concat", {
        SourceFile source(std::filesystem::path("concat.c"), "te\\\nst");
        Expect(source.get() == 't');
        Expect(source.position().line() == 1 && source.position().col() == 1);
        Expect(source.get() == 'e');
        Expect(source.position().line() == 1 && source.position().col() == 2);
        Expect(source.get() == 's');
        Expect(source.position().line() == 2 && source.position().col() == 1);
        Expect(source.get() == 't');
        Expect(source.position().line() == 2 && source.position().col() == 2);
    })
    Test("trigraph", {
        SourceFile source(std::filesystem::path("trigraph.txt"));
        Expect(source.get() == '#');
        Expect(source.position().line() == 1 && source.position().col() == 3);
        Expect(source.get() == '[');
        Expect(source.position().line() == 1 && source.position().col() == 6);
        Expect(source.get() == '\\');
        Expect(source.position().line() == 1 && source.position().col() == 9);
        Expect(source.get() == ']');
        Expect(source.position().line() == 1 && source.position().col() == 12);
        Expect(source.get() == '^');
        Expect(source.position().line() == 1 && source.position().col() == 15);
        Expect(source.get() == '{');
        Expect(source.position().line() == 1 && source.position().col() == 18);
        Expect(source.get() == '|');
        Expect(source.position().line() == 1 && source.position().col() == 21);
        Expect(source.get() == '}');
        Expect(source.position().line() == 1 && source.position().col() == 24);
        Expect(source.get() == '~');
        Expect(source.position().line() == 1 && source.position().col() == 27);
    })
};