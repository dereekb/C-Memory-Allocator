C-Memory-Allocator
==================

Created by Derek Burgman on 9/21/2013

C-based memory allocation program that runs a memory test using the Ackermann Function (http://en.wikipedia.org/wiki/Ackermann_function).

malloc is performed once when the allocation program itself is initialized. 

The memory allocator works by keeping track of a "freestore" that reference open areas. 

The allocator implements the "buddy system" to track blocks of memory that are of the specified block size * 2^nth power, and combines/separates these blocks as they're used.





Was an interesting project, but the allocator could definitely use some fine-tuning and code cleanup. Feel free to do what you want with it.
