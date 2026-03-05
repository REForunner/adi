
#include "adi.h"

/**
 * adi version v5.2
 */
#define ADI_Version     "ADIv5.2"
/**
 * idcode MINDP mask
 */
#define MINDP_MASK      0x00010000UL
/**
 * swd sequence direction
 */
#define SWJ_SEQUENCE_WRITE  0UL
#define SWJ_SEQUENCE_READ   1UL
/**
 * if wait, retry count
 */
#define RETRY_CNT       100UL
/**
 * APnDP enum
 */
typedef enum APnDP : bool {
    /**
     * Debug Port (DP)
     */
    DebugPort = 0,
    /**
     * Access Port (AP)
     */
    AccessPort
} APnDP;
/**
 * RnW enum
 */
typedef enum RnW : bool{
    /**
     * Write
     */
    Write = 0,
    /**
     * Read
     */
    Read
} RnW;
/**
 * generate even parity
 */
#define EVEN_PARITY(parity)     (parity ? 1UL : 0UL)
/**
 * start defined
 */
#define START_BIT   1UL
/**
 * stop defined
 */
#define STOP_BIT    0UL
/**
 * park defined
 */
#define PARK_BIT    1UL
/**
 * ap select or ap/dp bank select
 */
#define DP_BANK(reg)    ((reg >> 4) & DPBANKSEL)
#define AP_BANK(reg)    (reg & APBANKSEL)
#define AP_SEL(reg)     ((reg & 0xFF00) << 16)
/**
 * A2,A3 from register address
 */
#define A2nA3_ADDRESS(reg)      ((reg >> 2UL) & 0x03UL)

#pragma pack(1)  // Single-byte alignment
/**
 * swd protocol
 */
typedef struct request{
    uint8_t start : 1;  // must be 1
    uint8_t APnDP : 1;  // DP = 0; AP = 1
    uint8_t RnW : 1;    // write = 0; read = 1
    uint8_t A2nA3 : 2;  // A2,A3 first(LSB)
    uint8_t parity : 1; // Even parity(include APnDP, RnW, A2nA3)
    uint8_t stop : 1;   // must be 0
    uint8_t park : 1;   // must be 1
} request;
#pragma pack()

/**
 * generation a request
 * @param ad @ref APnDP
 * @param rw @ref RnW
 * @param reg @ref DPRegister or @ref APRegister
 * @return request
 */
static uint8_t generatRequest(APnDP ad, RnW rw, uint8_t reg)
{
    request r = { 0 };
    r.start = START_BIT;
    r.APnDP = ad;
    r.RnW = rw;
    r.A2nA3 = A2nA3_ADDRESS(reg);
    r.parity = EVEN_PARITY(r.APnDP ^ r.RnW ^ (reg & 1UL) ^ ((reg & 2UL) >> 1));
    r.stop = STOP_BIT;
    r.park = PARK_BIT;

    return *(uint8_t *)&r;
}

/**
 * Connect to target device
 * @param adi ADI instance
 * @param idcode idcode for read
 * @return 0 on success, error code otherwise
 */
static int connect(struct ADI * adi, int * idcode) 
{
    int result = 0;
    uint32_t value = 0UL;
    // reset swj interface(refer to jlink)
    const uint8_t idleCycles = 0x00;
    const uint16_t jtag2swd = 0xe79e;
    const uint16_t oldjtag2swd = 0xedb6;
    const uint32_t lineReset[2] = { 0xffffffff, 0xffffffff };
    // at least 50 clocks with high
    adi->swd->Sequence(adi->swd, SWJ_SEQUENCE_WRITE, (sizeof(lineReset) * 8UL), (uint8_t *)lineReset);
    // jtag -> swd  (0xe79e [lsb] or 0x79e7 [msb])
    adi->swd->Sequence(adi->swd, SWJ_SEQUENCE_WRITE, (sizeof(jtag2swd) * 8UL), (uint8_t *)&jtag2swd);
    // at least 50 clocks with high
    adi->swd->Sequence(adi->swd, SWJ_SEQUENCE_WRITE, (sizeof(lineReset) * 8UL), (uint8_t *)lineReset);
    // old jtag -> swd (0xedb6 [lsb] for compatibility)
    adi->swd->Sequence(adi->swd, SWJ_SEQUENCE_WRITE, (sizeof(oldjtag2swd) * 8UL), (uint8_t *)&oldjtag2swd);
#if 0
    // Leaving dormant state
    const uint32_t AlertSequence[4] = { 0x6209f392, 0x86852d95, 0xe3ddafe9, 0x19bc0ea2 };
    // at least 8 clocks with high
    adi->swd->Sequence(adi->swd, SWJ_SEQUENCE_WRITE, (sizeof(lineReset) * 8UL), (uint8_t *)lineReset);
    // send 128-bit Selection Alert sequence(lsb)
    adi->swd->Sequence(adi->swd, SWJ_SEQUENCE_WRITE, (sizeof(AlertSequence) * 8UL), (uint8_t *)AlertSequence);
    // 4 clocks with low
    adi->swd->Sequence(adi->swd, SWJ_SEQUENCE_WRITE, 4UL, (uint8_t *)&idleCycles);
    // Send the required activation code sequence
    // ...
#endif
    // at least 50 clocks with high
    adi->swd->Sequence(adi->swd, SWJ_SEQUENCE_WRITE, (sizeof(lineReset) * 8UL), (uint8_t *)lineReset);
    // then at least two idle cycles(2 clocks with low)
    adi->swd->Sequence(adi->swd, SWJ_SEQUENCE_WRITE, (sizeof(idleCycles) * 8UL), (uint8_t *)&idleCycles);
    // read idcode(dpidr); if success, record it
    result = adi->readDP(adi, IDCODE, (uint32_t *)idcode);
    if(SUCCESS != result)
        return (int)result;
    adi->last.idcode = * idcode;
    // set select register
    // adi->last.select = 0UL;  // apsel default maybe not 0!!!
    result = adi->writeDP(adi, SELECT, (uint32_t)adi->last.select);
    if(SUCCESS != result)
        return (int)result;
    // request debug and system power up
    result = adi->writeDP(adi, CTRL_STAT, (uint32_t)(CDBGPWRUPREQ | CSYSPWRUPREQ));
    if(SUCCESS != result)
        return (int)result;
    // wait some us
    volatile uint32_t timecnt = 100UL;
    do { timecnt--; } while(timecnt != 0UL);
    // check reponse
    timecnt = 0UL;
    do {
        result = adi->readDP(adi, CTRL_STAT, (uint32_t *)&value);
        if(SUCCESS != result)
            return (int)result;
        if((value & (CDBGPWRUPACK | CSYSPWRUPACK)) == (CDBGPWRUPACK | CSYSPWRUPACK))
            break;
        timecnt++;
    } while(timecnt <= adi->retry);
    // record ctrl/stat
    if((value & (CDBGPWRUPACK | CSYSPWRUPACK)) != (CDBGPWRUPACK | CSYSPWRUPACK))
        return (int)DBGPWRError;
    adi->last.ctrlstatus = CDBGPWRUPREQ | CSYSPWRUPREQ;
    // abort clear
    result = adi->writeDP(adi, ABORT, (uint32_t)(STKCMPCLR | STKERRCLR | WDERRCLR | ORUNERRCLR));
    if(SUCCESS != result)
        return (int)result;
    // request dbg rst
    value = adi->last.ctrlstatus | CDBGRSTREQ;
    result = adi->writeDP(adi, CTRL_STAT, value);
    if(SUCCESS != result)
        return (int)result;
    // relieve dbgrst and set maskline is all
    value = adi->last.ctrlstatus | MASKLANE;
    result = adi->writeDP(adi, CTRL_STAT, value);
    if(SUCCESS != result)
        return (int)result;
    adi->last.ctrlstatus = adi->last.ctrlstatus | MASKLANE;
    // config ap.csw
    result = adi->readAP(adi, CSW, (uint32_t *)&adi->last.csw);
    if(SUCCESS != result)
        return (int)result;
    // return ack
    return (int)SUCCESS;
}

