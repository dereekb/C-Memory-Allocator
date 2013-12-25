/* 
    File: my_allocator.c

    Author: Derek Burgman
            Texas A&M University
    Date  : 9/21/2013

    Modified: 

    This file contains the implementation of the module "MY_ALLOCATOR".

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

#define EMPTY_VALUE 0x0
#define EMPTY_ADDRESS EMPTY_VALUE
#define minValue(a, b) (a < b) ? a : b;
#define maxValue(a, b) (a > b) ? a : b;
#define withinValues(val, upper, lower) val <= upper && val >= lower
#define adjustedIndex(index, min) (index - min)

typedef enum { false, true } bool;
typedef enum { left, right, neither } side;

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include <stdlib.h>
#include "my_allocator.h"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

/*
    Structures:
        - MemoryHeader : Header that resides at the head of the allocated memory block. Notes the index it is a part of.
        - FreestoreBlock : Header that resides at the head of the free memory block. Points to the next free block.
 
 
    So, the total structure of a memory block looks like:
 
    {[FreestoreBlock]...}
     
    or, if the space is being used.
     
    {[MemoryHeader]...}
 
    The sizes of these types are computed when they're being looked for, since that's when they're needed.
 */

/*
    Additionally, the freestore will automatically adjust it's size to ignore indexes that are too small due to fit a header.
 
    So if n = 10, and m = 100, and the header is of size 16, then the smallest index 0 would hold values of 10, since
    2^0 * 10 = 10.
 
    The freestore will adjust it's indexes so the index 0 holds value 1.
 */

typedef struct FreestoreBlock {
    Addr address;
    struct FreestoreBlock* nextBlock;
} FreestoreBlock;

typedef FreestoreBlock* Freestore;  //The freestore is just an array of FreestoreBlocks.

typedef struct MemoryHeader {
    int index;
    //Addr buddyHeader;     //Couldn't be set due to separation of MemoryHeader and FreestoreBlock methods.
                            //Buddy still can be calculated using index and physical memory address location.
    Addr memoryStart;       //This value here just to keep MemoryHeader the same size as Freestore Block.
} MemoryHeader;

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- Definitions -- */
    unsigned int _basic_block_size;
    unsigned int _length;

    unsigned int _headerSize;
    unsigned int _minFreestoreIndex;
    unsigned int _maxFreestoreIndex;

    unsigned int _minFreestoreIndexMemorySize;
    unsigned int _maxFreestoreIndexMemorySize;

    Freestore _freestoreAddress;

/* -- Sizes -- */
    unsigned int _freestoreIndex;   //Index that the freestore fits into. Use to retrieve size and protect freestore.
    unsigned int _maxFreestoreIndexMemorySize;
    unsigned int _freestoreRange;

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* Freestore Support Functions */
void resetFreestore(Freestore freestore, unsigned int range);

//Printing
void printFreestoreBlock(FreestoreBlock* block, unsigned int index);
void printFreestore(Freestore freestore, unsigned int range);

//Unadjusted Index
unsigned int getSizeForFreestoreIndex(unsigned int index);
unsigned int getFreestoreIndexForSize(unsigned int size);

//Adjusted Index
unsigned int getSizeForAdjustedFreestoreIndex(unsigned int index);
unsigned int getAdjustedFreestoreIndexForSize(unsigned int size);
unsigned int getAdjustedMinFreestoreIndexForRequestedSize(unsigned int requestedSized);

//Freestore Accessors
FreestoreBlock* getFirstFreestoreBlockAtIndex(unsigned int index);
FreestoreBlock* getLastFreestoreBlockAtIndex(unsigned int index);
FreestoreBlock* getFirstFreestoreBlockAtAdjustedIndex(unsigned int index);
FreestoreBlock* getLastFreestoreBlockAtAdjustedIndex(unsigned int index);
bool addAddressToFreestoreForIndex(unsigned int index, Addr memoryAddress);
bool addAddressToFreestoreForAdjustedIndex(unsigned int adjustedIndex, Addr memoryAddress);
bool removeLastFreestoreBlockAtIndex(unsigned int index);
bool removeLastFreestoreBlockAtAdjustedIndex(unsigned int index);
bool containsFreeSpaceAtAdjustedIndex(unsigned int index);

bool createFreestoreBlockAtAdjustedIndex(unsigned int index);

//Freestore Header Functions
bool containsFreestoreBlockAtAdjustedIndexWithAddress(unsigned int index, Addr memoryAddress);

