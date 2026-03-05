
#ifndef PROCESSOR_H
#define PROCESSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../adi/adi.h"

/**
 * Processor Core States
 */
typedef enum CoreState : uint32_t {
    /**
     * The core has been reset
     */
    RESET,
    /**
     * Core is running with a lockup condition
     */
    LOCKUP,
    /**
     * The core is sleeping
     */
    SLEEPING,
    /**
     * The core is in debug state
     */
    HALT,
    /**
     * The core is running
     */
    RUNNING,
    /**
     * The core is not in debug state
     */
    NODEBUG

} CoreState;

/**
 * Processor Core Registers
 * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.100230_0004_00_en/way1435345987733.html
 */
typedef enum CoreRegister : uint32_t {
    /**
     * General purpose register
     */
    R0 = 0,
    /**
     * General purpose register
     */
    R1 = 1,
    /**
     * General purpose register
     */
    R2 = 2,
    /**
     * General purpose register
     */
    R3 = 3,
    /**
     * General purpose register
     */
    R4 = 4,
    /**
     * General purpose register
     */
    R5 = 5,
    /**
     * General purpose register
     */
    R6 = 6,
    /**
     * General purpose register
     */
    R7 = 7,
    /**
     * General purpose register
     */
    R8 = 8,
    /**
     * General purpose register
     */
    R9 = 9,
    /**
     * General purpose register
     */
    R10 = 10,
    /**
     * General purpose register
     */
    R11 = 11,
    /**
     * General purpose register
     */
    R12 = 12,
    /**
     * Stack Pointer
     */
    SP = 13,
    /**
     * The Link Register
     */
    LR = 14,
    /**
     * The Program Counter
     */
    PC = 15,
    /**
     * The Program Status Register
     */
    xPSR = 16,
    /**
     * Main Stack Pointer
     */
    MSP = 17,
    /**
     * Process Stack Pointer
     */
    PSP = 18,
    /**
     * Prevents activation of exceptions
     */
    PRIMASK = 20,
    /**
     * Controls the stack used
     */
    CONTROL = 20

} CoreRegister;

/**
 * Debug Registers
 * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.100165_0201_00_en/ric1417175947147.html
 */
typedef enum DebugRegister : uint32_t {
    /**
     * Debug Fault Status Register
     */
    DFSR = 0xE000ED30,
    /**
     * Debug Halting Control and Status Register
     */
    DHCSR = 0xE000EDF0,
    /**
     * Debug Core Register Selector Register, write only
     */
    DCRSR = 0xE000EDF4,
    /**
     * Debug Core Register Data Register
     */
    DCRDR = 0xE000EDF8,
    /**
     * Debug Exception and Monitor Control Register
     */
    DEMCR = 0xE000EDFC

} DebugRegister;

/**
 * NVIC Registers
 */
typedef enum NvicRegister : uint32_t {
    /**
     * NVIC: Interrupt Controller Type Register
     */
    ICT = 0xE000E004,
    /**
     * NVIC: CPUID Base Register
     */
    CPUID = 0xE000ED00,
    /**
     * NVIC: Application Interrupt/Reset Control Register
     */
    AIRCR = 0xE000ED0C,
    /**
     * NVIC: Debug Fault Status Register
     */
    NVIC_DFSR = 0xE000ED30

} NvicRegister;

/**
 * NVIC: Application Interrupt/Reset Control Register
 * @hidden
 */
typedef enum AircrMask : uint32_t {
    /**
     * Reset Cortex-M (except Debug)
     */
    VECTRESET = (1 << 0),
    /**
     * Clear Active Vector Bit
     */
    VECTCLRACTIVE = (1 << 1),
    /**
     * Reset System (except Debug)
     */
    SYSRESETREQ = (1 << 2),
    /**
     * Write Key
     */
    VECTKEY = 0x05FA0000

} AircrMask;

/**
 * Debug Halting Control and Status Register
 * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0337e/CEGCJAHJ.html
 * @hidden
 */
typedef enum DhcsrMask : uint32_t {
    /**
     * Enables debug
     */
    C_DEBUGEN = (1 << 0),
    /**
     * Halts the core
     */
    C_HALT = (1 << 1),
    /**
     * Steps the core in halted debug
     */
    C_STEP = (1 << 2),
    /**
     * Mask interrupts when stepping or running in halted debug
     */
    C_MASKINTS = (1 << 3),
    /**
     * Enables Halting debug to gain control
     */
    C_SNAPSTALL = (1 << 5),
    /**
     * Register Read/Write on the Debug Core Register Selector register is available
     */
    S_REGRDY = (1 << 16),
    /**
     * The core is in debug state
     */
    S_HALT = (1 << 17),
    /**
     * Indicates that the core is sleeping
     */
    S_SLEEP = (1 << 18),
    /**
     * Core is running (not halted) and a lockup condition is present
     */
    S_LOCKUP = (1 << 19),
    /**
     * An instruction has completed since last read
     */
    S_RETIRE_ST = (1 << 24),
    /**
     * The core has been reset
     */
    S_RESET_ST = (1 << 25),
    /**
     * Debug Key
     */
    DBGKEY = 0xA05F0000

} DhcsrMask;