/**
 * Disconnect from target device
 * @param adi ADI instance
 * @return 0 on success, error code otherwise
 */
static int disconnect(struct ADI * adi)
{
    int result = -1;
    uint32_t cntout = 0;
    uint32_t ctrlstat = 0;
    // refer to stlink
    const uint16_t swd2jtag = 0xe73c;
    const uint32_t lineReset[2] = { 0xffffffff, 0xffffffff };
    // abort
    result = adi->writeDP(adi, ABORT, (uint32_t)(DAPABORT | STKCMPCLR | STKERRCLR | WDERRCLR | ORUNERRCLR));
    if(SUCCESS != result)
        return (int)result;
    // power down
    // Per ADIv6 spec. Clear first CSYSPWRUPREQ followed by CDBGPWRUPREQ
    result = adi->readDP(adi, CTRL_STAT, &ctrlstat);
    if(SUCCESS != result)
        return (int)result;
    result = adi->writeDP(adi, CTRL_STAT, ctrlstat & ~CSYSPWRUPREQ);
    if(SUCCESS != result)
        return (int)result;
    do{ 
        result = adi->readDP(adi, CTRL_STAT, &ctrlstat);
        if(SUCCESS != result)
            return (int)result;
        cntout += 1;
        if(cntout >= adi->retry)
            return RETRY_OVERFLOW;
    } while((ctrlstat & (CSYSPWRUPACK)) != CSYSPWRUPACK);
    // Clear second CDBGPWRUPREQ
    result = adi->writeDP(adi, CTRL_STAT, ctrlstat & ~CDBGPWRUPREQ);
    if(SUCCESS != result)
        return (int)result;
    do{ 
        result = adi->readDP(adi, CTRL_STAT, &ctrlstat);
        if(SUCCESS != result)
            return (int)result;
        cntout += 1;
        if(cntout >= adi->retry)
            return RETRY_OVERFLOW;
    } while((ctrlstat & (CDBGPWRUPACK)) != CDBGPWRUPACK);
    // line reset(at least 50 clocks high)
    adi->swd->Sequence(adi->swd, SWJ_SEQUENCE_WRITE, (sizeof(lineReset) * 8UL), (uint8_t *)lineReset);
    // swd -> jtag
    adi->swd->Sequence(adi->swd, SWJ_SEQUENCE_WRITE, (sizeof(swd2jtag) * 8UL), (uint8_t *)&swd2jtag);
    // then at least five clocks with high
    adi->swd->Sequence(adi->swd, SWJ_SEQUENCE_WRITE, (sizeof(lineReset) * 8UL), (uint8_t *)lineReset);
    // return ack
    return (int)SUCCESS;
}

/**
 * Reset target device
 * @param adi ADI instance
 * @param hwReset reset the target use hardware
 *                  "true" : the hardware pin is catch
 *                  "false" : the hardware pin is release
 * @return 0 on success, error code otherwise
 */
static int reset(struct ADI * adi, bool hwReset)
{
    // reset operation
    adi->swd->hwReset(adi->swd, hwReset);
    // return
    return SUCCESS;
}

/**
 * Read from a debug port register
 * @param adi ADI instance
 * @param reg ID of register to read
 * @param value Pointer to store register value
 * @return 0 on success, error code otherwise
 */
static int readDP(struct ADI * adi, DPRegister reg, uint32_t * value)
{
    // check bank select
    if(DP_BANK(reg) != (adi->last.select & DPBANKSEL))
    {
        uint32_t select = (adi->last.select & (~DPBANKSEL)) | DP_BANK(reg);
        // if ack is wait, retry it
        for(int i = 0; ; i++)
        {
            // switch dp bank(write dp.select)
            adi->last.ack = (Ack)adi->swd->Transfer(adi->swd, generatRequest(DebugPort, Write, SELECT), &select);
            // if retry overflow
            if(i >= adi->retry)
                return (int)adi->last.ack;
            // contiue???
            if(WAIT == adi->last.ack)
                continue;
            else if(OK == adi->last.ack)
                break;
            else
                return (int)adi->last.ack;
        }
        // record bank
        adi->last.select = select;
    }
    // if ack is wait, retry it
    for(int j = 0; j < adi->retry; j++)
    {
        // read register
        adi->last.ack = (Ack)adi->swd->Transfer(adi->swd, generatRequest(DebugPort, Read, reg), value);
        if(WAIT == adi->last.ack)
            continue;
        else
            return (int)((OK == adi->last.ack) ? SUCCESS : adi->last.ack);
    }
    // return ack
    return (int)((OK == adi->last.ack) ? SUCCESS : adi->last.ack);            
}

/**
 * Write to a debug port register
 * @param adi ADI instance
 * @param reg ID of register to write
 * @param value Value to write
 * @return 0 on success, error code otherwise
 */
static int writeDP(struct ADI * adi, DPRegister reg, uint32_t value)
{
    // check bank select
    if(DP_BANK(reg) != (adi->last.select & DPBANKSEL))
    {
        uint32_t select = (adi->last.select & (~DPBANKSEL)) | DP_BANK(reg);
        // if ack is wait, retry it
        for(int i = 0; ; i++)
        {
            // switch dp bank(write dp.select)
            adi->last.ack = (Ack)adi->swd->Transfer(adi->swd, generatRequest(DebugPort, Write, SELECT), &select);
            // if retry overflow
            if(i >= adi->retry)
                return (int)adi->last.ack;
            // contiue???
            if(WAIT == adi->last.ack)
                continue;
            else if(OK == adi->last.ack)
                break;
            else
                return (int)adi->last.ack;
        }
        // record bank
        adi->last.select = select;
    }
    // if ack is wait, retry it
    for(int j = 0; j < adi->retry; j++)
    {
        // write register
        adi->last.ack = (Ack)adi->swd->Transfer(adi->swd, generatRequest(DebugPort, Write, reg), &value);
        if(WAIT == adi->last.ack)
            continue;
        else
            return (int)((OK == adi->last.ack) ? SUCCESS : adi->last.ack);
    }
    // return ack
    return (int)((OK == adi->last.ack) ? SUCCESS : adi->last.ack);  
}

/**
 * Read from an access port register
 * @param adi ADI instance
 * @param reg ID of register to read
 * @param value Pointer to store register value
 * @return 0 on success, error code otherwise
 */
