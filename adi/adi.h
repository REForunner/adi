
#ifndef ADI_H
#define ADI_H

#ifdef __cplusplus
extern "C" {
#endif

/*

Note:
1. when error ouccered, this program is not write "abort" register to clear error flag.
2. when use pushed-verify or pushed-compare, the AP read is not permitted.
3. pushed-verify operations: if the values match, the STICKYCMP flag is set to 0b1.
4. pushed-compare operations: if the values do not match, the STICKYCMP flag is set to 0b1.
5. when need to write or read idcode/select/ctrlstatus/csw register, please updata the struct of "ADI.last".

*/

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/**
 * Debug Port Registers
 */
typedef enum DPRegister : uint32_t {
    /**
     * AP Abort register, write only
     */
    ABORT = 0x00,
    /**
     * Debug Port Identification register, read only(idcode)
     */
    DPIDR = 0x00,
    IDCODE = 0x00,
    /**
     * Control/Status register, SELECT.DPBANKSEL 0x0
     */
    CTRL_STAT = 0x04,
    /**
     * Data Link Control Register, SELECT.DPBANKSEL 0x1
     */
    DLCR = 0x14,
    /**
     * Read Resend register, read only
     */
    RESEND = 0x08,
    /**
     * AP Select register, write only
     */
    SELECT = 0x08,
    /**
     * Read Buffer register, read only
     */
    RDBUFF = 0x0C,
    // Version 2
    /**
     * Target Identification register, read only, SELECT.DPBANKSEL 0x2
     */
    TARGETID = 0x24,
    /**
     * Data Link Protocol Identification Register, read only, SELECT.DPBANKSEL 0x3
     */
    DLPIDR = 0x34,
    /**
     * Event Status register, read only, SELECT.DPBANKSEL 0x4
     */
    EVENTSTAT = 0x44,
    /**
     * Target Selection, write only
     */
    TARGETSEL = 0x0C

} DPRegister;

/**
 * Abort Register Mask
 */
typedef enum AbortMask : uint32_t {
    /**
     * Generates a DAP abort, that aborts the current AP transaction
     */
    DAPABORT = (1 << 0),
    /**
     * Reserved
     */
    STKCMPCLR = (1 << 1),
    /**
     * Sets the STICKYERR sticky error flag to 0
     */
    STKERRCLR = (1 << 2),
    /**
     * Sets the WDATAERR write data error flag to 0
     */
    WDERRCLR = (1 << 3),
    /**
     * Sets the STICKYORUN overrun error flag to 0
     */
    ORUNERRCLR = (1 << 4)

} AbortMask;

/**
 * Control/Status Register Mask
 */
typedef enum CtrlStatMask : uint32_t {
    /**
     * This bit is set to 1 to enable overrun detection. The reset value is 0
     */
    ORUNDETECT = (1 << 0),
    /**
     * This bit is set to 1 when an overrun occurs, read only
     */
    STICKYORUN = (1 << 1),
    /**
     * This field sets the transfer mode for AP operations
     * 
     * TRNMODE can have one of the following values:
     * 
     * 0b00 Normal operation.
     * 0b01 Pushed-verify mode.
     * 0b10 Pushed-compare mode.
     * 0b11 Reserved.
     */
    TRNMODE = (3 << 2),
    /**
     * Normal operation.
     */
    TRNMODE_NORMAL_OPERATION = (0 << 2),
    /**
     * Pushed-verify mode.
     */
    TRNMODE_PUSH_VERIFY = (1 << 2),
    /**
     * Pushed-compare mode.
     */
    TRNMODE_PUSH_COMPARE = (2 << 2),
    /**
     * Reserved
     */
    STICKYCMP = (1 << 4),
    /**
     * If an error is returned by an access port transaction, this bit is set to 1, read only
     */
    STICKYERR = (1 << 5),
    /**
     * Whether the response to the previous access port read or RDBUFF read was OK, read only
     */
    READOK = (1 << 6),
    /**
     * If a Write Data Error occurs, read only
     */
    WDATAERR = (1 << 7),
    /**
     * The MASKLANE field is used to select the bytes to be included in this comparison
     * 
     * MASKLANE Effect                              Bits included in comparisons
     * 
     * 0b1xxx   Include byte lane 3 in comparisons. Bits[31:24].
     * 0bx1xx   Include byte lane 2 in comparisons. Bits[23:16].
     * 0bxx1x   Include byte lane 1 in comparisons. Bits[15:8].
     * 0bxxx1   Include byte lane 0 in comparisons. Bits[7:0].
     * 
     * a. Whether other bits are included is determined by the other bits of MASKLANE:
     * To compare the whole word, MASKLANE is set to 0b1111 to include all byte lanes. 
     * If a MASKLANE bit is 0b0, the corresponding byte lane is excluded from the comparison. 
     */
    MASKLANE = (15 << 8),
    /**
     * Include byte lane 0
     */
    BYTE_LANE_0 = (1 << 8),
    /**
     * Include byte lane 1
     */
    BYTE_LANE_1 = (2 << 8),    
    /**
     * Include byte lane 2
     */
    BYTE_LANE_2 = (4 << 8),
    /**
     * Include byte lane 3
     */
    BYTE_LANE_3 = (8 << 8),
    /**
     * Transaction counter
     */
    TRNCNT = (0xFFF << 12),
    /**
     * Debug reset request, the reset value is 0
     */
    CDBGRSTREQ = (1 << 26),
    /**
     * Debug reset acknowledge, read only
     */
    CDBGRSTACK = (1 << 27),
    /**
     * Debug powerup request, the reset value is 0
     */
    CDBGPWRUPREQ = (1 << 28),
    /**
     * Debug powerup acknowledge, read only
     */
    CDBGPWRUPACK = (1 << 29),
    /**
     * System powerup request, the reset value is 0
     */
    CSYSPWRUPREQ = (1 << 30),
    /**
     * System powerup acknowledge, read only
     */
    CSYSPWRUPACK = 0x80000000       // (1 << 31)

} CtrlStatMask;

/**
 * Data Link Control register Mask
 */
typedef enum DLCRMask : uint32_t {
    /**
     * For an SW-DP, this field defines the turnaround tristate period
     * After a powerup or line reset, this field is 0b00
     * Turnaround tristate period field, TURNROUND, bit definitions
     * 
     * DLCR.TURNROUND   Turnaround tristate period
     * 0b00             1 data period   a.
     * 0b01             2 data periods  a.
     * 0b10             3 data periods  a.
     * 0b11             4 data periods  a.
     * 
     * a. A data period is the period of a single data bit on the SWD interface.
     */
    TURNROUND = (3 << 8),
    /**
     * 1 data period
     */
    TURNROUND_1_PERIOD = (0 << 8),
    /**
     * 2 data period
     */
    TURNROUND_2_PERIOD = (1 << 8),
    /**
     * 3 data period
     */
    TURNROUND_3_PERIOD = (2 << 8),
    /**
     * 4 data period
     */
    TURNROUND_4_PERIOD = (3 << 8),

} DLCRMask;

/**
 * Bank Select Mask
 */
typedef enum BankSelectMask : uint32_t {
    /**
     * Selects the current access port
     */
    APSEL = 0xFF000000,
    /**
     * Selects the active 4-word register window on the current access port
     */
    APBANKSEL = 0x000000F0,
    /**
     * Selects the register that appears at DP register 0x4
     */
    DPBANKSEL = 0x0000000F

} BankSelectMask;

/**
 * Event Status Mask
 */
typedef enum EventStatMask : uint32_t {
    /**
     * Event status flag, indicates that the processor is halted when set to 0
     */
    EA = 0x01

} EventStatMask;

/**
 * Access Port Registers
 * 
 * Bit field description:
 *      bit[1:0] must be equal to 0!!!
 *      bit[3:2] is A[2:3] in Packet request.
 *      bit[7:4] is the Bank where is located in AP.
 *      bit[15:8] selects the AP with the ID number APSEL.
 */
typedef enum APRegister : uint32_t {
    /**
     * Control/Status Word register
     */
    CSW = 0x0000,
    /**
     * Transfer Address Register
     */
    TAR = 0x0004,
    /**
     * Data Read/Write register
     */
    DRW = 0x000C,
    /**
     * Banked Data register
     */
    BD0 = 0x0010,
    /**
     * Banked Data register
     */
    BD1 = 0x0014,
    /**
     * Banked Data register
     */
    BD2 = 0x0018,
    /**
     * Banked Data register
     */
    BD3 = 0x001C,
    /**
     * Memory Barrier Transfer register
     */
    MBT = 0x0020,
    /**
     * Tag 0 Transfer register
     */
    T0TR = 0x0030,
    /**
     * Configuration register
     */
    CFG1 = 0x00E0,
    /**
     * Debug Base Address register
     * 
     * If the implementation includes the Large Physical Address Extension, 
     * the word at this offset represents the most significant word of the debug 
     * base address
     */
    BASE = 0x00F0,
    /**
     * Configuration register
     */
    CFG = 0x00F4,
    /**
     * Debug Base Address register
     * 
     * If the implementation includes the Large Physical Address Extension, 
     * the word at this offset represents the least significant word of the debug 
     * base address
     */
    ROM = 0x00F8,
    /**
     * Identification Register
     */
    IDR = 0x00FC

} APRegister;

/**
 * Control/Status Word Register Mask
 */
typedef enum CSWMask : uint32_t {
    /**
     * 8 bits
     */
    SIZE_8 = (0 << 0),
    /**
     * 16 bits
     */
    SIZE_16 = (1 << 0),
    /**
     * 32 bits
     */
    SIZE_32 = (2 << 0),
    /**
     * 64 bits
     */
    SIZE_64 = (3 << 0),
    /**
     * 128 bits
     */
    SIZE_128 = (4 << 0),
    /**
     * 256 bits
     */
    SIZE_256 = (5 << 0),
    /**
     * size mask
     */
    SIZE_MASK = (7 << 0),
    /**
     * Auto address increment disable
     */
    ADDRINC_DISABLE = (0 << 4),
    /**
     * Auto address increment single
     */
    ADDRINC_SINGLE = (1 << 4),
    /**
     * Auto address increment packed
     */
    ADDRINC_PACKED = (1 << 5),
    /**
     * Auto address increment mask
     */
    ADDRINC_MASK = (3 << 4),
    /**
     * Indicates the status of the DAPEN port - AHB transfers permitted
     */
    DBGSTATUS = (1 << 6),
    /**
     * Indicates if a transfer is in progress
     */
    TRANSINPROG = (1 << 7),
    /**
     * Reserved
     */
    RESERVED = (1 << 24),
    /**
     * User and Privilege control
     */
    HPROT1 = (1 << 25),
    /**
     * Set to 1 for master type debug
     */
    MASTERTYPE = (1 << 29),
    /**
     * Debug software access enable
     */
    DBGSWENABLE = 0x80000000    // (1 << 31)

} CSWMask;

/**
 * ACK Response
 */
typedef enum Ack : uint32_t {
    /**
     * OK (for SWD protocol), OK or FAULT (for JTAG protocol)
     */
    OK = 0x01,
    /**
     * Wait
     */
    WAIT = 0x02,
    /**
     * Fault
     */
    FAULT = 0x04,
    /**
     * NO_ACK (no response from target)
     */
    NO_ACK = 0x07,
    /**
     * Error， when parity failed
     */
    Error = 0x08,

} Ack;

/**
 * error code
 */
typedef enum ErrorCode : int {
    /**
     * retry overflow
     */
    RETRY_OVERFLOW = -9,
    /**
     * parameter error
     */
    ParamError = -8,
    /**
     * polling timeout
     */
    PollTimout = -7,
    /**
     * push-compare
     */
    PushCompare = -6,
    /**
     * push-verify
     */
    PushVerify = -5,
    /**
     * overrun or an error of an transaction or write data error
     */
    CTRLSTATError = -4,
    /**
     * DBG power up error
     */
    DBGPWRError = -3,
    /**
     * min dp, some function is implemented.
     */
    isMINDP = -2,
    /**
     * bytes or address is not Aligned!!!
     */
    AlignedError = -1,
    /**
     * success
     */
    SUCCESS = 0

} ErrorCode;

/**
 * bit wide
 */
typedef enum bitWidth : uint32_t {
    /**
     * byte width   8-bit
     */
    BYTE_WIDE = SIZE_8,
    /**
     * half word width  16-bit
     */
    HALF_WORD_WIDE = SIZE_16,
    /**
     * word width   32-bit
     */
    WORD_WIDE = SIZE_32

} bitWidth;

/**
 * byte lane mask, when use push-verify or push-compare
 */
typedef enum ByteLane : uint32_t {
    /**
     * Include byte lane 0
     */
    LANE_0 = BYTE_LANE_0,
    /**
     * Include byte lane 1
     */
    LANE_1 = BYTE_LANE_1,
    /**
     * Include byte lane 2
     */
    LANE_2 = BYTE_LANE_2,
    /**
     * Include byte lane 3
     */
    LANE_3 = BYTE_LANE_3,
    /**
     * Include all bytes
     */
    LANE_ALL = MASKLANE

} ByteLane;

/**
 * generate swd waveform interface
 */
typedef struct SWD
{
    /**
     * Private data
     */
    void * private_data;
    
    /**
     * SWD config
     */
    struct {
        /**
         * speed(pin delay or baudrate)
         */
        uint32_t speed;
        /**
         * turn around(default 1, except config in dp.dlcr)
         */
        uint8_t turnaround;
        /**
         * time stamp(capture time, if used)
         */
        uint32_t timestamp;
        /**
         * idle cycle(when swd bus idle, default at least 2???)
         */
        uint8_t idleCycle;
        /**
         * have data phase? when ack is wait or fault
         */
        uint8_t dataPhase;
    } config;
    
    /**
     * Initialize swd port
     * @param swd SWD interface
     */
    void ( * Init)(struct SWD * swd);
        
    /**
     * De-initialize swd port
     * @param swd SWD interface
     */
    void ( * DeInit)(struct SWD * swd);

    /**
     * hardware reset operation
     * @param swd SWD interface
     * @param hwReset reset the target use hardware
     *                  "true" : the hardware pin is catch
     *                  "false" : the hardware pin is release
     */
    void ( * hwReset)(struct SWD * swd, bool hwReset);
    
    /**
     * Generate SWJ/SWD Sequence
     * @param swd SWD interface
     * @param info sequence information
     *             if info = 1: read data (swdi)
     *             if info = 0: write data (swdo)
     * @param bits read/write bit count
     * @param swdio pointer to SWDIO generated data or captured data refer to info
     */
    void ( * Sequence)(struct SWD * swd, uint8_t info, uint32_t bits, uint8_t * swdio);

    /**
     * SWD Transfer I/O
     * @param swd SWD interface
     * @param request packet request(start APnDP RnW A[3:2] parity stop park)
     * @param data DATA[31:0]
     * @return ACK[2:0]
    */
    uint8_t ( * Transfer)(struct SWD * swd, uint32_t request, uint32_t * data);

} SWD;

/**
 * ADI interface
 */
typedef struct ADI 
{ 
    /**
     * Private data
     */
    void * private_data;
 
    /**
     * generate swd waveform interface
     */
    SWD * swd;

    /**
     * is retry, when ack is wait
     */
    uint32_t retry;

    /**
     * supported adi version
     */
    char adiVer[8];
    
    /**
     * record in the last
     */
    struct{
        /**
         * idcode from read DPIDR
         */
        uint32_t idcode;
        /**
         * bank select
         */
        uint32_t select;
        /**
         * Control/Status register
         */
        uint32_t ctrlstatus;
        /**
         * Control/Status Word register
         */
        uint32_t csw;    
        /**
         * acknowledge
         */
        Ack ack;
    } last;

    /**
     * Connect to target device
     * @param adi ADI instance
     * @param idcode idcode for read
     * @return 0 on success, error code otherwise
     */
    int ( * connect)(struct ADI * adi, int * idcode);

    /**
     * Disconnect from target device
     * @param adi ADI instance
     * @return 0 on success, error code otherwise
     */
    int ( * disconnect)(struct ADI * adi);

    /**
     * Reset target device
     * @param adi ADI instance
     * @param hwReset reset the target use hardware
     *                  "true" : the hardware pin is catch
     *                  "false" : the hardware pin is release
     * @return 0 on success, error code otherwise
     */
    int ( * reset)(struct ADI * adi, bool hwReset);

    /**
     * Read from a debug port register
     * @param adi ADI instance
     * @param reg ID of register to read
     * @param value Pointer to store register value
     * @return 0 on success, error code otherwise
     */
    int ( * readDP)(struct ADI * adi, DPRegister reg, uint32_t * value);

    /**
     * Write to a debug port register
     * @param adi ADI instance
     * @param reg ID of register to write
     * @param value Value to write
     * @return 0 on success, error code otherwise
     */
    int ( * writeDP)(struct ADI * adi, DPRegister reg, uint32_t value);
    
    /**
     * Read from an access port register
     * @param adi ADI instance
     * @param reg ID of register to read
     * @param value Pointer to store register value
     * @return 0 on success, error code otherwise
     */
    int ( * readAP)(struct ADI * adi, APRegister reg, uint32_t * value);
    
    /**
     * Write to an access port register
     * @param adi ADI instance
     * @param reg ID of register to write
     * @param value Value to write
     * @return 0 on success, error code otherwise
     */
    int ( * writeAP)(struct ADI * adi, APRegister reg, uint32_t value);
    
    /**
     * Read an 8-bit word from a memory access port register
     * @param adi ADI instance
     * @param address Address to read from
     * @param value Pointer to store value
     * @return 0 on success, error code otherwise
     */
    int ( * readMem8)(struct ADI * adi, uint32_t address, uint8_t * value);

    /**
     * Write an 8-bit word to a memory access port register
     * @param adi ADI instance
     * @param address Address to write to
     * @param value The value to write
     * @return 0 on success, error code otherwise
     */
    int ( * writeMem8)(struct ADI * adi, uint32_t address, uint8_t value);
    
    /**
     * Read a 16-bit word from a memory access port register
     * @param adi ADI instance
     * @param address Address to read from
     * @param value Pointer to store value
     * @return 0 on success, error code otherwise
     */
    int ( * readMem16)(struct ADI * adi, uint32_t address, uint16_t * value);
    
    /**
     * Write a 16-bit word to a memory access port register
     * @param adi ADI instance
     * @param address Address to write to
     * @param value The value to write
     * @return 0 on success, error code otherwise
     */
    int ( * writeMem16)(struct ADI * adi, uint32_t address, uint16_t value);

    /**
     * Read a 32-bit word from a memory access port register
     * @param adi ADI instance
     * @param address Address to read from
     * @param value Pointer to store value
     * @return 0 on success, error code otherwise
     */
    int ( * readMem32)(struct ADI * adi, uint32_t address, uint32_t * value);
    
    /**
     * Write a 32-bit word to a memory access port register
     * @param adi ADI instance
     * @param address Address to write to
     * @param value The value to write
     * @return 0 on success, error code otherwise
     */
    int ( * writeMem32)(struct ADI * adi, uint32_t address, uint32_t value);

    /**
     * Read a block of 32-bit words from a memory access port register
     * @param adi ADI instance
     * @param address Address to read from
     * @param bytes The bytes of values to read
     * @param buffer Buffer to store read values
     * @return 0 on success, error code otherwise
     */
    int ( * readBlock)(struct ADI * adi, uint32_t address, size_t bytes, uint32_t * buffer);

    /**
     * Write a block of 32-bit words to a memory access port register
     * @param adi ADI instance
     * @param address Address to write to
     * @param buffer The values to write
     * @param bytes Number bytes of values
     * @return 0 on success, error code otherwise
     */
    int ( * writeBlock)(struct ADI * adi, uint32_t address, const uint32_t * buffer, size_t bytes);

    /**
     * Read a block of bytes from a memory access port register
     * @param adi ADI instance
     * @param address Address to read from
     * @param bytes The bytes of bytes to read
     * @param buffer Buffer to store read values
     * @return 0 on success, error code otherwise
     */
    int ( * readBytes)(struct ADI * adi, uint32_t address, size_t bytes, uint8_t * buffer);

    /**
     * Write a block of bytes to a memory access port register
     * @param adi ADI instance
     * @param address Address to write to
     * @param buffer The values to write
     * @param bytes Number of bytes
     * @return 0 on success, error code otherwise
     */
    int ( * writeBytes)(struct ADI * adi, uint32_t address, const uint8_t * buffer, size_t bytes);
    
    /**
     * Fast fill memory with a 8-bit word
     * @param adi ADI instance
     * @param address Address to fill to start
     * @param value The values to write
     * @param bytes Number of bytes
     * @return 0 on success, error code otherwise
     */
    int ( * fillMem8)(struct ADI * adi, uint32_t address, const uint8_t value, size_t bytes);
    
    /**
     * Fast fill memory with a 16-bit word
     * @param adi ADI instance
     * @param address Address to fill to start
     * @param value The values to write
     * @param bytes Number of bytes
     * @return 0 on success, error code otherwise
     */
    int ( * fillMem16)(struct ADI * adi, uint32_t address, const uint16_t value, size_t bytes);

    /**
     * Fast fill memory with a 32-bit word
     * @param adi ADI instance
     * @param address Address to fill to start
     * @param value The values to write
     * @param bytes Number of bytes
     * @return 0 on success, error code otherwise
     */
    int ( * fillMem32)(struct ADI * adi, uint32_t address, const uint32_t value, size_t bytes);

    /**
     * Push verify with 8-bit words
     * @param adi ADI instance
     * @param address start address when be called, and unmatch address when the values unmatch
     * @param buffer The values buffer to verify
     * @param bytes Number of bytes
     * @return 0 on success, error code otherwise
     */
    int ( * pushVerify8)(struct ADI * adi, uint32_t * address, const uint8_t * buffer, size_t bytes);

    /**
     * Push verify with 16-bit words
     * @param adi ADI instance
     * @param address start address when be called, and unmatch address when the values unmatch
     * @param buffer The values buffer to verify
     * @param bytes Number of bytes
     * @param lane byte-lane @ref ByteLane
     * @return 0 on success, error code otherwise
     */
    int ( * pushVerify16)(struct ADI * adi, uint32_t * address, const uint16_t * buffer, size_t bytes, ByteLane lane);

    /**
     * Push verify with 32-bit words
     * @param adi ADI instance
     * @param address start address when be called, and unmatch address when the values unmatch
     * @param buffer The values buffer to verify
     * @param bytes Number of bytes
     * @param lane byte-lane @ref ByteLane
     * @return 0 on success, error code otherwise
     */
    int ( * pushVerify32)(struct ADI * adi, uint32_t * address, const uint32_t * buffer, size_t bytes, ByteLane lane);
    
    /**
     * Push compare with 8-bit words
     * @param adi ADI instance
     * @param address start address when be called, and match address when the values match
     * @param value The values to compare(find)
     * @param bytes Number of bytes
     * @return 0 on success, error code otherwise
     */
    int ( * pushCompare8)(struct ADI * adi, uint32_t * address, const uint8_t value, size_t bytes);

    /**
     * Push compare with 16-bit words
     * @param adi ADI instance
     * @param address start address when be called, and match address when the values match
     * @param value The values to compare(find)
     * @param bytes Number of bytes
     * @param lane byte-lane @ref ByteLane
     * @return 0 on success, error code otherwise
     */
    int ( * pushCompare16)(struct ADI * adi, uint32_t * address, const uint16_t value, size_t bytes, ByteLane lane);

    /**
     * Push compare with 32-bit words
     * @param adi ADI instance
     * @param address start address when be called, and match address when the values match
     * @param value The values to compare(find)
     * @param bytes Number of bytes
     * @param lane byte-lane @ref ByteLane
     * @return 0 on success, error code otherwise
     */
    int ( * pushCompare32)(struct ADI * adi, uint32_t * address, const uint32_t value, size_t bytes, ByteLane lane);

    /**
     * Find a 8-bit word in memory
     * @param adi ADI instance
     * @param address start address when be called, and match address when the values match
     * @param value The values to find
     * @param bytes Number of bytes
     * @return 0 on success, error code otherwise
     */
    int ( * findMem8)(struct ADI * adi, uint32_t * address, const uint8_t value, size_t bytes);

    /**
     * Find a 16-bit word in memory
     * @param adi ADI instance
     * @param address start address when be called, and match address when the values match
     * @param value The values to find
     * @param bytes Number of bytes
     * @param lane byte-lane @ref ByteLane
     * @return 0 on success, error code otherwise
     */
    int ( * findMem16)(struct ADI * adi, uint32_t * address, const uint16_t value, size_t bytes, ByteLane lane);

    /**
     * Find a 32-bit word in memory
     * @param adi ADI instance
     * @param address start address when be called, and match address when the values match
     * @param value The values to compare(find)
     * @param bytes Number of bytes
     * @param lane byte-lane @ref ByteLane
     * @return 0 on success, error code otherwise
     */
    int ( * findMem32)(struct ADI * adi, uint32_t * address, const uint32_t value, size_t bytes, ByteLane lane);
    
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
    int ( * pollUntil)(struct ADI * adi, uint32_t address, const uint32_t value, bitWidth width, ByteLane lane, uint32_t ms, bool (* isTimeOut)(uint32_t ms));

} ADI;

/**
 * Create a new ADI instance
 * @param adi ADI instance
 * @param swd Generate swd waveform interface
 * @param select default value of dp.select used by apsel/apbanksel/dpbanksel
 * @param private_data private_data
 * @return ADI intance, or NULL on error
 */
ADI * adi_create(ADI * adi, SWD * swd, uint32_t select, void * private_data);

/**
 * Destroy a ADI instance
 * @param adi ADI instance
 */
void adi_destroy(ADI * adi);

#ifdef __cplusplus
}
#endif

#endif /* ADI_H */