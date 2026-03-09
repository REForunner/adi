
#ifndef MEMORY_H
#define MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "../adi/adi.h"

/**
 * memory interface
 */
typedef struct Mem {
    /**
     * Private data
     */
    void * private_data;
    
    /**
     * ADI interface
     */
    ADI * adi;

    /**
     * Poll until the value of address is equal [value]
     * @param mem memory interface
     * @param address address of read
     * @param bytes Number of bytes
     * @param buffer Buffer to store read values
     * @return 0 on success, error code otherwise
     */
    int ( * read)(struct Mem * mem, uint32_t address, uint32_t bytes, uint8_t * buffer);

    /**
     * Poll until the value of address is equal [value]
     * @param mem memory interface
     * @param address address of write
     * @param bytes Number of bytes
     * @param buffer Buffer to store write values
     * @return 0 on success, error code otherwise
     */
    int ( * write)(struct Mem * mem, uint32_t address, uint32_t bytes, const uint8_t * buffer);

} Mem;

/**
 * Creat a new memory image intance
 * @param mem memory interface
 * @param adi ADI instance
 * @param private_data private data
 * @return Mem instance, or NULL on error
 */
Mem * mem_creat(Mem * mem, ADI * adi, void * private_data);

/**
 * Destroy a Mem instance
 * @param mem Mem instance
 */
void mem_destroy(Mem * mem);

#ifdef __cplusplus
}
#endif

#endif /* MEMORY_H */