static int readAP(struct ADI * adi, APRegister reg, uint32_t * value)
{
    // check bank select
    if((AP_BANK(reg) != (adi->last.select & APBANKSEL)) || (AP_SEL(reg) != (adi->last.select & APSEL)))
    {
        uint32_t select = (adi->last.select & (~(APBANKSEL | APSEL))) | AP_BANK(reg) | AP_SEL(reg);
        // if ack is wait, retry it
        for(int i = 0; ; i++)
        {
            // switch dp bank(write dp.select)
            adi->last.ack = (Ack)adi->swd->Transfer(adi->swd, generatRequest(DebugPort, Write, SELECT), &select);
            // if retry overflow
            if(i >= adi->retry)
                return (int)adi->last.ack;
            // contiue???
            if(WAIT == adi->last.ack)
                continue;
            else if(OK == adi->last.ack)
                break;
            else
                return (int)adi->last.ack;
        }
        // record bank
        adi->last.select = select;
    }
    // the first read is invalid
    // if ack is wait, retry it
    for(int j = 0; j < adi->retry; j++)
    {
        // read register
        adi->last.ack = (Ack)adi->swd->Transfer(adi->swd, generatRequest(AccessPort, Read, reg), value);
        if(WAIT == adi->last.ack)
            continue;
        else
            break;
    }
    // check ack
    if(OK != adi->last.ack)
        return (int)adi->last.ack;
    // if ack is wait, retry it
    for(int k = 0; k < adi->retry; k++)
    {
        // read register
        adi->last.ack = (Ack)adi->swd->Transfer(adi->swd, generatRequest(AccessPort, Read, reg), value);
        if(WAIT == adi->last.ack)
            continue;
        else
            return (int)((OK == adi->last.ack) ? SUCCESS : adi->last.ack);
    }
    // return ack
    return (int)((OK == adi->last.ack) ? SUCCESS : adi->last.ack);
}

/**
 * Write to an access port register
 * @param adi ADI instance
 * @param reg ID of register to write
 * @param value Value to write
 * @return 0 on success, error code otherwise
 */
static int writeAP(struct ADI * adi, APRegister reg, uint32_t value)
{
    // check bank select
    if((AP_BANK(reg) != (adi->last.select & APBANKSEL)) || (AP_SEL(reg) != (adi->last.select & APSEL)))
    {
        uint32_t select = (adi->last.select & (~(APBANKSEL | APSEL))) | AP_BANK(reg) | AP_SEL(reg);
        // if ack is wait, retry it
        for(int i = 0; ; i++)
        {
            // switch dp bank(write dp.select)
            adi->last.ack = (Ack)adi->swd->Transfer(adi->swd, generatRequest(DebugPort, Write, SELECT), &select);
            // if retry overflow
            if(i >= adi->retry)
                return (int)adi->last.ack;
            // contiue???
            if(WAIT == adi->last.ack)
                continue;
            else if(OK == adi->last.ack)
                break;
            else
                return (int)adi->last.ack;
        }
        // record bank
        adi->last.select = select;
    }
    // if ack is wait, retry it
    for(int j = 0; j < adi->retry; j++)
    {
        // read register
        adi->last.ack = (Ack)adi->swd->Transfer(adi->swd, generatRequest(AccessPort, Write, reg), &value);
        if(WAIT == adi->last.ack)
            continue;
        else
            return (int)((OK == adi->last.ack) ? SUCCESS : adi->last.ack);
    }
    // return ack
    return (int)((OK == adi->last.ack) ? SUCCESS : adi->last.ack);  
}

/**
 * Read an 8-bit word from a memory access port register
 * @param adi ADI instance
 * @param address Address to read from
 * @param value Pointer to store value
 * @return 0 on success, error code otherwise
 */
static int readMem8(struct ADI * adi, uint32_t address, uint8_t * value)
{
    int result = 0;
    uint32_t drwValue = 0;
    // Configure CSW for 8-bit access
    if((SIZE_8 != (adi->last.csw & SIZE_MASK)) || ADDRINC_DISABLE != (adi->last.csw & ADDRINC_MASK))
    {
        // write ap.csw
        uint32_t csw = (adi->last.csw & ~(SIZE_MASK | ADDRINC_MASK)) | SIZE_8 | ADDRINC_DISABLE;
        result = adi->writeAP(adi, CSW, csw);
        if(SUCCESS != result)
            return (int)result;
        // record csw
        adi->last.csw = csw;
    }
    // Write address to TAR
    // write ap.tar
    result = adi->writeAP(adi, TAR, address);
    if(SUCCESS != result)
        return (int)result;
    // Read value from DRW
    result = adi->readAP(adi, DRW, &drwValue);
    // value
    * value = (uint8_t)drwValue;
    // return ack
    return result;
}

/**
 * Write an 8-bit word to a memory access port register
 * @param adi ADI instance
 * @param address Address to write to
 * @param value The value to write
 * @return 0 on success, error code otherwise
 */
static int writeMem8(struct ADI * adi, uint32_t address, uint8_t value)
{
    int result = 0;
    uint32_t drwValue = (uint32_t)value;
    // Configure CSW for 8-bit access
    if((SIZE_8 != (adi->last.csw & SIZE_MASK)) || ADDRINC_DISABLE != (adi->last.csw & ADDRINC_MASK))
    {
        // write ap.csw
        uint32_t csw = (adi->last.csw & ~(SIZE_MASK | ADDRINC_MASK)) | SIZE_8 | ADDRINC_DISABLE;
        result = adi->writeAP(adi, CSW, csw);
        if(SUCCESS != result)
            return (int)result;
        // record csw
        adi->last.csw = csw;
    }
    // Write address to TAR
    // write ap.tar
    result = adi->writeAP(adi, TAR, address);
    if(SUCCESS != result)
        return (int)result;
    // Write value to DRW and return ack
    return adi->writeAP(adi, DRW, drwValue);
}

/**
 * Read a 16-bit word from a memory access port register
 * @param adi ADI instance
 * @param address Address to read from
 * @param value Pointer to store value
 * @return 0 on success, error code otherwise
 */
static int readMem16(struct ADI * adi, uint32_t address, uint16_t * value) 
{
    int result = 0;
    uint32_t drwValue = 0;
    // Configure CSW for 16-bit access
    if((SIZE_16 != (adi->last.csw & SIZE_MASK)) || ADDRINC_DISABLE != (adi->last.csw & ADDRINC_MASK))
    {
        // write ap.csw
        uint32_t csw = (adi->last.csw & ~(SIZE_MASK | ADDRINC_MASK)) | SIZE_16 | ADDRINC_DISABLE;
        result = adi->writeAP(adi, CSW, csw);
        if(SUCCESS != result)
            return (int)result;
        // record csw
        adi->last.csw = csw;
    }
    // Write address to TAR
    // write ap.tar
    result = adi->writeAP(adi, TAR, address);
    if(SUCCESS != result)
        return (int)result;
    // Read value from DRW
    result = adi->readAP(adi, DRW, &drwValue);
    // value
    * value = (uint16_t)drwValue;
    // return ack
    return result;
}

