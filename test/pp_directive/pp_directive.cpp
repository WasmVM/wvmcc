// Copyright (C) 2024 Luis Hsu
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
#include <PreProcessor.hpp>
#include <Error.hpp>

using namespace WasmVM;
using namespace Testing;

Suite pp_directive {
    Category("define", {
        Test("object-like", {
            PreProcessor pp(std::filesystem::path("obj_like.c"), "#define foo int test();");
            pp.get();
            Expect(pp.macros.size() == 1);
            Expect(pp.macros.begin()->name == "foo");
            Expect(pp.macros.begin()->replacement.size() == 6);
            Expect(!pp.macros.begin()->params);
        })
        Test("no-parameter", {
            PreProcessor pp(std::filesystem::path("no_param.c"), "#define foo() int test();");
            pp.get();
            Expect(pp.macros.size() == 1);
            Expect(pp.macros.begin()->name == "foo");
            Expect(pp.macros.begin()->replacement.size() == 6);
            Expect(pp.macros.begin()->params);
            Expect(pp.macros.begin()->params->size() == 0);
        })
        Test("one-parameter", {
            PreProcessor pp(std::filesystem::path("one_param.c"), "#define foo( par1) int test();");
            pp.get();
            Expect(pp.macros.size() == 1);
            Expect(pp.macros.begin()->name == "foo");
            Expect(pp.macros.begin()->replacement.size() == 6);
            Expect(pp.macros.begin()->params);
            Expect(pp.macros.begin()->params->size() == 1);
        })
        Test("more-parameter", {
            PreProcessor pp(std::filesystem::path("more_param.c"), "#define foo( par1 ,par2 ) int test();");
            pp.get();
            Expect(pp.macros.size() == 1);
            Expect(pp.macros.begin()->name == "foo");
            Expect(pp.macros.begin()->replacement.size() == 6);
            Expect(pp.macros.begin()->params);
            Expect(pp.macros.begin()->params->size() == 2);
        })
        Test("variable", {
            PreProcessor pp(std::filesystem::path("more_param.c"), "#define foo(...) int test();");
            pp.get();
            Expect(pp.macros.size() == 1);
            Expect(pp.macros.begin()->name == "foo");
            Expect(pp.macros.begin()->replacement.size() == 6);
            Expect(pp.macros.begin()->params);
            Expect(pp.macros.begin()->params->size() == 1);
            Expect(pp.macros.begin()->params->back() == "...");
        })
        Test("variable-parameter", {
            PreProcessor pp(std::filesystem::path("more_param.c"), "#define foo(par1, par2, ...) int test();");
            pp.get();
            Expect(pp.macros.size() == 1);
            Expect(pp.macros.begin()->name == "foo");
            Expect(pp.macros.begin()->replacement.size() == 6);
            Expect(pp.macros.begin()->params);
            Expect(pp.macros.begin()->params->size() == 3);
            Expect(pp.macros.begin()->params->back() == "...");
        })
        Test("variable-unclose", {
            PreProcessor pp(std::filesystem::path("more_param.c"), "#define foo( ...,) int test();");
            Throw(Exception::Error, pp.get())
        })
    })
};