FreestoreBlock* createFreestoreHeaderAtAddress(Addr memoryAddress, FreestoreBlock* nextBlock);
bool removeFreestoreBlockAtIndexWithAddress(unsigned int index, Addr memoryAddress);
bool removeFreestoreBlockAtAdjustedIndexWithAddress(unsigned int index, Addr memoryAddress);

Addr getAddressAtSide(unsigned int index, Addr memoryAddress, side side);
bool mergeBuddiesAtAddress(unsigned int index, Addr firstAddress, Addr secondAddress);
bool attemptBuddyMergeAtAdjustedIndexWithAddress(unsigned int index, Addr memoryAddress);

//Freestore Initialization
Addr subAddressForAdjustedIndex(Addr address, unsigned int index, side splitSide);
bool protectFreestoreHeader(Freestore freestore, unsigned int minFreestoreIndex, unsigned int maxFreestoreIndex);
Addr initFreestoreHeader(Addr startAddress, unsigned int minFreestoreIndex, unsigned int maxFreestoreIndex);

//Allocation Initialization and Lifetime
unsigned int minFreestoreIndexForSize(unsigned int basic_block_size, unsigned int headerSize);
unsigned int maxFreestoreIndexForSize(unsigned int basic_block_size, unsigned int length, unsigned int headerSize);
unsigned int init_allocator(unsigned int basic_block_size, unsigned int length);
int release_allocator();

//Allocation
bool canMeetMemoryRequest(unsigned int size);
MemoryHeader* allocateHeaderForSize(unsigned int size);

MemoryHeader* memoryHeaderForAddress(Addr memoryAddress);
bool deallocateHeaderAtAddress(Addr memoryAddress);
bool deallocateHeader(MemoryHeader* header);

/*--------------------------------------------------------------------------*/
// SUPPORT FUNCTIONS FOR FREESTORE
/*--------------------------------------------------------------------------*/

//Range is maxIndex - minIndex
void resetFreestore(Freestore freestore, unsigned int range)
{
    //printf("-Resetting Freestore Headers: Range(%d) \n",range);
    for(int i = 0; i <= range; i++)
    {
        FreestoreBlock* block = &freestore[i];   //Reset all values.
        block->address = EMPTY_ADDRESS;
        block->nextBlock = EMPTY_ADDRESS;
        //printf("Reset FreestoreBlock[%d] : Address: %p NextBlock: %p \n", i, block->address, block->nextBlock);
    }
    //printf("-\n\n");
}

void printFreestoreBlock(FreestoreBlock* block, unsigned int index)
{
    Addr blockAddress = block->address;
    Addr nextBlockAddress = block->nextBlock;
    unsigned int indexSize = getSizeForAdjustedFreestoreIndex(index);
    printf("FreestoreBlock (Size:%d) : Address: %p NextBlock: %p \n", indexSize, blockAddress, nextBlockAddress);
}

void printFreestore(Freestore freestore, unsigned int range)
{
    printf("-Printing Freestore Headers: Range(%d) \n",range);
    for(int i = 0; i <= range; i++)
    {
        FreestoreBlock* block = &freestore[i];
        Addr blockAddress = block->address;
        Addr nextBlockAddress = block->nextBlock;
        unsigned int indexSize = getSizeForAdjustedFreestoreIndex(i);
        printf("FreestoreBlock[%d] (Size:%d) (Addr:%p) : Address: %p NextBlock: %p \n", i, indexSize, block, blockAddress, nextBlockAddress);
    }
    printf("-\n\n");
}

void printDefaultFreestore(void)
{
    Freestore freestore = _freestoreAddress;
    unsigned int range = _freestoreRange;
    
    printf("-Printing Freestore Headers: Range(%d) \n",range);
    for(int i = 0; i <= range; i++)
    {
        FreestoreBlock* block = &freestore[i];
        Addr blockAddress = block->address;
        Addr nextBlockAddress = block->nextBlock;
        unsigned int indexSize = getSizeForAdjustedFreestoreIndex(i);
        printf("FreestoreBlock[%d] (Size:%d) (Addr:%p) : Address: %p NextBlock: %p \n", i, indexSize, block, blockAddress, nextBlockAddress);
    }
    printf("-\n\n");
}

/* Basic Freestore Index Math */
/*
    Unadjusted math is computed here.
 */
unsigned int getSizeForFreestoreIndex(unsigned int index)
{
    unsigned int size = _basic_block_size;
    
    if(index > 0)
    {
        unsigned int exponent = (1 << index);   //2^3 = 8
        size = (exponent * _basic_block_size);
    }
    
    return size;
}

