/**
 *    Copyright 2017 Luis Hsu
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

#include <FileReader.h>

static int run_FileReader(Pass* pass){
    for(size_t i = 0; i < pass->input_count; ++i){
        const char* fileName = (const char*)pass->input[i]->data;
        FILE* fin = fopen(fileName, "rb");
        if(!fin){
            int curerrno = errno;
            fprintf(stderr, "error: %s in %s\n", strerror(errno), fileName);
            return curerrno;
        }
        if(fseek(fin, 0, SEEK_END)){
            int curerrno = errno;
            fprintf(stderr, "error: %s in %s\n", strerror(errno), fileName);
            return curerrno;
        }
        // read to output
        pass->output[i]->length = ftell(fin);
        if(pass->output[i]->length == -1L){
            int curerrno = errno;
            fprintf(stderr, "error: %s in %s\n", strerror(errno), fileName);
            return curerrno;
        }
        rewind(fin);
        pass->output[i]->data = malloc(sizeof(unsigned char) * pass->output[i]->length);
        fread(pass->output[i]->data, sizeof(unsigned char), pass->output[i]->length, fin);
        fclose(fin);
    }
    return 0;
}

static void free_FileReader(Pass** pass){
    free(*pass);
    *pass = NULL;
}

Pass* new_FileReader(Buffer** input, size_t input_count, Buffer** output, size_t output_count){
    // Input count should equal output count
    if(input_count != output_count){
        fprintf(stderr, "error: input count not equal to output in FileReader\n");
        return NULL;
    }
    // Create pass
    Pass *pass = (Pass*) malloc(sizeof(Pass));
    pass->input = input;
    pass->input_count = input_count;
    pass->output = output;
    pass->output_count = output_count;
    pass->contextMap = NULL;
    pass->run = run_FileReader;
    pass->free = free_FileReader;
    return pass;
}
