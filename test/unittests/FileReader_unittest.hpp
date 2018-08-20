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

#include <skypat/skypat.h>

#define restrict __restrict__
extern "C"{
    #include <adt/Buffer.h>
    #include <ByteBuffer.h>
    #include <FileReader.h>
    #include <string.h>
}
#undef restrict

SKYPAT_F(FileReader_unittest, input_not_equal_output)
{
    Buffer* input[3] = {
        new Buffer,
        new Buffer,
        new Buffer
    };
    Buffer* output[2] = {
        new Buffer,
        new Buffer
    };
    FileReader* reader = new_FileReader(input, 3, output, 2);
    EXPECT_EQ(reader, NULL);
    for(int i = 0; i < 3; ++i){
        delete input[i];
    }
    for(int i = 0; i < 2; ++i){
        delete output[i];
    }
}

SKYPAT_F(FileReader_unittest, read_file)
{
    const char* fileName = ".testFile";
    // Create test file
    FILE* ftmp = fopen(fileName, "w");
    fprintf(ftmp, "testtest");
    fclose(ftmp);
    // Prepare
    Buffer* input = new_ByteBuffer(10);
    strcpy((char*)input->data, fileName);
    Buffer* output = new_ByteBuffer(0);
    FileReader* reader = new_FileReader(&input, 1, &output, 1);
    // Run
    reader->run(reader);
    // Check
    EXPECT_EQ(output->length, 8);
    printf("%s\n", (char*) output->data);
    // Clean
    input->free(&input);
    output->free(&output);
    reader->free(&reader);
    remove(fileName);
}