unsigned int getFreestoreIndexForSize(unsigned int size)
{
    unsigned int index = 0;
    
    if(_basic_block_size < size)
    {
        //If BBS = 128, and size = 256, index will be 1 (2^i * BBS).
        //If BBS = 128 and size = 512, index will be 2 (2^2 * 128) = 512.
        double reducedSize = (size / (_basic_block_size * 1.0)); //2^i = (512/128) => 2^i = 4
        double logSize = log2(reducedSize); //log2(4) = i
        index = ceil(logSize);
    }
    
    return index;
}

/* Freestore Retrieval */

//Adjusted for space-saving technique described above.
unsigned int getSizeForAdjustedFreestoreIndex(unsigned int index)
{
    unsigned int adjustedIndex = (index + _minFreestoreIndex);  //Unadjust the size
    unsigned int size = getSizeForFreestoreIndex(adjustedIndex);
    return size;
}

unsigned int getAdjustedFreestoreIndexForSize(unsigned int size)
{
    unsigned int index = 0;
    
    if(_minFreestoreIndexMemorySize < size){
        index = getFreestoreIndexForSize(size);
        index = adjustedIndex(index, _minFreestoreIndex);
    }
    
    return index;
}

unsigned int getAdjustedMinFreestoreIndexForRequestedSize(unsigned int requestedSized)
{
    unsigned int completeSize = requestedSized + _headerSize;
    unsigned int minimum = getFreestoreIndexForSize(completeSize);
    minimum = adjustedIndex(minimum, _minFreestoreIndex);
    return minimum;
}

FreestoreBlock* getFirstFreestoreBlockAtIndex(unsigned int index)
{
    unsigned int adjustedIndex = adjustedIndex(index, _minFreestoreIndex);
    return getFirstFreestoreBlockAtAdjustedIndex(adjustedIndex);
}

/*
    Returns the last freestore block in the chain.
 */
FreestoreBlock* getLastFreestoreBlockAtIndex(unsigned int index)
{
    unsigned int adjustedIndex = adjustedIndex(index, _minFreestoreIndex);
    return getLastFreestoreBlockAtAdjustedIndex(adjustedIndex);
}

FreestoreBlock* getFirstFreestoreBlockAtAdjustedIndex(unsigned int adjustedIndex)
{
    Freestore freestore = _freestoreAddress;
    FreestoreBlock* block = &freestore[adjustedIndex];
    return block;
}

FreestoreBlock* getLastFreestoreBlockAtAdjustedIndex(unsigned int adjustedIndex)
{
    Freestore freestore = _freestoreAddress;
    FreestoreBlock* block = 0x0;

    block = &freestore[adjustedIndex];
    FreestoreBlock* nextBlock = block->nextBlock;
    
    //Iterate to the last block.
        while(nextBlock != 0x0)
        {
            nextBlock = block->nextBlock;
            if(nextBlock != 0x0)
            {
                block = nextBlock;
            }
        }
    
    return block;
}

bool addAddressToFreestoreForIndex(unsigned int index, Addr memoryAddress)
{
    unsigned int adjustedIndex = adjustedIndex(index, _minFreestoreIndex);
    return addAddressToFreestoreForAdjustedIndex(adjustedIndex, memoryAddress);
}

bool addAddressToFreestoreForAdjustedIndex(unsigned int adjustedIndex, Addr memoryAddress)
{
    bool success = false;
    FreestoreBlock* lastBlock = getLastFreestoreBlockAtAdjustedIndex(adjustedIndex);
    
    //Check to see if it is the default block.
    Addr lastBlockAddressValue = lastBlock->address;
    
    if(lastBlockAddressValue == 0x0)
    {
        lastBlock->address = memoryAddress; //Just set the address. Easy.
    } else {
        
        FreestoreBlock* newBlock = memoryAddress;   //Create the new block at the address.
        
        newBlock->address = memoryAddress;
        newBlock->nextBlock = EMPTY_ADDRESS;
        
        lastBlock->nextBlock = newBlock;            //Make the last block point to this one.
    }

    success = true;
    
    return success;
}

bool removeLastFreestoreBlockAtIndex(unsigned int index)
{
    unsigned int adjustedIndex = adjustedIndex(index, _minFreestoreIndex);
    return removeLastFreestoreBlockAtAdjustedIndex(adjustedIndex);
}

