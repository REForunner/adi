
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
 * memory information
 */
typedef struct MemInfo {
    /**
     * used flag
     */
    bool isUSE;
    /**
     * start address
     */
    uint32_t addr;
    /**
     * memory size bytes[all data]
     */
    uint32_t bytes;
    /**
     * memory data pointer
     */
    uint8_t * data;
    /**
     *  data buffer size
     */
    uint32_t bufSize;
    /**
     * Private data maybe "getData" used
     */
    void * private_data;
    /**
     * get current memory data
     * @param info memory information interface
     * @param private_data private data eg. Transmit and obtain custom structures related to data
     * @param data pointer of store the get data
     * @param needbytes need to bytes when obtaining, must be less then MemInfo.bufSize!!!
     * @return 0 on success, error code otherwise
     */
    int ( * getData)(struct MemInfo * info, void * private_data, uint8_t * data, uint32_t needbytes);

} MemInfo;

/**
 * memory image
 */
typedef struct MemImage {
    /**
     * Private data
     */
    void * private_data;

    /**
     * stack
     */
    MemInfo stack;

    /**
     * breakpoint instruction 
     */
    MemInfo bkpt;

    /**
     * ram code (include static areas)
     */
    MemInfo ramCode;

    /**
     * data buffer
     */
    MemInfo buffer;
    
    /**
     * data buffer 2, if use double buffer
     */
    MemInfo buffer_2;

} MemImage;

/**
 * memory interface
 */
typedef struct Mem {
    /**
     * Private data
     */
    void * private_data;
    
    /**
     * memory image
     */
    MemImage image;

    /**
     * ADI interface
     */
    ADI * adi;

    /**
     * build ram image in the target
     * @param mem memory interface
     * @return 0 on success, error code otherwise
     */
    int ( * buildImage)(struct Mem * mem);
    
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