/**
 * Write a 16-bit word to a memory access port register
 * @param adi ADI instance
 * @param address Address to write to
 * @param value The value to write
 * @return 0 on success, error code otherwise
 */
static int writeMem16(struct ADI * adi, uint32_t address, uint16_t value)
{
    int result = 0;
    uint32_t drwValue = (uint32_t)value;
    // Configure CSW for 16-bit access
    if((SIZE_16 != (adi->last.csw & SIZE_MASK)) || ADDRINC_DISABLE != (adi->last.csw & ADDRINC_MASK))
    {
        // write ap.csw
        uint32_t csw = (adi->last.csw & ~(SIZE_MASK | ADDRINC_MASK)) | SIZE_16 | ADDRINC_DISABLE;
        result = adi->writeAP(adi, CSW, csw);
        if(SUCCESS != result)
            return (int)result;
        // record csw
        adi->last.csw = csw;
    }
    // Write address to TAR
    // write ap.tar
    result = adi->writeAP(adi, TAR, address);
    if(SUCCESS != result)
        return (int)result;
    // Write value to DRW
    return adi->writeAP(adi, DRW, drwValue);
}

/**
 * Read a 32-bit word from a memory access port register
 * @param adi ADI instance
 * @param address Address to read from
 * @param value Pointer to store value
 * @return 0 on success, error code otherwise
 */
static int readMem32(struct ADI * adi, uint32_t address, uint32_t * value)
{
    int result = 0;
    uint32_t drwValue = 0;
    // Configure CSW for 32-bit access
    if((SIZE_32 != (adi->last.csw & SIZE_MASK)) || ADDRINC_DISABLE != (adi->last.csw & ADDRINC_MASK))
    {
        // write ap.csw
        uint32_t csw = (adi->last.csw & ~(SIZE_MASK | ADDRINC_MASK)) | SIZE_32 | ADDRINC_DISABLE;
        result = adi->writeAP(adi, CSW, csw);
        if(SUCCESS != result)
            return (int)result;
        // record csw
        adi->last.csw = csw;
    }
    // Write address to TAR
    // write ap.tar
    result = adi->writeAP(adi, TAR, address);
    if(SUCCESS != result)
        return (int)result;
    // Read value from DRW
    result = adi->readAP(adi, DRW, &drwValue);
    // value
    * value = drwValue;
    // return ack
    return result;
}

/**
 * Write a 32-bit word to a memory access port register
 * @param adi ADI instance
 * @param address Address to write to
 * @param value The value to write
 * @return 0 on success, error code otherwise
 */
static int writeMem32(struct ADI * adi, uint32_t address, uint32_t value) 
{
    int result = 0;
    uint32_t drwValue = value;
    // Configure CSW for 32-bit access
    if((SIZE_32 != (adi->last.csw & SIZE_MASK)) || ADDRINC_DISABLE != (adi->last.csw & ADDRINC_MASK))
    {
        // write ap.csw
        uint32_t csw = (adi->last.csw & ~(SIZE_MASK | ADDRINC_MASK)) | SIZE_32 | ADDRINC_DISABLE;
        result = adi->writeAP(adi, CSW, csw);
        if(SUCCESS != result)
            return (int)result;
        // record csw
        adi->last.csw = csw;
    }
    // Write address to TAR
    // write ap.tar
    result = adi->writeAP(adi, TAR, address);
    if(SUCCESS != result)
        return (int)result;
    // Write value to DRW
    return adi->writeAP(adi, DRW, drwValue);
}

/**
 * Read a block of 32-bit words from a memory access port register
 * @param adi ADI instance
 * @param address Address to read from
 * @param bytes The bytes of values to read
 * @param buffer Buffer to store read values
 * @return 0 on success, error code otherwise
 */
static int readBlock(struct ADI * adi, uint32_t address, size_t bytes, uint32_t * buffer)
{
    int result = 0;
    // bytes and address must be aligned 4
    if(bytes % 4UL || address % 4UL)
        return (int)AlignedError;
    // Configure CSW for 32-bit access with auto-increment
    if((SIZE_32 | ADDRINC_SINGLE) != (adi->last.csw & (SIZE_MASK | ADDRINC_MASK)))
    {
        // write ap.csw
        uint32_t csw = (adi->last.csw & ~(SIZE_MASK | ADDRINC_MASK)) | (SIZE_32 | ADDRINC_SINGLE);
        result = adi->writeAP(adi, CSW, csw);
        if(SUCCESS != result)
            return (int)result;
        // record csw
        adi->last.csw = csw;
    }
    // Write address to TAR
    // write ap.tar
    result = adi->writeAP(adi, TAR, address);
    if(SUCCESS != result)
        return (int)result;
    // the first read is invalid
    // if ack is wait, retry it
    for(int j = 0; j < adi->retry; j++)
    {
        // read register
        adi->last.ack = (Ack)adi->swd->Transfer(adi->swd, generatRequest(AccessPort, Read, DRW), buffer);
        if(WAIT == adi->last.ack)
            continue;
        else
            break;
    }
    // check ack
    if(OK != adi->last.ack)
        return (int)adi->last.ack;
    // read loop
    int i = 0UL;
    for( ; i < (bytes / 4UL) - 1UL; i++)
    {
        // if ack is wait, retry it
        for(int k = 0; k < adi->retry; k++)
        {
            // read register
            adi->last.ack = (Ack)adi->swd->Transfer(adi->swd, generatRequest(AccessPort, Read, DRW), &buffer[i]);
            if(WAIT == adi->last.ack)
                continue;
            else if(OK == adi->last.ack)
                break;
            else
                return (int)adi->last.ack;
        }
        // if retry overflow
        if(OK != adi->last.ack)
            return (int)adi->last.ack;
    }
    // read the last
    return adi->readDP(adi, RDBUFF, &buffer[i]);
}

/**
 * Write a block of 32-bit words to a memory access port register
 * @param adi ADI instance
 * @param address Address to write to
 * @param buffer The values to write
 * @param bytes Number bytes of values
 * @return 0 on success, error code otherwise
 */
static int writeBlock(struct ADI * adi, uint32_t address, const uint32_t * buffer, size_t bytes)
{
    int result = 0;
    // bytes and address must be aligned 4
    if(bytes % 4UL || address % 4UL)
        return (int)AlignedError;
    // Configure CSW for 32-bit access with auto-increment
    if((SIZE_32 | ADDRINC_SINGLE) != (adi->last.csw & (SIZE_MASK | ADDRINC_MASK)))
    {
        // write ap.csw
        uint32_t csw = (adi->last.csw & ~(SIZE_MASK | ADDRINC_MASK)) | (SIZE_32 | ADDRINC_SINGLE);
        result = adi->writeAP(adi, CSW, csw);
        if(SUCCESS != result)
            return (int)result;
        // record csw
        adi->last.csw = csw;
    }
    // Write address to TAR
    // write ap.tar
    result = adi->writeAP(adi, TAR, address);
    if(SUCCESS != result)
        return (int)result;
    // Write block of value
    for(int i = 0; i < bytes / 4UL; i++)
    {
        // write ap.drw
        result = adi->writeAP(adi, DRW, (uint32_t)buffer[i]);
        if(SUCCESS != result)
            return (int)result;
    }
    // return ack
    return (int)SUCCESS;
}