bool removeLastFreestoreBlockAtAdjustedIndex(unsigned int index)
{
    bool success = false;
    
    Freestore freestore = _freestoreAddress;
    FreestoreBlock* block = &freestore[index];
    FreestoreBlock* lastBlock = block->nextBlock;
    
    //The Freestore Index Header block is first.
    if(lastBlock == 0x0)
    {
        //Clear the address.
        block->address = EMPTY_ADDRESS;
    } else {
        
        //Find the last block in the chain, then remove it.
        FreestoreBlock* nextToLastBlock = block;
        lastBlock = block->nextBlock;
    
        while(true)
        {
            FreestoreBlock* nextBlock = lastBlock->nextBlock;
            if(nextBlock != 0x0)
            {
                nextToLastBlock = lastBlock;
                lastBlock = nextBlock;
            } else {
                break;
            }
        }
        
        //Clear Last Block
        lastBlock->address = EMPTY_ADDRESS;
        lastBlock->nextBlock = EMPTY_ADDRESS;
        
        //Clear Next To Last Block's pointer.
        nextToLastBlock->nextBlock = EMPTY_ADDRESS;
    }
    
    return success;
}

bool containsFreeSpaceAtAdjustedIndex(unsigned int index)
{
    bool contained = false;
    
    Freestore freestore = _freestoreAddress;
    FreestoreBlock* block = &freestore[index];
    contained = (block->address != 0x0);
    
    return contained;
}

bool createFreestoreBlockAtAdjustedIndex(unsigned int adjustedIndex)
{
    //Will create a new one, regardless if one already exists at this index.
    bool success = false;

    //Check to see if freespace at the upper index exists.
    //If one doesn't exist, recursively call this to create one at the next index.
    unsigned int splitIndex = adjustedIndex + 1;
    
    //Bug Patch: Keeps the program from crashing when it attempts to split the largest value.
    if(splitIndex >= _freestoreRange){
        return false;
    }
    
    bool splitIndexContainsBlock = containsFreeSpaceAtAdjustedIndex(splitIndex);
    bool splitSuccess = false;
    
    if(splitIndexContainsBlock == false)
    {
        //Recursively split the next block.
        splitSuccess = createFreestoreBlockAtAdjustedIndex(splitIndex);
    }

        //This code is executed when either a split occurs or the index contains a block.
    if (splitSuccess || splitIndexContainsBlock)
    {
        FreestoreBlock* lastBlock = getLastFreestoreBlockAtAdjustedIndex(splitIndex);
        Addr address = lastBlock->address;
        
        //Remove this from the chain.
        removeLastFreestoreBlockAtAdjustedIndex(splitIndex);
        
        //Split this address into two next Freestore blocks.
        Addr leftAddress = subAddressForAdjustedIndex(address, splitIndex, left);
        Addr rightAddress = subAddressForAdjustedIndex(address, splitIndex, right);
        
        //Add these addresses to the freestore
        addAddressToFreestoreForAdjustedIndex(adjustedIndex, leftAddress);
        addAddressToFreestoreForAdjustedIndex(adjustedIndex, rightAddress);
        
        //Mark our success.
        success = true;
    }
    
    return success;
}

FreestoreBlock* createFreestoreHeaderAtAddress(Addr memoryAddress, FreestoreBlock* nextBlock)
{
    FreestoreBlock* block = memoryAddress;
    block->address = memoryAddress;
    block->nextBlock = nextBlock;
    return block;
}

bool containsFreestoreBlockAtAdjustedIndexWithAddress(unsigned int index, Addr memoryAddress)
{
    bool contained = false;

    Freestore freestore = _freestoreAddress;
    FreestoreBlock* block = &freestore[index];
    
    while (block != 0x0 && (contained == false)) {
        contained = (block->address == memoryAddress);
        block = block->nextBlock;
    }

    return contained;
}

bool removeFreestoreBlockAtIndexWithAddress(unsigned int index, Addr memoryAddress)
{
    unsigned int adjustedIndex = adjustedIndex(index, _minFreestoreIndex);
    return removeFreestoreBlockAtAdjustedIndexWithAddress(adjustedIndex, memoryAddress);
}

