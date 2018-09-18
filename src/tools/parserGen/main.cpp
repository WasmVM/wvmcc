//    Copyright 2018 Luis Hsu
// 
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
// 
//        http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.

#include <iostream>
#include <cstring>

#define restrict __restrict__
#define _Bool bool
extern "C"{
    #include <PassManager.h>
    #include <ByteBuffer.h>
    #include <FileReader.h>
}
#undef restrict

int main(int argc, char const *argv[])
{
    // Check arguments
    if(argc < 2){
        std::cout << "Usage:" << std::endl;
        std::cout << "\twvmcc-parsergen [Rule file] [output path]" << std::endl;
        return -1;
    }
    // Prepare Passes
    PassManager* passManager = new_PassManager();
    // FileReader
    ByteBuffer* inputRuleName = new_ByteBuffer(strlen(argv[0]));
    strcpy((char*)inputRuleName->data, argv[0]);
    ByteBuffer* inputRuleSrc = new_ByteBuffer(0);
    FileReader* fileReader = new_FileReader(&inputRuleName, 1, &inputRuleSrc, 1);
    passManager->addPass(passManager, fileReader);
    // Run pass
    run_PassManager(passManager);
    // Clean
    free_PassManager(&passManager);
    return 0;
}