/**
 * Read a block of bytes from a memory access port register
 * @param adi ADI instance
 * @param address Address to read from
 * @param bytes The bytes of bytes to read
 * @param buffer Buffer to store read values
 * @return 0 on success, error code otherwise
 */
static int readBytes(struct ADI * adi, uint32_t address, size_t bytes, uint8_t * buffer)
{
    int result = 0;
    uint32_t value = 0UL;
    // Configure CSW for 8-bit access with auto-increment
    if((SIZE_8 | ADDRINC_SINGLE) != (adi->last.csw & (SIZE_MASK | ADDRINC_MASK)))
    {
        // write ap.csw
        uint32_t csw = (adi->last.csw & ~(SIZE_MASK | ADDRINC_MASK)) | (SIZE_8 | ADDRINC_SINGLE);
        result = adi->writeAP(adi, CSW, csw);
        if(SUCCESS != result)
            return (int)result;
        // record csw
        adi->last.csw = csw;
    }
    // Write address to TAR
    // write ap.tar
    result = adi->writeAP(adi, TAR, address);
    if(SUCCESS != result)
        return (int)result;
    // the first read is invalid
    // if ack is wait, retry it
    for(int j = 0; j < adi->retry; j++)
    {
        // read register
        adi->last.ack = (Ack)adi->swd->Transfer(adi->swd, generatRequest(AccessPort, Read, DRW), &value);
        if(WAIT == adi->last.ack)
            continue;
        else
            break;
    }
    // check ack
    if(OK != adi->last.ack)
        return (int)adi->last.ack;
    // read loop
    int i = 0UL;
    for( ; i < (bytes / 4UL) - 1UL; i++)
    {
        // if ack is wait, retry it
        for(int k = 0; k < adi->retry; k++)
        {
            // read register
            adi->last.ack = (Ack)adi->swd->Transfer(adi->swd, generatRequest(AccessPort, Read, DRW), &value);
            if(WAIT == adi->last.ack)
                continue;
            else if(OK == adi->last.ack)
                break;
            else
                return (int)adi->last.ack;
        }
        // if retry overflow
        if(OK != adi->last.ack)
            return (int)adi->last.ack;
        // value
        buffer[i] = value;
    }
    // read the last
    result = adi->readDP(adi, RDBUFF, &value);
    // value
    buffer[i] = value;
    // return ack
    return result;
}

/**
 * Write a block of bytes to a memory access port register
 * @param adi ADI instance
 * @param address Address to write to
 * @param buffer The values to write
 * @param bytes Number of bytes
 * @return 0 on success, error code otherwise
 */
static int writeBytes(struct ADI * adi, uint32_t address, const uint8_t * buffer, size_t bytes)
{
    int result = 0;
    uint32_t value = 0UL;
    // Configure CSW for 8-bit access with auto-increment
    if((SIZE_8 | ADDRINC_SINGLE) != (adi->last.csw & (SIZE_MASK | ADDRINC_MASK)))
    {
        // write ap.csw
        uint32_t csw = (adi->last.csw & ~(SIZE_MASK | ADDRINC_MASK)) | (SIZE_8 | ADDRINC_SINGLE);
        result = adi->writeAP(adi, CSW, csw);
        if(SUCCESS != result)
            return (int)result;
        // record csw
        adi->last.csw = csw;
    }
    // Write address to TAR
    // write ap.tar
    result = adi->writeAP(adi, TAR, address);
    if(SUCCESS != result)
        return (int)result;
    // Write block of value
    for(int i = 0; i < bytes / 4UL; i++)
    {
        value = (uint32_t)buffer[i];
        // write ap.drw
        result = adi->writeAP(adi, DRW, value);
        if(SUCCESS != result)
            return (int)result;
    }
    // return ack
    return (int)SUCCESS;
}

/**
 * Fast fill memory
 * @param adi ADI instance
 * @param address Address to fill to start
 * @param value The values to write
 * @param bytes Number of bytes
 * @param width @ref bitWidth
 * @return 0 on success, error code otherwise
 */
static int fillMem(struct ADI * adi, uint32_t address, const uint32_t value, size_t bytes, bitWidth width)
{
    int result = 0;
    uint32_t aligned = 1UL;
    uint32_t recover_ctrlstat = adi->last.ctrlstatus;
    // if MINDP???
    if(MINDP_MASK == (adi->last.idcode & MINDP_MASK))
        return isMINDP;
    // muet be aligned
    aligned = 1UL << width;
    // set csw for 'width' access with auto-increment
    if((width | ADDRINC_SINGLE) != (adi->last.csw & (SIZE_MASK | ADDRINC_MASK)))
    {
        // write ap.csw
        uint32_t csw = (adi->last.csw & ~(SIZE_MASK | ADDRINC_MASK)) | (width | ADDRINC_SINGLE);
        result = adi->writeAP(adi, CSW, csw);
        if(SUCCESS != result)
            return (int)result;
        // record csw
        adi->last.csw = csw;
    }
    uint32_t trncnt = bytes / aligned;
    // once transfer numbers is 4096!!!
    int i = 0;
    do{
        uint32_t adr = address;
        uint32_t num = 0UL;
        uint32_t ctrlstat = adi->last.ctrlstatus;
        // if need multiple
        if(trncnt > ((TRNCNT >> 12) + 1UL))
            num = ((TRNCNT >> 12) + 1UL);
        else
            num = trncnt;
        // Write address to TAR. must set address first!!!
        adr = adr + i * aligned;
        result = adi->writeAP(adi, TAR, adr);
        if(SUCCESS != result)
            return (int)result;
        // set CTRL/STAT.trncnt
        ctrlstat &= ~(TRNCNT | MASKLANE);
        ctrlstat |= ((num - 1) << 12);
        ctrlstat |= MASKLANE;
        result = adi->writeDP(adi, CTRL_STAT, ctrlstat);
        if(SUCCESS != result)
            return (int)result;
        // record ctrlstate
        adi->last.ctrlstatus = ctrlstat;        
        // write value to DRW
        result = adi->writeAP(adi, DRW, value);
        if(SUCCESS != result)
            return (int)result;
        // wait finish or occure error
        do {
            // wait some us
            volatile uint32_t timecnt = 100UL;
            do { timecnt--; } while(timecnt != 0UL);
            // read dp.ctrlstat
            result = adi->readDP(adi, CTRL_STAT, &adi->last.ctrlstatus);
            if(SUCCESS != result)
                return (int)result;
            if(0UL != (adi->last.ctrlstatus & (STICKYORUN | STICKYCMP | STICKYERR | WDATAERR )))
                return (int)CTRLSTATError;
        } while(0UL != (adi->last.ctrlstatus & TRNCNT));
        // step
        i += num;
        trncnt -= num;
    }while(0 != trncnt);
    // recover ctrlstat
    result = adi->writeDP(adi, CTRL_STAT, recover_ctrlstat);
    if(SUCCESS != result)
        return (int)result;
    // record ctrlstate
    adi->last.ctrlstatus = recover_ctrlstat;
    // return ack
    return (int)SUCCESS;
}

