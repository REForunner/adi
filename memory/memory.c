
#include "memory.h"
    
/**
 * Poll until the value of address is equal [value]
 * @param mem memory interface
 * @param address address of read
 * @param bytes Number of bytes
 * @param buffer Buffer to store read values
 * @return 0 on success, error code otherwise
 */
static int read(struct Mem * mem, uint32_t address, uint32_t bytes, uint8_t * buffer)
{
    uint8_t * buf = buffer;
    uint32_t adr = address;
    int result = -1;
    uint32_t mod = adr % 4UL;
    // when address is unaligned
    if(0UL != mod)
    {
        uint32_t rbsz = 4 - mod;
        // Align to the forward
        adr = adr - mod;
        // read
        result = mem->adi->readBytes(mem->adi, adr, (size_t)rbsz, buf);
        if(SUCCESS != result)
            return result;
        // update
        buf = buf + rbsz;
        bytes = bytes - rbsz;
        adr = (adr & ~3UL) + 4UL;
    }
    // when bytes is unaligned
    mod = bytes % 4UL;
    if(0UL != mod)
    {
        uint32_t rbsz = bytes - mod;
        // read
        result = mem->adi->readBlock(mem->adi, adr, (size_t)rbsz, (uint32_t *)buf);
        if(SUCCESS != result)
            return result;
        // update
        buf = buf + rbsz;
        bytes = bytes - rbsz;
        adr = adr + rbsz;
    }
    // if no read finish
    if(0UL != bytes)
    {
        // read
        result = mem->adi->readBytes(mem->adi, adr, (size_t)bytes, buf);
        if(SUCCESS != result)
            return result;
    }
    // return
    return (int)SUCCESS;
}

/**
 * Poll until the value of address is equal [value]
 * @param mem memory interface
 * @param address address of write
 * @param bytes Number of bytes
 * @param buffer Buffer to store write values
 * @return 0 on success, error code otherwise
 */
static int write(struct Mem * mem, uint32_t address, uint32_t bytes, const uint8_t * buffer)
{
    const uint8_t * buf = buffer;
    uint32_t adr = address;
    int result = -1;
    uint32_t mod = adr % 4UL;
    // when address is unaligned
    if(0UL != mod)
    {
        uint32_t rbsz = 4 - mod;
        // Align to the forward
        adr = adr - mod;
        // write
        result = mem->adi->writeBytes(mem->adi, adr, buf, (size_t)rbsz);
        if(SUCCESS != result)
            return result;
        // update
        buf = buf + rbsz;
        bytes = bytes - rbsz;
        adr = (adr & ~3UL) + 4UL;
    }
    // when bytes is unaligned
    mod = bytes % 4UL;
    if(0UL != mod)
    {
        uint32_t rbsz = bytes - mod;
        // write
        result = mem->adi->writeBlock(mem->adi, adr, (const uint32_t *)buf, (size_t)rbsz);
        if(SUCCESS != result)
            return result;
        // update
        buf = buf + rbsz;
        bytes = bytes - rbsz;
        adr = adr + rbsz;
    }
    // if no write finish
    if(0UL != bytes)
    {
        // write
        result = mem->adi->writeBytes(mem->adi, adr, buf, (size_t)bytes);
        if(SUCCESS != result)
            return result;
    }
    // return
    return (int)SUCCESS;
}

/**
 * Creat a new memory image intance
 * @param mem memory interface
 * @param adi ADI instance
 * @param private_data private data
 * @return Mem instance, or NULL on error
 */
Mem * mem_creat(Mem * mem, ADI * adi, void * private_data)
{
    // check param
    if (NULL == adi || NULL == mem)
        return NULL;
    
    // Initialize function pointers
    mem->read = read;
    mem->write = write;
    // set adi
    mem->adi = adi;
    // set private data
    mem->private_data = private_data;

    return mem;
}

/**
 * Destroy a Mem instance
 * @param mem Mem instance
 */
void mem_destroy(Mem * mem)
{
    // do something
}