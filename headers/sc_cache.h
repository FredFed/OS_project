/* DESCRIZIONE CACHE */

#ifndef sc_cache_h
#define sc_cache_h

#include "conc_elem.h"
#include "conc_hash.h"
#include "conc_fifo.h"

byte nth_chance=2;      // indicates the "chance" order of the algorithm (2 for second chance)

// defining the "second chance cache" structure
typedef struct sc_cache {
    conc_hash_table* ht;
    unsigned max_file_number;
    unsigned max_byte_size;
    unsigned curr_file_number;
    unsigned curr_byte_size;
    pthread_mutex_t mem_check_mtx;
} sc_cache;

// defining the structure of a file inside the cache
typedef struct file {
    char* name;     // represents the file's identifier inside the file server
    // HUFFMAN TREE CONTAINING THE DECOMPRESSION INFORMATION
    int file_size;
    byte* data;     // represents the actual information contained in the file
    byte f_lock;     // flag indicating if and by whom the file is currently locked
    byte f_open;     // flag indicating if the file is currently open
    byte f_write;    // flag indicating if the writeFile operation can be executed
    // LOCK QUEUE
    // OPEN LIST
} file;


// MAIN OPERATIONS
sc_cache* sc_cache_create(int, int);                // returns an empty sc-cache data structure of capacity and size(bytes) given
int sc_cache_insert(sc_cache*, file*, file**);      // pushes a file in the cache, as a "recently used" file, getting the expelled files
int sc_algorithm(sc_cache*, unsigned, file**);         // second chance replacement algorithm

#endif // sc_cache_h