/**
 * Fast fill memory with a 8-bit word
 * @param adi ADI instance
 * @param address Address to fill to start
 * @param value The values to write
 * @param bytes Number of bytes
 * @return 0 on success, error code otherwise
 */
static int fillMem8(struct ADI * adi, uint32_t address, const uint8_t value, size_t bytes)
{
    // fast fill memory with byte
    return fillMem(adi, address, (const uint32_t)value, bytes, BYTE_WIDE);
}

/**
 * Fast fill memory with a 16-bit word
 * @param adi ADI instance
 * @param address Address to fill to start
 * @param value The values to write
 * @param bytes Number of bytes
 * @return 0 on success, error code otherwise
 */
static int fillMem16(struct ADI * adi, uint32_t address, const uint16_t value, size_t bytes)
{
    // bytes and address must be aligned 2
    if(((bytes % 2) != 0) || ((address % 2) != 0))
        return (int)AlignedError;
    // fast fill memory with half word
    return fillMem(adi, address, (const uint16_t)value, bytes, HALF_WORD_WIDE);
}

/**
 * Fast fill memory with a 32-bit word
 * @param adi ADI instance
 * @param address Address to fill to start
 * @param value The values to write
 * @param bytes Number of bytes
 * @return 0 on success, error code otherwise
 */
static int fillMem32(struct ADI * adi, uint32_t address, const uint32_t value, size_t bytes)
{
    // bytes and address must be aligned 4
    if(((bytes % 4) != 0) || ((address % 4) != 0))
        return (int)AlignedError;
    // fast fill memory with word
    return fillMem(adi, address, value, bytes, HALF_WORD_WIDE);
}

/**
 * Push verify (values unmatch, STICKYCMP = 1) [Note: At this moment, reading of the AP operation is not permitted!!!]
 * @param adi ADI instance
 * @param address start address when be called, and unmatch address when the values unmatch
 * @param buffer The values buffer to verify
 * @param bytes Number of bytes
 * @param bitwidth @ref bitWidth
 * @param lane byte-lane @ref ByteLane
 * @return 0 on success, error code otherwise
 */
static int pushVerify(struct ADI * adi, uint32_t * address, const uint32_t * buffer, size_t bytes, bitWidth width, ByteLane lane)
{
    int result = 0;
    uint32_t aligned = 1UL;
    uint32_t recover_ctrlstat = adi->last.ctrlstatus;
    // if MINDP???
    if(MINDP_MASK == (adi->last.idcode & MINDP_MASK))
        return isMINDP;
    // muet be aligned
    aligned = 1UL << width;
    // set csw for 'width' access with auto-increment
    if((width | ADDRINC_SINGLE) != (adi->last.csw & (SIZE_MASK | ADDRINC_MASK)))
    {
        // write ap.csw
        uint32_t csw = (adi->last.csw & ~(SIZE_MASK | ADDRINC_MASK)) | (width | ADDRINC_SINGLE);
        result = adi->writeAP(adi, CSW, csw);
        if(SUCCESS != result)
            return (int)result;
        // record csw
        adi->last.csw = csw;
    }
    uint32_t trncnt = bytes / aligned;
    // once transfer numbers is 4096!!!
    int i = 0;
    do{
        uint32_t adr = * address;
        uint32_t num = 0UL;
        uint32_t ctrlstat = adi->last.ctrlstatus;
        // if need multiple
        if(trncnt > ((TRNCNT >> 12) + 1UL))
            num = ((TRNCNT >> 12) + 1UL);
        else
            num = trncnt;
        // Write address to TAR. must set address first!!!
        adr = adr + i * aligned;
        result = adi->writeAP(adi, TAR, adr);
        if(SUCCESS != result)
            return (int)result;
        // set CTRL/STAT.trncnt with TRNMODE "pushed-verify" mode
        ctrlstat &= ~(TRNCNT | MASKLANE | TRNMODE);
        // ctrlstat |= ((num - 1) << 12);   // if pushed-find, use it
        ctrlstat |= lane;
        ctrlstat |= TRNMODE_PUSH_VERIFY;
        result = adi->writeDP(adi, CTRL_STAT, ctrlstat);
        if(SUCCESS != result)
            return (int)result;
        // record ctrlstate
        adi->last.ctrlstatus = ctrlstat;
        // loop write and verify
        for(int m = 0; m < num; m++)
        {
            uint32_t value = 0UL;
            switch(width)
            {
                case BYTE_WIDE: value = ((uint8_t *)buffer)[m]; break;
                case HALF_WORD_WIDE: value = ((uint16_t *)buffer)[m]; break;
                case WORD_WIDE:
                default: value = buffer[m]; break;
            }
            // write value to DRW
            adi->writeAP(adi, DRW, value);
            if(OK == adi->last.ack)
                continue;
            else if(FAULT == adi->last.ack)
                break;
            else
                return (int)adi->last.ack;
        }
        // read dp.ctrlstat and check error
        result = adi->readDP(adi, CTRL_STAT, &adi->last.ctrlstatus);
        if(SUCCESS != result)
            return (int)result;
        if(0UL != (adi->last.ctrlstatus & (STICKYORUN | STICKYERR | WDATAERR)))
            return (int)CTRLSTATError;
        // check resault
        if(STICKYCMP == (adi->last.ctrlstatus & STICKYCMP))
        {
            // clear STKCMPCLR flag
            result = adi->writeDP(adi, ABORT, STKCMPCLR);
            if(SUCCESS != result)
                return (int)result;
            // recover ctrlstat
            result = adi->writeDP(adi, CTRL_STAT, recover_ctrlstat);
            if(SUCCESS != result)
                return (int)result;
            // record ctrlstate
            adi->last.ctrlstatus = recover_ctrlstat;
            // calculated address
            result = adi->readAP(adi, TAR, address);
            if(SUCCESS != result)
                return (int)result;
            // record adr
            * address -= aligned;
            // return ack
            return (int)PushVerify;
        }
        // step
        i += num;
        trncnt -= num;
    }while(0 != trncnt);
    // recover ctrlstat
    result = adi->writeDP(adi, CTRL_STAT, recover_ctrlstat);
    if(SUCCESS != result)
        return (int)result;
    // record ctrlstate
    adi->last.ctrlstatus = recover_ctrlstat;
    // return ack
    return (int)SUCCESS;
}

/**
 * Push verify with 8-bit words
 * @param adi ADI instance
 * @param address start address when be called, and unmatch address when the values unmatch
 * @param buffer The values buffer to verify
 * @param bytes Number of bytes
 * @return 0 on success, error code otherwise
 */
static int pushVerify8(struct ADI * adi, uint32_t * address, const uint8_t * buffer, size_t bytes)
{
    return pushVerify(adi, address, (const uint32_t *)buffer, bytes, BYTE_WIDE, LANE_0);
}

/**
 * Push verify with 16-bit words
 * @param adi ADI instance
 * @param address start address when be called, and unmatch address when the values unmatch
 * @param buffer The values buffer to verify
 * @param bytes Number of bytes
 * @param lane byte-lane @ref ByteLane
 * @return 0 on success, error code otherwise
 */
