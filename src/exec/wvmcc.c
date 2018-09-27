/**
 *    Copyright 2018 LuisHsu
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CCOption.h>
#include <PassManager.h>
#include <ByteBuffer.h>
#include <FileReader.h>

int main(int argc, const char *argv[])
{
    // Set Option
    CCOption* option = new_CCOption(argc - 1, argv + 1);
    if(option->input_count < 1){
        fprintf(stderr, "error: no input file\n");
        exit(-1);
    }else if(option->input_count > 1){
        fprintf(stderr, "error: only one input file is allowed\n");
        exit(-1);
    }
    // PassManager
    PassManager* passManager = new_PassManager();
    // FileReader
    ByteBuffer* inputFileName = new_ByteBuffer(strlen(option->input_files[0]));
    strcpy((char*)inputFileName->data, option->input_files[0]);
    ByteBuffer* inputSrc = new_ByteBuffer(0);
    FileReader* fileReader = new_FileReader(&inputFileName, 1, &inputSrc, 1);
    passManager->addPass(passManager, fileReader);
    // Run pass
    run_PassManager(passManager);
    // Clean
    free_CCOption(&option);
    free_PassManager(&passManager);
    return 0;
}