#include <skypat/skypat.h>

#define restrict __restrict__
extern "C"{
    #include <adt/Buffer.h>
    #include <ByteBuffer.h>
    #include <FileReader.h>
    #include <string.h>
}
#undef restrict

SKYPAT_F(FileReader, input_not_equal_output)
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

SKYPAT_F(FileReader, read_file)
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