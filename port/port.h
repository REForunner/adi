
#ifndef TRANSPORT_H
#define TRANSPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "../adi/adi.h"
#include "../processor/processor.h"
#include "../memory/memory.h"

/**
 * this libary version
 */
#define LIB_VERSION     "0.0.1"

/**
 * port interface
 */
typedef struct Port {
    /**
     * Private data
     */
    void * private_data;

    /**
     * generate swd waveform interface
     */
    SWD swd;

    /**
     * ADI interface
     */
    ADI adi;

    /**
     * Processor interface
     */
    Processor processor;

    /**
     * memory interface
     */
    Mem mem;

} Port;

#ifdef __cplusplus
}
#endif

#endif /* TRANSPORT_H */