
#include <stdlib.h>
#include <string.h>
#include "processor.h"

/**
 * if register no ready, retry count
 */
#define S_REGRDY_RETRY_CNT  100UL

/**
 * Enable debug
 * @param processor Processor instance
 * @return 0 on success, error code otherwise
 */
static int debuegEnable(struct Processor * processor)
{
    // debug enable
    return processor->adi->writeMem32(processor->adi, (uint32_t)DHCSR, DBGKEY | C_DEBUGEN);
}

/**
 * Disable debug
 * @param processor Processor instance
 * @return 0 on success, error code otherwise
 */
static int debuegDisable(struct Processor * processor)
{
    // debug disable
    return processor->adi->writeMem32(processor->adi, (uint32_t)DHCSR, DBGKEY);
}

/**
 * Whether the target is debug enable
 * @param processor Processor instance
 * @param debug Pointer to store debug state
 *                 "true" is halt
 *                 "false" is other states
 * @return 0 on success, error code otherwise
 */
static int isDebug(struct Processor * processor, bool * debug)
{
    uint32_t dhcsr = 0;
    // read register DHCSR
    int result = processor->adi->readMem32(processor->adi, (uint32_t)DHCSR, &dhcsr);
    if(result != SUCCESS)
        return result;
    // halt check
    * debug = (C_DEBUGEN == (dhcsr & C_DEBUGEN)) ? true : false;

    return SUCCESS;
}

/**
 * Connect the target chip with in debug
 * @param processor Processor instance
 * @param idcode idcode for read
 * @return 0 on success, error code otherwise
 */
static int connect(struct Processor * processor, int * idcode)
{
    int result = -1;
    // swd bus
    result = processor->adi->connect(processor->adi, idcode);
    if(SUCCESS != result)
        return result;
    // in debug state
    return processor->debuegEnable(processor);
}

/**
 * Disconnect the target chip with leave debug
 * @param processor Processor instance
 * @return 0 on success, error code otherwise
 */
static int disconnect(struct Processor * processor)
{
    int result = -1;
    // leave debug state
    result = processor->debuegDisable(processor);
    if(SUCCESS != result)
        return result;
    // swd bus reset
    return processor->adi->disconnect(processor->adi);
}

/**
 * Get the state of the processor core
 * @param processor Processor instance
 * @param state Pointer to store core state
 * @return 0 on success, error code otherwise
 */
static int getState(struct Processor * processor, CoreState * state)
{
    uint32_t dhcsr = 0;
    // read register DHCSR
    int result = processor->adi->readMem32(processor->adi, (uint32_t)DHCSR, &dhcsr);
    if(result != SUCCESS)
        return result;
    
    // if debug???
    if(C_DEBUGEN == (dhcsr & C_DEBUGEN))
    {
        // Check status bits
        if(S_LOCKUP == (dhcsr & S_LOCKUP))
            * state = LOCKUP;
        else if(S_SLEEP == (dhcsr & S_SLEEP))
            * state = SLEEPING;
        else if(S_HALT == (dhcsr & S_HALT))
            * state = HALT;
        else if(S_RESET_ST == (dhcsr & S_RESET_ST) && (S_RETIRE_ST != (dhcsr & S_RETIRE_ST)))
            * state = RESET;
        else
            * state = RUNNING;
    }
    else
    {
        // no debug
        * state = NODEBUG;
    }

    return SUCCESS;
}

/**
 * Whether the target is halted
 * @param processor Processor instance
 * @param halted Pointer to store halted state
 *                 "true" is halt
 *                 "false" is other states
 * @return 0 on success, error code otherwise
 */
static int isHalted(struct Processor * processor, bool * halted)
{
    uint32_t dhcsr = 0;
    // read register DHCSR
    int result = processor->adi->readMem32(processor->adi, (uint32_t)DHCSR, &dhcsr);
    if(result != SUCCESS)
        return result;
    // halt check
    * halted = (S_HALT == (dhcsr & S_HALT)) ? true : false;

    return SUCCESS;
}

/**
 * Halt the target
 * @param processor Processor instance
 * @return 0 on success, error code otherwise
 */
static int halt(struct Processor * processor)
{
    uint32_t value = 0;
    int result = 0;
    // set halt
    result =  processor->adi->writeMem32(processor->adi, DHCSR, (uint32_t)(DBGKEY | C_DEBUGEN | C_HALT));
    if(SUCCESS != result)
        return result;
    // if no read, some chip maybe no halt
    return processor->adi->readMem32(processor->adi, DHCSR, &value);
}

/**
 * Run the target
 * @param processor Processor instance
 * @return 0 on success, error code otherwise
 */
static int run(struct Processor * processor)
{
    uint32_t value = 0;
    // clear DFSR
    int result = processor->adi->writeMem32(processor->adi, DFSR, (uint32_t)(HALTED | BKPT | DWTTRAP | VCATCH | EXTERNAL));
    if(SUCCESS != result)
        return result;
    // then run
    result = processor->adi->writeMem32(processor->adi, DHCSR, (uint32_t)(DBGKEY | C_DEBUGEN | C_MASKINTS));
    if(SUCCESS != result)
        return result;
    // if no read, some chip maybe no run
    return processor->adi->readMem32(processor->adi, DHCSR, &value);
}