bool removeFreestoreBlockAtAdjustedIndexWithAddress(unsigned int index, Addr memoryAddress)
{
    bool success = false;

    Freestore freestore = _freestoreAddress;
    FreestoreBlock* initialBlock = &freestore[index];
    
    if(initialBlock->address == memoryAddress)
    {
        initialBlock->address = 0x0;
        
        //Now we'll have to remove the first chained block, if it exists.
        //If it doesn't exist, we're done.
        
        FreestoreBlock* nextBlock = initialBlock->nextBlock;
        
        if(nextBlock != 0x0)
        {
            Addr nextAddress = nextBlock->address;
            initialBlock->address = nextAddress;
            
            //Now remove the "nextBlock" from the link.
            FreestoreBlock* continueBlock = nextBlock->nextBlock;
            initialBlock->nextBlock = continueBlock;
            
            //Clear the rest of nextBlock. It shouldn't be referenced anymore at this point.
            nextBlock->nextBlock = EMPTY_ADDRESS;
            nextBlock->address = EMPTY_ADDRESS;
        }
        
        success = true;
    } else {
        
        FreestoreBlock* currentBlock = initialBlock->nextBlock;
        FreestoreBlock* nextBlock = 0x0;
        FreestoreBlock* nextToLastBlock = initialBlock;
        
        while (currentBlock != 0x0) {
            
            nextBlock = currentBlock->nextBlock;
            
            Addr blockAddress = currentBlock->address;
            if(blockAddress == memoryAddress)
            {
                //Remove this block.
                currentBlock->address = EMPTY_ADDRESS;
                currentBlock->nextBlock = EMPTY_ADDRESS;
                
                //Patch the link between nextBlock and lastBlock
                nextToLastBlock->nextBlock = nextBlock;
                success = true;
                
            } else {
                nextToLastBlock = currentBlock;
                currentBlock = currentBlock->nextBlock;
            }
        }
    }
    
    return success;
}

Addr getContiguousAddressAtSide(unsigned int index, Addr memoryAddress, side side)
{
    unsigned int indexSize = getSizeForAdjustedFreestoreIndex(index);
    Addr buddyAddress = EMPTY_ADDRESS;
    
    switch (side) {
        case left:
        {
            buddyAddress = (memoryAddress - indexSize);
        }break;
        case right:
        {
            buddyAddress = (memoryAddress + indexSize);
        }break;
        default: break;
    }
    
    return buddyAddress;
}

bool mergeBuddiesAtAddress(unsigned int adjustedIndex, Addr firstAddress, Addr secondAddress)
{
    bool success = false;
    
    /*
        This function will not check to see if firstAddress and secondAddress are even connected.
     
        Used attemptBuddyMergeAtAdjustedIndexWithAddress to handle that.
     */
    bool removedFirst = removeFreestoreBlockAtAdjustedIndexWithAddress(adjustedIndex, firstAddress);
    bool removedSecond = removeFreestoreBlockAtAdjustedIndexWithAddress(adjustedIndex, secondAddress);
    
    if(!removedFirst || !removedSecond)
    {
        printf("ERROR> Merge Failed. One or more buddy(s) could not be removed from the freestore. \n");
    }
    
    //The Lowest Address is the Right-most address that will encompass both.
    Addr nextAddress = (firstAddress < secondAddress) ? firstAddress : secondAddress;
    
    unsigned superIndex = adjustedIndex + 1;
    success = addAddressToFreestoreForAdjustedIndex(superIndex, nextAddress);
    return success;
}

side getBuddySideForAddress(unsigned int index, Addr memoryAddress)
{
    
    //First, we need to protect the freestore with a quick check.
    //If the starting address matches the buddy address here, stop, since we don't want to corrupt our freestore.
    unsigned int indexSize = getSizeForAdjustedFreestoreIndex(index);
    bool buddyIsFreestore = (_freestoreAddress == (memoryAddress - indexSize));

    if (buddyIsFreestore)
    {
        return neither;
    }
    
    /*
     The buddy for this address is continguous, so it is either before or after this address.
     To figure this out, we need to check the bit the index coordinates to.
     
     Since we just need to know left or right (we know how far in either direction to reach the FreestoreHeader),
     we can use what we know to find this.
     
     What we know:
     - Start Address
     - Current Address
     - Distance from start address
     - Index
     
     Using these alone, we can see whether or not this is odd or even by comparing them to the initial address.
     If our address is even, then:
     If it is odd, this is the left block; if it's even, we're the right block.
     
     Simple Binary Example:
     00 01
     
     We'll use math to reduce the address into an even or odd value.
     
     */
    
    
    Addr startAddress = _freestoreAddress;
    /*
    long startAddressValue = (long)startAddress;    //Long to support 64bit systems.
    bool startAddressIsEven = ((startAddressValue%1) == 0);
    */
    
    //Now that we know whether or not the address is even, we can now see the difference.
    //long memoryAddressValue = (long)memoryAddress;
    long difference = (memoryAddress - startAddress); //Difference from the start address to this address.
    
    //Since difference was computed with the basic block size, remove that.
    long basicDifference = (difference / _basic_block_size);
    
    //This basic difference means if everything were in index 0, it would be this many blocks of BBS over.
    bool basicDifferenceIsEven = ((basicDifference%1) == 0);
    
    side memorySide = ((basicDifferenceIsEven) ? right : left);
    return memorySide;
}