/**
 * Debug Fault Status Register Mask
 * http://infocenter.arm.com/help/topic/com.arm.doc.ddi0413d/Cihdifbf.html
 * @hidden
 */
typedef enum DfsrMask : uint32_t {
    /**
     * Halt request flag
     */
    HALTED = (1 << 0),
    /**
     * BKPT instruction or hardware breakpoint match
     */
    BKPT = (1 << 1),
    /**
     * Data Watchpoint (DW) flag
     */
    DWTTRAP = (1 << 2),
    /**
     * Vector catch occurred
     */
    VCATCH = (1 << 3),
    /**
     * External debug request (EDBGRQ) has halted the core
     */
    EXTERNAL = (1 << 4)

} DfsrMask;

/**
 * Debug Core Register Selector Register Mask
 * http://infocenter.arm.com/help/topic/com.arm.doc.ddi0337e/CEGIAJBH.html
 * @hidden
 */
typedef enum DcrsrMask : uint32_t {
    /**
     * Register write or read, write is 1
     */
    REGWnR = (1 << 16),
    /**
     * Register select - DebugReturnAddress & PSR/Flags, Execution Number, and state information
     */
    REGSEL = 0x1F,

} DcrsrMask;

/**
 * Debug Exception and Monitor Control Register Mask
 * http://infocenter.arm.com/help/topic/com.arm.doc.ddi0337e/CEGHJDCF.html
 * @hidden
 */
typedef enum DemcrMask : uint32_t {
    /**
     * Reset Vector Catch
     */
    CORERESET = (1 << 0),
    /**
     * Debug Trap on MMU Fault
     */
    MMERR = (1 << 4),
    /**
     * Debug Trap on No Coprocessor Fault
     */
    NOCPERR = (1 << 5),
    /**
     * Debug Trap on Checking Error Fault
     */
    CHKERR = (1 << 6),
    /**
     * Debug Trap on State Error Fault
     */
    STATERR = (1 << 7),
    /**
     * Debug Trap on Bus Error Fault
     */
    BUSERR = (1 << 8),
    /**
     * Debug Trap on Interrupt Error Fault
     */
    INTERR = (1 << 9),
    /**
     * Debug Trap on Hard Fault
     */
    HARDERR = (1 << 10),
    /**
     * Monitor Enable
     */
    MON_EN = (1 << 16),
    /**
     * Monitor Pend
     */
    MON_PEND = (1 << 17),
    /**
     * Monitor Step
     */
    MON_STEP = (1 << 18),
    /**
     * Monitor Request
     */
    MON_REQ = (1 << 19),
    /**
     * Trace Enable
     */
    TRCENA = (1 << 24)

} DemcrMask;

/**
 * Flash Patch and Breakpoint Registers
 * http://infocenter.arm.com/help/topic/com.arm.doc.100165_0201_00_en/ric1417175949176.html
 * @hidden
 */
typedef enum FPBRegister : uint32_t {
    /**
     * FlashPatch Control Register
     */
    FP_CTRL = 0xE0002000,
    /**
     * FlashPatch Remap Register
     */
    FP_REMAP = 0xE0002004,
    /**
     * FlashPatch Comparator Register0
     */
    FP_COMP0 = 0xE0002008,
    /**
     * FlashPatch Comparator Register1
     */
    FP_COMP1 = 0xE000200C,
    /**
     * FlashPatch Comparator Register2
     */
    FP_COMP2 = 0xE0002010,
    /**
     * FlashPatch Comparator Register3
     */
    FP_COMP3 = 0xE0002014,
    /**
     * FlashPatch Comparator Register4
     */
    FP_COMP4 = 0xE0002018,
    /**
     * FlashPatch Comparator Register5
     */
    FP_COMP5 = 0xE000201C,
    /**
     * FlashPatch Comparator Register6
     */
    FP_COMP6 = 0xE0002020,
    /**
     * FlashPatch Comparator Register7
     */
    FP_COMP7 = 0xE0002024,

} FPBRegister;