static int pushVerify16(struct ADI * adi, uint32_t * address, const uint16_t * buffer, size_t bytes, ByteLane lane)
{
    return pushVerify(adi, address, (const uint32_t *)buffer, bytes, HALF_WORD_WIDE, lane);
}

/**
 * Push verify with 32-bit words
 * @param adi ADI instance
 * @param address start address when be called, and unmatch address when the values unmatch
 * @param buffer The values buffer to verify
 * @param bytes Number of bytes
 * @param lane byte-lane @ref ByteLane
 * @return 0 on success, error code otherwise
 */
static int pushVerify32(struct ADI * adi, uint32_t * address, const uint32_t * buffer, size_t bytes, ByteLane lane)
{
    return pushVerify(adi, address, buffer, bytes, WORD_WIDE, lane);
}

/**
 * Push compare (values match, STICKYCMP = 1) [Note: At this moment, reading of the AP operation is not permitted!!!]
 * @param adi ADI instance
 * @param address start address when be called, and match address when the values match
 * @param value The values to compare(find)
 * @param bytes Number of bytes
 * @param bitwidth @ref bitWidth
 * @param lane byte-lane @ref ByteLane
 * @return 0 on success, error code otherwise
 */
static int pushCompare(struct ADI * adi, uint32_t * address, const uint32_t value, size_t bytes, bitWidth width, ByteLane lane)
{
    int result = 0;
    uint32_t aligned = 1UL;
    uint32_t recover_ctrlstat = adi->last.ctrlstatus;
    // if MINDP???
    if(MINDP_MASK == (adi->last.idcode & MINDP_MASK))
        return isMINDP;
    // muet be aligned
    aligned = 1UL << width;
    // set csw for 'width' access with auto-increment
    if((width | ADDRINC_SINGLE) != (adi->last.csw & (SIZE_MASK | ADDRINC_MASK)))
    {
        // write ap.csw
        uint32_t csw = (adi->last.csw & ~(SIZE_MASK | ADDRINC_MASK)) | (width | ADDRINC_SINGLE);
        result = adi->writeAP(adi, CSW, csw);
        if(SUCCESS != result)
            return (int)result;
        // record csw
        adi->last.csw = csw;
    }
    uint32_t trncnt = bytes / aligned;
    // once transfer numbers is 4096!!!
    int i = 0;
    do{
        uint32_t adr = * address;
        uint32_t num = 0UL;
        uint32_t ctrlstat = adi->last.ctrlstatus;
        // if need multiple
        if(trncnt > ((TRNCNT >> 12) + 1UL))
            num = ((TRNCNT >> 12) + 1UL);
        else
            num = trncnt;
        // Write address to TAR. must set address first!!!
        adr = adr + i * aligned;
        result = adi->writeAP(adi, TAR, adr);
        if(SUCCESS != result)
            return (int)result;
        // set CTRL/STAT.trncnt with TRNMODE "pushed-compare" mode
        ctrlstat &= ~(TRNCNT | MASKLANE | TRNMODE);
        ctrlstat |= ((num - 1) << 12);   // if pushed-find, use it
        ctrlstat |= lane;
        ctrlstat |= TRNMODE_PUSH_VERIFY;
        result = adi->writeDP(adi, CTRL_STAT, ctrlstat);
        if(SUCCESS != result)
            return (int)result;
        // record ctrlstate
        adi->last.ctrlstatus = ctrlstat;
        // write value to DRW
        result = adi->writeAP(adi, DRW, value);
        if(SUCCESS != result)
            return (int)result;
        // wait finish
        do{
            // read dp.ctrlstat and check error
            result = adi->readDP(adi, CTRL_STAT, &adi->last.ctrlstatus);
            if(SUCCESS != result)
                return (int)result;
            if(0UL != (adi->last.ctrlstatus & (STICKYORUN | STICKYERR | WDATAERR)))
                return (int)CTRLSTATError;
            // check resault
            if(STICKYCMP == (adi->last.ctrlstatus & STICKYCMP))
            {
                // clear STKCMPCLR flag
                result = adi->writeDP(adi, ABORT, STKCMPCLR);
                if(SUCCESS != result)
                    return (int)result;
                // recover ctrlstat
                result = adi->writeDP(adi, CTRL_STAT, recover_ctrlstat);
                if(SUCCESS != result)
                    return (int)result;
                // record ctrlstate
                adi->last.ctrlstatus = recover_ctrlstat;
                // calculated address
                result = adi->readAP(adi, TAR, address);
                if(SUCCESS != result)
                    return (int)result;
                // record adr
                * address -= aligned;
                // return ack
                return (int)SUCCESS;
            }
        } while(0UL != (adi->last.ctrlstatus & TRNCNT));
        // step
        i += num;
        trncnt -= num;
    }while(0 != trncnt);
    // recover ctrlstat
    result = adi->writeDP(adi, CTRL_STAT, recover_ctrlstat);
    if(SUCCESS != result)
        return (int)result;
    // record ctrlstate
    adi->last.ctrlstatus = recover_ctrlstat;
    // return ack
    return (int)PushCompare;
}

/**
 * Push compare with 8-bit words
 * @param adi ADI instance
 * @param address start address when be called, and match address when the values match
 * @param value The values to compare(find)
 * @param bytes Number of bytes
 * @return 0 on success, error code otherwise
 */
static int pushCompare8(struct ADI * adi, uint32_t * address, const uint8_t value, size_t bytes)
{
    return pushCompare(adi, address, (const uint32_t)value, bytes, BYTE_WIDE, LANE_0);
}

/**
 * Push compare with 16-bit words
 * @param adi ADI instance
 * @param address start address when be called, and match address when the values match
 * @param value The values to compare(find)
 * @param bytes Number of bytes
 * @param lane byte-lane @ref ByteLane
 * @return 0 on success, error code otherwise
 */
static int pushCompare16(struct ADI * adi, uint32_t * address, const uint16_t value, size_t bytes, ByteLane lane)
{
    return pushCompare(adi, address, (const uint32_t)value, bytes, HALF_WORD_WIDE, lane);
}

/**
 * Push compare with 32-bit words
 * @param adi ADI instance
 * @param address start address when be called, and match address when the values match
 * @param value The values to compare(find)
 * @param bytes Number of bytes
 * @param lane byte-lane @ref ByteLane
 * @return 0 on success, error code otherwise
 */
static int pushCompare32(struct ADI * adi, uint32_t * address, const uint32_t value, size_t bytes, ByteLane lane)
{
    return pushCompare(adi, address, value, bytes, WORD_WIDE, lane);
}

/**
 * Find a 8-bit word in memory
 * @param adi ADI instance
 * @param address start address when be called, and match address when the values match
 * @param value The values to find
 * @param bytes Number of bytes
 * @return 0 on success, error code otherwise
 */
static int findMem8(struct ADI * adi, uint32_t * address, const uint8_t value, size_t bytes)
{
    return adi->pushCompare8(adi, address, value, bytes);
}

/**
 * Find a 16-bit word in memory
 * @param adi ADI instance
 * @param address start address when be called, and match address when the values match
 * @param value The values to find
 * @param bytes Number of bytes
 * @param lane byte-lane @ref ByteLane
 * @return 0 on success, error code otherwise
 */
