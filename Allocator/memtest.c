#include "ackerman.h"
#include "my_allocator.h"
#include <stdlib.h>

#define B * 1
#define KB * 1024
#define MB * 1048576

/*
 Derek Burgman
 Allocator Memory Test
 
 Commands:
 -b : Basic Block Size to use in this test.
 -s : Memory Size in Bytes to use in this test.
 -k : Memory Size in Kilobytes to use in this test.
 -m : Memory Size in Megabytes to use in this test.
 -t : Identifier of more simple test to run before ackermann memtest.
 -x : First parameter of simple memtest.
 -y : Second parameter of simple memtest.
 -z : When to run the simple memtest. (Will not run if -t = 0);
 
 
 Example: 
 memtest -b 5 -m 128   //Runs with Basic Block Size of 5 and 128MB
*/


typedef struct Options{
    unsigned int error;
    unsigned int testIdentifier;
    unsigned int testParamA;
    unsigned int testParamB;
    unsigned int testAfterAckermann;
    unsigned int basicBlockSize;
    unsigned int memorySize;
} Options;

/*
    Rapidly consumes the input at a time, causing indexes to split.
 
    Best tester of splitting indexes.
    Non-recursive since in most cases it would exceed the stack.
 */
int mawTest(unsigned int size)
{
    Addr address = my_malloc(size);
    
    while(address != 0)
    {
        address = my_malloc(size);
    }
    
    return 0;
}

/*
    Starting at the smallest index, the required memory increases at an exponential rate, ending at the given index.
 
    Set free to true to free immediately after allocating. 
 
    Used to just test that each index is working properly.
 */
int forTest(unsigned int maxIndex, unsigned int free)
{
    for(int i = 0; i < maxIndex; i++)
    {
        unsigned int memorySize = (1 << i);  //1 MB, then 2, etc...
       
        Addr address = my_malloc(memorySize);
        
        if(free){
            my_free(address);
        }
    }
    
    return 0;
}

/*
    Recursively calls this function and exponentially increases memory until ending memory is reached.
 
    Similar to for-loop, except this function will not free memory until it has finished allocating memory.
 */
int recursiveTest(unsigned int memory, unsigned int endingMemory)
{
    Addr result = my_malloc(memory);
    
    if(result != 0){
        
        if(memory < endingMemory)
        {
            recursiveTest(memory * 2, endingMemory);
        }
        
        my_free(result);
    }
    
    return 0;
}

int runTest(Options options)
{
    unsigned int testIdentifier = options.testIdentifier;
    unsigned int parameterA = options.testParamA;
    unsigned int parameterB = options.testParamB;
    
    printf("Running Test(%d): A(%d) B(%d)",testIdentifier,parameterA,parameterB);
    
    switch (testIdentifier) {
        case 1:{
            return mawTest(parameterA);
        }break;
        case 2:{
            return forTest(parameterA, parameterB);
        }break;
        case 3:{
            return recursiveTest(parameterA, parameterB);
        }break;
    }
    
    return 0;
}

Options buildOptions(int argc, char ** argv)
{
    Options options;
    
    options.memorySize = 512 KB;
    options.basicBlockSize = 128 B;
    options.testIdentifier = 0;
    options.testParamA = 2;
    options.testParamB = 128 KB;
    options.testAfterAckermann = 0;
    
    
    for (int i = 1; i < argc; i += 2)
    {
        char* p = argv[i];
        
        // if option does not start from dash, report an error
        if (*p != '-'){
            options.error = 1;
            return options;
        }
        
        switch (p[1]) // switch on whatever comes after dash
        {
            case 'b': options.basicBlockSize = atoi(argv[i+1]); break;
            case 's': options.memorySize = atoi(argv[i+1]); break;
            case 'k': options.memorySize = (atoi(argv[i+1]) KB); break; //Define kilobytes instead of bytes using s
            case 'm': options.memorySize = (atoi(argv[i+1]) MB); break; //Define megabytes insetad of bytes using m
            case 't': options.testIdentifier = atoi(argv[i+1]); break;      //Run tests.
            case 'x': options.testParamA = atoi(argv[i+1]); break;          //Test Parameter A
            case 'y': options.testParamB = atoi(argv[i+1]); break;          //Test Parameter B
            case 'z': options.testAfterAckermann = atoi(argv[i+1]); break;  //Run before/after ackermann mem test.
            default : { options.error = 1; return options; }
        }
    }
    
    return options;
}

int main(int argc, char ** argv) {
    
    Options options = buildOptions(argc, argv);
    
    printf("Derek Burgman - Allocator Memory Test\n\n");
    printf("Commands: \n");
    printf("-b : Basic Block Size to use in this test.\n");
    printf("-s : Memory Size in Bytes to use in this test.\n");
    printf("-k : Memory Size in Kilobytes to use in this test.\n");
    printf("-m : Memory Size in Megabytes to use in this test.\n");
    printf("-t : Identifier of more simple test to run before ackermann memtest.\n");
    printf("-x : First parameter of simple memtest.\n");
    printf("-y : Second parameter of simple memtest.\n");
    printf("-z : When to run the simple memtest. (Will not run if -t = 0);\n");
    printf("Example: memtest -b 5 -m 128\n\n\n");
    
    unsigned int memorySize = options.memorySize;
    unsigned int basic_block_size = options.basicBlockSize;
    
    printf("memtest options:\n - memory: ~%d KB\n - block size: %d B\n - testId: %d\n\n", options.memorySize / 1024, options.basicBlockSize, options.testIdentifier);
    
    init_allocator(basic_block_size, memorySize);
    
    if(options.testIdentifier > 0 && options.testAfterAckermann == 0){
        runTest(options);
    }
    
    ackerman_main();
    
    if(options.testIdentifier > 0 && options.testAfterAckermann == 1){
        runTest(options);
    }
    
    release_allocator();
}


