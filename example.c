#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "port/port.h"

/*-----------------------------------------------------------*/

/*
    Note: The test "SWD" comes from "DAP-Link -> SW_DP.c".
*/

/*-----------------------------------------------------------*/

static void fake_Initialize(struct SWD * swd) 
{ 

}

static void fake_DeInitialize(struct SWD * swd) 
{ 

}

/**
 * hardware reset operation
 * @param swd SWD interface
 * @param hwReset reset the target use hardware
 *                  "true" : the hardware pin is catch
 *                  "false" : the hardware pin is release
 */
static void fake_hwReset(struct SWD * swd, bool hwReset)
{

}

/**
 * Generate SWJ/SWD Sequence
 * @param swd SWD interface
 * @param info sequence information
 *             if info = 1: read data (swdi)
 *             if info = 0: write data (swdo)
 * @param bits read/write bit count
 * @param swdio pointer to SWDIO generated data or captured data refer to info
 */
static void fake_SWJ_Sequence(struct SWD * swd, uint8_t info, uint32_t bits, uint8_t * swdio)
{
    if(0 == info)
    {
        extern void SWJ_Sequence (unsigned int count, const uint8_t *data);
        SWJ_Sequence((unsigned int)bits, swdio);
    }
    else
    {
        extern void SWD_Sequence (unsigned int info, const uint8_t *swdo, uint8_t *swdi);
        SWD_Sequence((unsigned int)(bits | 0x80), (const uint8_t *)swdio, swdio);
    }
}

static uint8_t fake_SWD_Transfer(struct SWD * swd, uint32_t request, uint32_t * data)
{
    extern uint8_t SWD_Transfer (unsigned int request, unsigned int *data);
    // remove [start] bit
    return SWD_Transfer((unsigned int)(request >> 1), (unsigned int *)data);
}

static SWD fake_swd = 
{
    .private_data = NULL,
    .Init = fake_Initialize,
    .DeInit = fake_DeInitialize,
    .hwReset = fake_hwReset,
    .Sequence = fake_SWJ_Sequence,
    .Transfer = fake_SWD_Transfer
};

/*-----------------------------------------------------------*/

Port portopd = { 0 };

static uint32_t id[0x100];
static uint32_t addressAAA;

int adi_example(void)
{
    int idcode = 0;
    addressAAA = 0x20000000;
    Processor * p = processor_create(&portopd.processor, adi_create(&portopd.adi, (SWD *)&fake_swd, 0UL, NULL), NULL);
    if(&portopd.processor != p)
        return -1;
    Mem * pmem = mem_creat(&portopd.mem, &portopd.adi, NULL);
    if(&portopd.mem != pmem)
        return -1;
    p->adi->swd->Init(p->adi->swd);
    p->adi->connect(p->adi, &idcode);
    // p->run(p);
    // p->setResetStateWithSoft(p);
    // p->readCoreRegister(p, xPSR, (uint32_t *)&idcode);
    // p->adi->writeMem32(p->adi, 0xe000ed0c, 0x05fa0004);
    p->halt(p);
    // p->adi->readBlock(p->adi, 0x20000000, 0x100, id);
    // p->adi->fillMem32(p->adi, 0x20000000, 0x00000000, 0x100);
    // p->adi->readBytes(p->adi, 0x20000000, 0x100, (uint8_t *)id);
    // p->adi->writeMem32(p->adi, 0x20000000, 1);
    // p->adi->pushVerify32(p->adi, &addressAAA, id, 0x100, LANE_ALL);
    // p->adi->readBlock(p->adi, 0x20000000, 0x100, id);
    // p->adi->disconnect(p->adi);
    p->adi->swd->DeInit(p->adi->swd);
    adi_destroy(p->adi);
    processor_destroy(p);
    return 0;
}