bool attemptBuddyMergeAtAdjustedIndexWithAddress(unsigned int index, Addr memoryAddress)
{
    bool mergeSuccess = true;
    side buddySide = getBuddySideForAddress(index, memoryAddress);
    
    if(buddySide != neither)
    {
        //Continue to attempt to merge buddies.
        Addr buddyAddress = getContiguousAddressAtSide(index, memoryAddress, buddySide);
        
        if(buddyAddress != EMPTY_ADDRESS)
        {
            //Once last check to see they exist in the freestore.
            bool memoryAddressExistsInFreestore = containsFreestoreBlockAtAdjustedIndexWithAddress(index, memoryAddress);
            bool buddyAddressExistsInFreestore = containsFreestoreBlockAtAdjustedIndexWithAddress(index, buddyAddress);
            
            if(memoryAddressExistsInFreestore && buddyAddressExistsInFreestore)
            {
                mergeSuccess = mergeBuddiesAtAddress(index, memoryAddress, buddyAddress);
                
                if(mergeSuccess)
                {
                    unsigned int upperIndex = index + 1;
                    if(buddySide == left)
                    {
                        attemptBuddyMergeAtAdjustedIndexWithAddress(upperIndex, buddyAddress);
                    } else {
                        attemptBuddyMergeAtAdjustedIndexWithAddress(upperIndex, memoryAddress);
                    }
                }
            }
        }
    }
    
    return mergeSuccess;
}

/*--------------------------------------------------------------------------*/
// INITIALIZATION FUNCTIONS FOR FREESTORE
/*--------------------------------------------------------------------------*/

Addr subAddressForAdjustedIndex(Addr address, unsigned int index, side splitSide)
{
    Addr returnAddress = address;
    
    if(splitSide == right)
    {
        unsigned int size = getSizeForAdjustedFreestoreIndex(index);
        unsigned int splitSize = size/2;
        returnAddress = (address + splitSize);
    }
    
    return returnAddress;
}

/*
    We'll protect the header by permanently allocating a chunk of the memory to the freestore header.
 
    We'll attempt to minimize the memory usage by storing it in the smallest index possible.
 
    The header is incomplete here, since no headers have been written yet.
 */
bool protectFreestoreHeader(Freestore freestore, unsigned int minFreestoreIndex, unsigned int maxFreestoreIndex)
{
    unsigned int freestoreRange = adjustedIndex(maxFreestoreIndex, minFreestoreIndex);
    unsigned int adjustedMaxFreestoreIndex = freestoreRange;
    
    //Calculate size of the array
    unsigned int blockSize = sizeof(FreestoreBlock);
    unsigned int arraySize = blockSize * freestoreRange;
    
    //Retrieve the minimum Memory Block Index we can fit this freestore block into.
    unsigned int storeIndex = getAdjustedMinFreestoreIndexForRequestedSize(arraySize);
    _freestoreIndex = storeIndex;
    
    unsigned int maxIndexSize = getSizeForFreestoreIndex(maxFreestoreIndex);
    
    //Start building the free memory blocks space and the freestore by recursively splitting the open space.
    //We treat this area as the size of the maximum index.
    
    //Freestore marks the start of the allocated memory.
    Addr currentLeftAddress = freestore;
    
    //Break up the blocks to fit into this, and give those blocks headers and put them into the freestore.
    for(unsigned int i = adjustedMaxFreestoreIndex; i > storeIndex; i -= 1)
    {
        unsigned int subIndex = i - 1;
        
        //Split up the greatest index first into the left and right.
        Addr leftAddress = subAddressForAdjustedIndex(currentLeftAddress, i, left);
        Addr rightAddress = subAddressForAdjustedIndex(currentLeftAddress, i, right);
        
        FreestoreBlock* rightBlock = &freestore[subIndex];
        rightBlock->address = rightAddress; //Assign the Right address into the freestore.
        rightBlock->nextBlock = 0x0;
        
        //Add the right address to the freestore at the given index.
        currentLeftAddress = leftAddress;
    }

    //Now the freestore should be initialized fully.
    //We have only addressed half of our memory at this point, so we need to address the rest.
    if(maxIndexSize < _length)
    {
        unsigned int totalLeftoverSpace = _length - maxIndexSize;
        unsigned int currentLeftoverSpace = totalLeftoverSpace;
        unsigned int minimumIndexSize = _minFreestoreIndexMemorySize;
        Addr freestoreAddress = freestore;
        Addr currentLeftoverAddress = (freestoreAddress + _maxFreestoreIndexMemorySize);
        
        while(currentLeftoverSpace > minimumIndexSize)
        {
            unsigned int currentLeftoverIndex = getAdjustedFreestoreIndexForSize(currentLeftoverSpace);
            unsigned int sizeForLeftoverIndex = getSizeForAdjustedFreestoreIndex(currentLeftoverIndex);
            unsigned int storageIndex = (sizeForLeftoverIndex > currentLeftoverSpace) ? (currentLeftoverIndex - 1) : currentLeftoverIndex;
            unsigned int sizeForStorageIndex = getSizeForAdjustedFreestoreIndex(storageIndex);
            
            addAddressToFreestoreForAdjustedIndex(storageIndex, currentLeftoverAddress);
            
            currentLeftoverAddress = currentLeftoverAddress + sizeForStorageIndex;
            currentLeftoverSpace = currentLeftoverSpace - sizeForStorageIndex;
        }
        
        
        if(currentLeftoverSpace > 0)
        {
            printf("WARNING: A small amount of memory of size '%d' was unable to fit into the freestore. \n\n", currentLeftoverSpace);
        }
    }

    return true;
}

