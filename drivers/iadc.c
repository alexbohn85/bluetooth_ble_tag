/*
 * iadc.c
 *
 *  Created on: Mar. 24, 2022
 *      Author: jruda
 */

#include "em_ramfunc.h"
#include "em_core.h"
#include "em_iadc.h"

#include "battery_machine.h"
#include "iadc.h"


//******************************************************************************
// Defines
//******************************************************************************

// Set CLK_ADC to 10 MHz
//#define CLK_SRC_ADC_FREQ             (20000000)  // CLK_SRC_ADC
#define CLK_SRC_ADC_FREQ             (10000000)  // CLK_SRC_ADC
#define CLK_ADC_FREQ                 (10000000)  // CLK_ADC - 10 MHz max in normal mode

void IADC_IRQHandler (void)
{
    CORE_DECLARE_IRQ_STATE;
    uint32_t irq_flag;

    CORE_ENTER_ATOMIC();
    irq_flag = IADC_getInt(IADC0);

    // Clear IADC flags
    IADC_clearInt(IADC0, (irq_flag & IADC_IF_SCANTABLEDONE));
    NVIC_ClearPendingIRQ(IADC_IRQn);

    if (irq_flag & IADC_IF_SCANTABLEDONE) {
        IADC_Result_t adc_result;
        //   Read a result from the FIFO
        adc_result = IADC_pullScanFifoResult(IADC0);

        /*
         * Calculate the voltage converted as follows:
         *
         * For single-ended conversions, the result can range from 0 to
         * +Vref, i.e., for Vref = AVDD = 3.30 V, 0xFFF = 2^12 bit  represents the
         * full scale value of 1.21 V.
         */

        double voltage;
        voltage = 4 * adc_result.data * 1.21 / 0xFFF;

        // Update Battery Machine battery voltage
        btm_set_battery_voltage(&voltage);

        //  volatile IADC_Result_t TEMP = IADC_pullScanFifoResult(IADC0);
        //
        //  /*
        //   * Calculate the voltage converted as follows:
        //   *
        //   * For single-ended conversions, the result can range from 0 to
        //   * +Vref, i.e., for Vref = AVDD = 3.30 V, 0xFFF = 2^12 bit  represents the
        //   * full scale value of 1.20 V.
        //   */
        //
        //  volatile double batteryVoltage1;
        //  batteryVoltage1 = 4 * TEMP.data * 1.21 / 0xFFF;
        //
        //  int int_part = (int) batteryVoltage1;
        //  int dec_part = ((int) (batteryVoltage1 * BATTERY_READING_PRECISION)
        //      % BATTERY_READING_PRECISION);
    }

    CORE_EXIT_ATOMIC();
}

/**
 * @brief Initialize IADC driver
 */
void adc_init(void)
{
  // Declare initialization structures
  IADC_Init_t init = IADC_INIT_DEFAULT;
  IADC_AllConfigs_t init_all_config = IADC_ALLCONFIGS_DEFAULT;
  IADC_InitScan_t init_scan = IADC_INITSCAN_DEFAULT;

  // Scan table structure
  IADC_ScanTable_t scan_table = IADC_SCANTABLE_DEFAULT;

  CMU_ClockEnable(cmuClock_IADC0, true);

  // Use the FSRC0 as the IADC clock so it can run in EM2
  CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_FSRCO);

  // Set the prescaler needed for the intended IADC clock frequency
  init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, CLK_SRC_ADC_FREQ, 0);

  // Shutdown between conversions to reduce current
  init.warmup = iadcWarmupNormal;

  /*
   * Configuration 0 is used by both scan and single conversions by
   * default.
   * VREF = Internal Voltage Reference (1.21 Volt)
   */

  init_all_config.configs[0].reference = iadcCfgReferenceInt1V2;
  init_all_config.configs[0].vRef = 1210;
  init_all_config.configs[0].osrHighSpeed = iadcCfgOsrHighSpeed2x;

  /*
   * CLK_SRC_ADC is prescaled to derive the intended CLK_ADC frequency.
   *
   * Based on the default 2x oversampling rate (OSRHS)...
   *
   * conversion time = ((4 * OSRHS) + 2) / fCLK_ADC
   *
   * ...which, results in a maximum sampling rate of 833 ksps with the
   * 2-clock input multiplexer switching time is included.
   */
  init_all_config.configs[0].adcClkPrescale = IADC_calcAdcClkPrescale( IADC0,
                                                                       CLK_ADC_FREQ,
                                                                       0,
                                                                       iadcCfgModeNormal,
                                                                       init.srcClkPrescale);

  /*
   * Set the SCANFIFODVL flag when there is one entry in the scan
   * FIFO.
   *
   * Tag FIFO entries with scan table entry IDs.
   *
   * Allow a scan conversion sequence to start as soon as there is a
   * trigger event.
   */

  init_scan.dataValidLevel = iadcFifoCfgDvl1; // 1 Entry
  init_scan.showId = true;
  init_scan.start = true;

  scan_table.entries[0].posInput = iadcPosInputAvdd;      // Battery Input Pin
  scan_table.entries[0].negInput = iadcNegInputGnd | 1;   // When measuring a supply, PINNEG must be odd (1, 3, 5,...)
  scan_table.entries[0].includeInScan = true;

  // Initialize IADC
  IADC_init(IADC0, &init, &init_all_config);

  // Initialize scan
  IADC_initScan(IADC0, &init_scan, &scan_table);

  // Clear any previous interrupt flags
  IADC_clearInt(IADC0, _IADC_IF_MASK);

  // Enable scan interrupts
  IADC_enableInt(IADC0, IADC_IEN_SCANTABLEDONE);

  // Enable IADC interrupts
  NVIC_ClearPendingIRQ(IADC_IRQn);
  NVIC_EnableIRQ(IADC_IRQn);
}