/**
 * Soft reset the target
 * @param processor Processor instance
 * @param hwReset reset the target use hardware
 *                 "true" : reset use hardware
 *                 "false" : reset use soft
 * @param us used by call-back "waitDelay"
 * @param waitDelay if "hwReset" = true used it. Determine if it is overdue, return "0" is timeout
 * @return 0 on success, error code otherwise
 */
static int reset(struct Processor * processor, ResetMode hwReset, uint32_t us, bool (* waitDelay)(uint32_t us))
{
    uint32_t value = 0;
    int result = -1;
    // clear core reset request
    result = processor->adi->writeMem32(processor->adi, DEMCR, 0UL);
    if(result != SUCCESS)
        return result;
    // if soft reset
    if(SystemReset == hwReset)
    {
        // sys reset request
        result = processor->adi->writeMem32(processor->adi, AIRCR, (uint32_t)(VECTKEY | SYSRESETREQ));
        if(result != SUCCESS)
            return result;
        // if no read, some chip maybe no reset
        return processor->adi->readMem32(processor->adi, DHCSR, &value);
    }
    // if hardware reset
    else
    {
        // check param
        if(NULL == waitDelay)
            return ParamError;
        // rst pin catch
        result = processor->adi->reset(processor->adi, true);
        if(result != SUCCESS)
            return result;
        // wait delay
        while(true == waitDelay(us)) { ; }
        // rst pin relases
        return processor->adi->reset(processor->adi, false);
    }
}

/**
 * Set the target to reset state with software
 * @param processor Processor instance
 * @return 0 on success, error code otherwise
 */
static int setResetStateWithSoft(struct Processor * processor)
{
    int result = 0;
    uint32_t value = 0;
    int cntout = 0;
    // halt
    result = processor->halt(processor);
    if(result != SUCCESS)
        return result;
    // set corereset
    result = processor->adi->readMem32(processor->adi, DEMCR, &value);
    if(result != SUCCESS)
        return result;
    result = processor->adi->writeMem32(processor->adi, DEMCR, value | CORERESET);
    if(result != SUCCESS)
        return result;
    // system reset request
    result = processor->adi->writeMem32(processor->adi, AIRCR, (uint32_t)(VECTKEY | SYSRESETREQ));
    if(result != SUCCESS)
        return result;
    // wait reset finish
    do{
        result = processor->adi->readMem32(processor->adi, DHCSR, &value);
        if(result != SUCCESS)
            return result;
        cntout += 1;
        if(cntout >= S_REGRDY_RETRY_CNT)
            return RETRY_OVERFLOW;
    } while(S_RESET_ST != (S_RESET_ST & value));
    //
    cntout = 0;
    //
    do{
        result = processor->adi->readMem32(processor->adi, DHCSR, &value);
        if(result != SUCCESS)
            return result;
        cntout += 1;
        if(cntout >= S_REGRDY_RETRY_CNT)
            return RETRY_OVERFLOW;
    } while(S_RESET_ST == (S_RESET_ST & value));    
    // clear corereset
    result = processor->adi->readMem32(processor->adi, DEMCR, &value);
    if(result != SUCCESS)
        return result;
    return processor->adi->writeMem32(processor->adi, DEMCR, value & ~CORERESET);
}

/**
 * Read from a core register
 * @param processor Processor instance
 * @param reg The register to read
 * @param value Pointer to store value
 * @return 0 on success, error code otherwise
 */
static int readCoreRegister(struct Processor * processor, CoreRegister reg, uint32_t * value)
{
    uint32_t dhcsr = 0;
    uint32_t cntout = 0;
    // Write register number to DCRSR
    int result = processor->adi->writeMem32(processor->adi, DCRSR, (uint32_t)reg);
    if (result != SUCCESS)
        return result;
    // wait ready
    do{
        result = processor->adi->readMem32(processor->adi, DHCSR, &dhcsr);
        if (result != SUCCESS)
            return result;
        cntout += 1;
        if(cntout >= S_REGRDY_RETRY_CNT)
            return RETRY_OVERFLOW;
    } while(S_REGRDY != (dhcsr & S_REGRDY));
    // Read value from DCRDR
    return processor->adi->readMem32(processor->adi, DCRDR, value);
}

/**
 * Write to a core register
 * @param processor Processor instance
 * @param reg The register to write to
 * @param value The value to write
 * @return 0 on success, error code otherwise
 */