static int findMem16(struct ADI * adi, uint32_t * address, const uint16_t value, size_t bytes, ByteLane lane)
{
    return adi->pushCompare16(adi, address, value, bytes, lane);
}

/**
 * Find a 32-bit word in memory
 * @param adi ADI instance
 * @param address start address when be called, and match address when the values match
 * @param value The values to compare(find)
 * @param bytes Number of bytes
 * @param lane byte-lane @ref ByteLane
 * @return 0 on success, error code otherwise
 */
static int findMem32(struct ADI * adi, uint32_t * address, const uint32_t value, size_t bytes, ByteLane lane)
{
    return adi->pushCompare16(adi, address, value, bytes, lane);
}

/**
 * Poll until the value of address is equal [value]
 * @param adi ADI instance
 * @param address address of polling
 * @param value The values to polling
 * @param bitwidth @ref bitWidth
 * @param lane byte-lane @ref ByteLane
 * @param ms used by call-back "isTimeOut"
 * @param isTimeOut Determine if it is overdue, return "0" is timeout.
 * @return 0 on success, error code otherwise
 */
static int pollUntil(struct ADI * adi, uint32_t address, const uint32_t value, bitWidth width, ByteLane lane, uint32_t ms, bool (* isTimeOut)(uint32_t ms))
{
    int result = 0;
    uint32_t aligned = 1UL;
    uint32_t recover_ctrlstat = adi->last.ctrlstatus;
    // if MINDP???
    if(MINDP_MASK == (adi->last.idcode & MINDP_MASK))
        return isMINDP;
    // if isTimeOut = NULL
    if(NULL == isTimeOut)
        return ParamError;
    // set csw for 'width' access with auto-increment
    if((width | ADDRINC_DISABLE) != (adi->last.csw & (SIZE_MASK | ADDRINC_MASK)))
    {
        // write ap.csw
        uint32_t csw = (adi->last.csw & ~(SIZE_MASK | ADDRINC_MASK)) | (width | ADDRINC_DISABLE);
        result = adi->writeAP(adi, CSW, csw);
        if(SUCCESS != result)
            return (int)result;
        // record csw
        adi->last.csw = csw;
    }
    // Write address to TAR. must set address first!!!
    result = adi->writeAP(adi, TAR, address);
    if(SUCCESS != result)
        return (int)result;
    // set CTRL/STAT.trncnt with TRNMODE "pushed-verify" mode
    uint32_t ctrlstat = adi->last.ctrlstatus;
    ctrlstat &= ~(TRNCNT | MASKLANE | TRNMODE);
    // ctrlstat |= ((num - 1) << 12);   // if pushed-find, use it
    ctrlstat |= lane;
    ctrlstat |= TRNMODE_PUSH_VERIFY;
    result = adi->writeDP(adi, CTRL_STAT, ctrlstat);
    if(SUCCESS != result)
        return (int)result;
    // record ctrlstate
    adi->last.ctrlstatus = ctrlstat;
    // polling
    do{
        // write value to DRW
        adi->writeAP(adi, DRW, value);
        if(OK == adi->last.ack)
            continue;
        else if(FAULT == adi->last.ack)
            break;
        else
            return (int)adi->last.ack;
        // read dp.ctrlstat and check error
        result = adi->readDP(adi, CTRL_STAT, &adi->last.ctrlstatus);
        if(SUCCESS != result)
            return (int)result;
        if(0UL != (adi->last.ctrlstatus & (STICKYORUN | STICKYERR | WDATAERR)))
            return (int)CTRLSTATError;
        // check resault
        if(STICKYCMP == (adi->last.ctrlstatus & STICKYCMP))
        {
            // clear STKCMPCLR flag
            result = adi->writeDP(adi, ABORT, STKCMPCLR);
            if(SUCCESS != result)
                return (int)result;
            // recover ctrlstat
            result = adi->writeDP(adi, CTRL_STAT, recover_ctrlstat);
            if(SUCCESS != result)
                return (int)result;
            // record ctrlstate
            adi->last.ctrlstatus = recover_ctrlstat;
            // return ack
            return (int)SUCCESS;
        }
    }while(false != isTimeOut(ms));
    // recover ctrlstat
    result = adi->writeDP(adi, CTRL_STAT, recover_ctrlstat);
    if(SUCCESS != result)
        return (int)result;
    // record ctrlstate
    adi->last.ctrlstatus = recover_ctrlstat;
    // return ack
    return (int)PollTimout;
}

/**
 * Create a new ADI instance
 * @param adi ADI instance
 * @param swd Generate swd waveform interface
 * @param select default value of dp.select used by apsel/apbanksel/dpbanksel
 * @param private_data private_data
 * @return ADI intance, or NULL on error
 */
ADI * adi_create(ADI * adi, SWD * swd, uint32_t select, void * private_data)
{
    const char verison[8] = ADI_Version;
    // check param
    if (NULL == adi)
        return NULL;
    
    // Initialize function pointers
    adi->connect = connect;
    adi->disconnect = disconnect;
    adi->reset = reset;
    adi->readDP = readDP;
    adi->writeDP = writeDP;
    adi->readAP = readAP;
    adi->writeAP = writeAP;
    adi->readMem8 = readMem8;
    adi->writeMem8 = writeMem8;
    adi->readMem16 = readMem16;
    adi->writeMem16 = writeMem16;
    adi->readMem32 = readMem32;
    adi->writeMem32 = writeMem32;
    adi->readBlock = readBlock;
    adi->writeBlock = writeBlock;
    adi->readBytes = readBytes;
    adi->writeBytes = writeBytes;
    // only not mindp
    adi->fillMem8 = fillMem8;
    adi->fillMem16 = fillMem16;
    adi->fillMem32 = fillMem32;
    adi->pushVerify8 = pushVerify8;
    adi->pushVerify16 = pushVerify16;
    adi->pushVerify32 = pushVerify32;
    adi->pushCompare8 = pushCompare8;
    adi->pushCompare16 = pushCompare16;
    adi->pushCompare32 = pushCompare32;
    adi->findMem8 = findMem8;
    adi->findMem16 = findMem16;
    adi->findMem32 = findMem32;
    adi->pollUntil = pollUntil;

    // Set private_data
    adi->private_data = private_data;
    // Set adi version
    adi->adiVer[0] = verison[0];
    adi->adiVer[1] = verison[1];
    adi->adiVer[2] = verison[2];
    adi->adiVer[3] = verison[3];
    adi->adiVer[4] = verison[4];
    adi->adiVer[5] = verison[5];
    adi->adiVer[6] = verison[6];
    adi->adiVer[7] = verison[7];
    // Set retry
    adi->retry = RETRY_CNT;
    // Initialize
    adi->last.idcode = 0UL;
    adi->last.select = select;
    adi->last.ctrlstatus = 0UL;
    adi->last.csw = 0UL;
    adi->last.ack = NO_ACK;
    // Set swd
    adi->swd = swd;

    return adi;
}

/**
 * Destroy a ADI instance
 * @param adi ADI instance
 */
void adi_destroy(ADI * adi)
{
    // do somrthing...
}