/*
 Build the freestore header that keeps track of free blocks.
 
 The freestore header is built at the front of the allocated space.
 */
Addr initFreestoreHeader(Addr startAddress, unsigned int minFreestoreIndex, unsigned int maxFreestoreIndex)
{
    //The pointer to pointers of the freestore block now start at the startAddress.
    unsigned int freestoreRange = maxFreestoreIndex - minFreestoreIndex;
    
    if(freestoreRange == 0){
        return 0;   //If there is no freestore range, then function fails.
    }
    
    Freestore freestore = startAddress;
    
    resetFreestore(freestore, freestoreRange);
    
    _freestoreRange = freestoreRange;
    _maxFreestoreIndexMemorySize = getSizeForAdjustedFreestoreIndex(freestoreRange);
    
    protectFreestoreHeader(freestore, minFreestoreIndex, maxFreestoreIndex);
    
    
    
    return startAddress;
}

unsigned int minFreestoreIndexForSize(unsigned int basic_block_size, unsigned int headerSize)
{
    unsigned int minIndex = 0;
    
    if(headerSize > basic_block_size)
    {
        double reducedSize = (headerSize / (_basic_block_size * 1.0));
        double logSize = log2(reducedSize);
        unsigned int fitIndex = ceil(logSize);
        
        minIndex = fitIndex;
    }
    
    //Make sure that we can fit more than the header in fitIndexSize.
    unsigned int fitIndexSize = getSizeForFreestoreIndex(minIndex);
    if(fitIndexSize <= headerSize)
    {
        minIndex += 1;
    }
    
    return minIndex;
}

unsigned int maxFreestoreIndexForSize(unsigned int basic_block_size, unsigned int length, unsigned int headerSize)
{
    unsigned int maxIndex = 0;
    
    double reducedSize = ((length - headerSize) / (_basic_block_size * 1.0));
    double logSize = log2(reducedSize);
    maxIndex = floor(logSize);             //We want the lowest index to ensure it will fit.
    
    return maxIndex;
}

/*--------------------------------------------------------------------------*/
/* SUPPORT FUNCTIONS FOR MODULE MY_ALLOCATOR */
/*--------------------------------------------------------------------------*/
bool indexCanMeetMemoryRequest(unsigned int index)
{
    return containsFreeSpaceAtAdjustedIndex(index);
}

bool canMeetMemoryRequest(unsigned int size)
{
    bool canMeet = false;
    
    //First check the index, then recursively go up
    unsigned int minIndex = getAdjustedFreestoreIndexForSize(size);
    unsigned int currentIndex = minIndex;
    unsigned int maxAdjustedIndex = _freestoreRange;
    while(canMeet == false && currentIndex <= maxAdjustedIndex)
    {
        canMeet = containsFreeSpaceAtAdjustedIndex(currentIndex);
        currentIndex++;
    }
    
    return canMeet;
}