static int writeCoreRegister(struct Processor * processor, CoreRegister reg, uint32_t value)
{
    uint32_t dhcsr = 0;
    uint32_t cntout = 0;
    // Write value to DCRDR
    int result = processor->adi->writeMem32(processor->adi, DCRDR, value);
    if (result != SUCCESS)
        return result;
    // Write register number to DCRSR
    result = processor->adi->writeMem32(processor->adi, DCRSR, (uint32_t)reg | REGWnR);
    if (result != SUCCESS)
        return result;
    // wait ready
    do{
        result = processor->adi->readMem32(processor->adi, DHCSR, &dhcsr);
        if (result != SUCCESS)
            return result;
        cntout += 1;
        if(cntout >= S_REGRDY_RETRY_CNT)
            return RETRY_OVERFLOW;
    } while(S_REGRDY != (dhcsr & S_REGRDY));
    // success
    return SUCCESS;
}


/**
 * read core register
 * @param processor Processor instance
 * @param reg The register to read  eg. reg[0] = R0, reg[1] = R1, etc.
 * @param value The value to read
 * @param count register number to read
 * @return 0 on success, error code otherwise
 */
static int readMultiCoreRegisters(struct Processor * processor, const CoreRegister * reg, uint32_t * value, const uint32_t count)
{
    uint32_t cntout = 0;
    uint32_t dhcsr = 0;
    int result = 0;
    ADI * adi = processor->adi;
    // set ap.TAR = DHCSR
    // then ap.BD0 = DHCSR; ap.BD1 = DCRSR; ap.BD2 = DCRDR; ap.BD3 = DEMCR
    result = adi->writeAP(adi, TAR, DHCSR);
    if(SUCCESS != result)
        return result;
    // read loop
    for(int i = 0; i < count; i++)
    {
        cntout = 0;
        // Write register number to BD1[DCRSR]
        result = adi->writeAP(adi, BD1, (uint32_t)(reg[i]));
        if(SUCCESS != result)
            return result;
        // wait ready
        do{ // BD0[DHCSR]
            result = adi->readAP(adi, BD0, &dhcsr);
            if(SUCCESS != result)
                return result;
            cntout += 1;
            if(cntout >= S_REGRDY_RETRY_CNT)
                return RETRY_OVERFLOW;
        } while(S_REGRDY != (dhcsr & S_REGRDY));
        // Read value from BD2[DCRDR]
        result = adi->readAP(adi, BD2, &(value[i]));
        if(SUCCESS != result)
            return result;
    }
    // return
    return SUCCESS;
}

/**
 * Write to core register
 * @param processor Processor instance
 * @param reg The register to write to  eg. reg[0] = R0, reg[1] = R1, etc.
 * @param value The value to write
 * @param count register number to write to
 * @return 0 on success, error code otherwise
 */
static int writeMultiCoreRegisters(struct Processor * processor, const CoreRegister * reg, const uint32_t * value, const uint32_t count)
{
    uint32_t cntout = 0;
    uint32_t dhcsr = 0;
    int result = 0;
    ADI * adi = processor->adi;
    // set ap.TAR = DHCSR
    // then ap.BD0 = DHCSR; ap.BD1 = DCRSR; ap.BD2 = DCRDR; ap.BD3 = DEMCR
    result = adi->writeAP(adi, TAR, DHCSR);
    if(SUCCESS != result)
        return result;
    // read loop
    for(int i = 0; i < count; i++)
    {
        cntout = 0;
        // Write value to BD2[DCRDR]
        result = adi->writeAP(adi, BD2, value[i]);
        if(SUCCESS != result)
            return result;
        // Write register number to BD1[DCRSR]
        result = adi->writeAP(adi, BD1, (uint32_t)reg[i] | REGWnR);
        if(SUCCESS != result)
            return result;
        // wait ready
        do{ // BD0[DHCSR]
            result = adi->readAP(adi, BD0, &dhcsr);
            if(SUCCESS != result)
                return result;
            cntout += 1;
            if(cntout >= S_REGRDY_RETRY_CNT)
                return RETRY_OVERFLOW;
        } while(S_REGRDY != (dhcsr & S_REGRDY));
    }
    // return
    return SUCCESS;
}

/**
 * Create a new Processor instance
 * @param adi ADI instance to use
 * @param private_data Private data
 * @return Processor instance, or NULL on error
 */
Processor * processor_create(Processor * processor, ADI * adi, void * private_data)
{
    // check param
    if (NULL == processor || NULL == adi)
        return NULL;
    
    // Initialize ADI interface
    processor->adi = adi;
    // Initialize function pointers
    processor->debuegEnable = debuegEnable;
    processor->debuegDisable = debuegDisable;
    processor->isDebug = isDebug;
    processor->connect = connect;
    processor->disconnect = disconnect;
    processor->getState = getState;
    processor->isHalted = isHalted;
    processor->halt = halt;
    processor->run = run;
    processor->reset = reset;
    processor->setResetStateWithSoft = setResetStateWithSoft;
    processor->readCoreRegister = readCoreRegister;
    processor->writeCoreRegister = writeCoreRegister;
    processor->readMultiCoreRegisters = readMultiCoreRegisters;
    processor->writeMultiCoreRegisters = writeMultiCoreRegisters;
    // Initialize private data
    processor->private_data = private_data;

    return processor;
}

/**
 * Destroy a Processor instance
 * @param processor Processor instance
 */
void processor_destroy(Processor *processor) 
{
    // do somrthing...
}