/**
 * Flash Patch and Breakpoint Control Register Mask
 * http://infocenter.arm.com/help/topic/com.arm.doc.ddi0337e/ch11s04s01.html#BABCAFAG
 * @hidden
 */
typedef enum FPBCtrlMask : uint32_t {
    /**
     * Flash patch unit enable
     */
    ENABLE = (1 << 0),
    /**
     * Key field which enables writing to the Flash Patch Control Register
     */
    KEY = (1 << 1)

} FPBCtrlMask;

/**
 * reset target mode
 */
typedef enum ResetMode : bool {
    /**
     * soft reset
     */
    SystemReset = false,
    /**
     * hardware reset
     */
    HardwareReset = true

} ResetMode;

/**
 * Processor interface
 */
typedef struct Processor 
{
    /**
     * Private data
     */
    void * private_data;

    /**
     * ADI interface
     */
    ADI * adi;

    /**
     * Enable debug
     * @param processor Processor instance
     * @return 0 on success, error code otherwise
     */
    int ( * debuegEnable)(struct Processor * processor);

    /**
     * Disable debug
     * @param processor Processor instance
     * @return 0 on success, error code otherwise
     */
    int ( * debuegDisable)(struct Processor * processor);

    /**
     * Whether the target is debug enable
     * @param processor Processor instance
     * @param debug Pointer to store debug state
     *                 "true" is halt
     *                 "false" is other states
     * @return 0 on success, error code otherwise
     */
    int ( * isDebug)(struct Processor * processor, bool * debug);

    /**
     * Connect the target chip with in debug
     * @param processor Processor instance
     * @param idcode idcode for read
     * @return 0 on success, error code otherwise
     */
    int ( * connect)(struct Processor * processor, int * idcode);

    /**
     * Disconnect the target chip with leave debug
     * @param processor Processor instance
     * @return 0 on success, error code otherwise
     */
    int ( * disconnect)(struct Processor * processor);

    /**
     * Get the state of the processor core
     * @param processor Processor instance
     * @param state Pointer to store core state
     * @return 0 on success, error code otherwise
     */
    int ( * getState)(struct Processor * processor, CoreState * state);

    /**
     * Whether the target is halted
     * @param processor Processor instance
     * @param halted Pointer to store halted state
     *                 "true" is halt
     *                 "false" is other states
     * @return 0 on success, error code otherwise
     */
    int ( * isHalted)(struct Processor * processor, bool * halted);

    /**
     * Halt the target
     * @param processor Processor instance
     * @return 0 on success, error code otherwise
     */
    int ( * halt)(struct Processor * processor);

    /**
     * Run the target
     * @param processor Processor instance
     * @return 0 on success, error code otherwise
     */
    int ( * run)(struct Processor * processor);

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
    int ( * reset)(struct Processor * processor, ResetMode hwReset, uint32_t us, bool (* waitDelay)(uint32_t us));
    
    /**
     * Set the target to reset state with software
     * @param processor Processor instance
     * @return 0 on success, error code otherwise
     */
    int ( * setResetStateWithSoft)(struct Processor * processor);

    /**
     * Read from a core register
     * @param processor Processor instance
     * @param reg The register to read
     * @param value Pointer to store value
     * @return 0 on success, error code otherwise
     */
    int ( * readCoreRegister)(struct Processor * processor, CoreRegister reg, uint32_t * value);
    
    /**
     * read core register
     * @param processor Processor instance
     * @param reg The register to read
     * @param value The value to read
     * @param count register number to read
     * @return 0 on success, error code otherwise
     */
    int ( * readMultiCoreRegisters)(struct Processor * processor, const CoreRegister * reg, uint32_t * value, const uint32_t count);

    /**
     * Write to a core register
     * @param processor Processor instance
     * @param reg The register to write to
     * @param value The value to write
     * @return 0 on success, error code otherwise
     */
    int ( * writeCoreRegister)(struct Processor * processor, CoreRegister reg, uint32_t value);
    
    /**
     * Write to core register
     * @param processor Processor instance
     * @param reg The register to write to
     * @param value The value to write
     * @param count register number to write to
     * @return 0 on success, error code otherwise
     */
    int ( * writeMultiCoreRegisters)(struct Processor * processor, const CoreRegister * reg, const uint32_t * value, const uint32_t count);

} Processor;

/**
 * Create a new Processor instance
 * @param adi ADI instance to use
 * @param private_data Private data
 * @return Processor instance, or NULL on error
 */
Processor * processor_create(Processor * processor, ADI * adi, void * private_data);

/**
 * Destroy a Processor instance
 * @param processor Processor instance
 */
void processor_destroy(Processor *processor);

#ifdef __cplusplus
}
#endif

#endif /* PROCESSOR_H */