MemoryHeader* allocateHeaderForSize(unsigned int size)
{
    MemoryHeader* header = 0x0;
    
    unsigned int combinedSize = size + _headerSize;
    
    if(canMeetMemoryRequest(combinedSize) == false){
        return EMPTY_ADDRESS;   //Can't allocate more than is available.
    }

    unsigned int targetIndex = getAdjustedFreestoreIndexForSize(combinedSize);
    bool freeAvailable = containsFreeSpaceAtAdjustedIndex(targetIndex);
    bool created = freeAvailable;
    
    if(freeAvailable == false)
    {
        //Create a new block.
        created = createFreestoreBlockAtAdjustedIndex(targetIndex);
    }
    
    //Retrieve our new block if created. If created is false, we've run out of memory.
    if(created == true)
    {
        FreestoreBlock* freeblock = getLastFreestoreBlockAtAdjustedIndex(targetIndex);
        Addr freeblockAddress = freeblock->address;
        
        //Delete our free block.
        //Values for this block are reset in this function so it can be used as a header now.
        removeLastFreestoreBlockAtAdjustedIndex(targetIndex);
        
        header = freeblockAddress;    //Treat the target address as a header now.
        
        MemoryHeader testBlock = *header;
        testBlock.index = 0;
        testBlock.memoryStart = 0;
        
        header->index = targetIndex;
        void* memoryStartAddress = (freeblockAddress + _headerSize);
        header->memoryStart = memoryStartAddress;
    }
    
    return header;
}

MemoryHeader* memoryHeaderForAddress(Addr memoryAddress)
{
    unsigned int headerSize = _headerSize;
    Addr address = (memoryAddress - headerSize);
    
    //Verify MemoryHeader integrity.
    MemoryHeader* header = address;
    
    if(header != 0x0)
    {
        bool valid = (header->memoryStart == memoryAddress);
        
        if(!valid){
            header = 0x0;
        }
    }
    
    return header;
}

bool deallocateHeaderAtAddress(Addr memoryAddress)
{
    bool success = false;
    MemoryHeader* header = memoryHeaderForAddress(memoryAddress);
    
    if(header != 0x0){
        success = deallocateHeader(header);
    }
    
    return success;
}

bool deallocateHeader(MemoryHeader* header)
{
    bool success = false;
    
    if(header != 0x0){
        unsigned int adjustedIndex = header->index;
        Addr startAddress = header;
        
        //Clear the header.
        header->memoryStart = EMPTY_ADDRESS;
        header->index = EMPTY_VALUE;
        
        //Address is returned to the freestore. We still need to check for buddies though.
        bool addSuccess = addAddressToFreestoreForAdjustedIndex(adjustedIndex, startAddress);
        
        if(!addSuccess){
            printf("ERROR> Reinsert Failure: Could not reinsert address(%p) into freestore. \n",startAddress);
        }
        
        //Attempt to merge buddies, if it's buddy is free.
        attemptBuddyMergeAtAdjustedIndexWithAddress(adjustedIndex, startAddress);
    }

    return success;
}

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTIONS FOR MODULE MY_ALLOCATOR */
/*--------------------------------------------------------------------------*/

unsigned int init_allocator(unsigned int basic_block_size, unsigned int length){
    
    int allocatedSize = 0;
    
    if(basic_block_size < length){
        
        size_t size = (size_t)length;
        Addr startAddress = malloc(size);
        
        _basic_block_size = basic_block_size;
        _length = length;
        _headerSize = sizeof(FreestoreBlock);
        
        _minFreestoreIndex = minFreestoreIndexForSize(basic_block_size, _headerSize);
        _minFreestoreIndexMemorySize = getSizeForFreestoreIndex(_minFreestoreIndex);
        _maxFreestoreIndex = maxFreestoreIndexForSize(basic_block_size, length, _headerSize);
        _maxFreestoreIndexMemorySize = getSizeForFreestoreIndex(_minFreestoreIndex);
        _freestoreAddress = startAddress;
        
        Addr freestoreAddress = initFreestoreHeader(startAddress, _minFreestoreIndex, _maxFreestoreIndex);
        _freestoreAddress = freestoreAddress;
        
        if(_freestoreAddress == EMPTY_ADDRESS)
        {
            free(startAddress);
        }
        
        allocatedSize = _length;
    }
    
    return allocatedSize;
}

int release_allocator(){
    free(_freestoreAddress);
    return 0;
}

extern Addr my_malloc(unsigned int length) {
    MemoryHeader* header = allocateHeaderForSize(length);
    Addr address = 0x0;
    
    if(header != 0x0)
    {
        address = header->memoryStart;
    } else {
        printf("ERROR> Allocation Failure: Could not deliver size(%d) for request. \n",length);
    }
    
    return address;
}

extern int my_free(Addr address) {
    bool success = deallocateHeaderAtAddress(address);
    return (success == true) ? 0 : 1;
}

