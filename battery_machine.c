/*
 * battery.c
 *
 * Reads Battery Voltage using IADC peripheral, keeps track of sending report events
 * to Tag Beacon Machine.
 */


#include "battery_machine.h"
#include "tag_sw_timer.h"

//******************************************************************************
// Defines
//******************************************************************************

#define CLK_SRC_ADC_FREQ        20000000  // CLK_SRC_ADC
#define CLK_ADC_FREQ            100000  // CLK_ADC - 10 MHz max in normal mode


//******************************************************************************
// Globals
//******************************************************************************
static tag_sw_timer_t btm_timer;
static volatile uint16_t battery_voltage_in_mv; //Voltage is in mV

//******************************************************************************
// Static Functions
//******************************************************************************
static void read_battery_voltage(void)
{

  uint32_t iadc_reg_data;

  IADC_Init_t init = IADC_INIT_DEFAULT;
  IADC_AllConfigs_t init_all_config = IADC_ALLCONFIGS_DEFAULT;
  IADC_InitSingle_t init_single = IADC_INITSINGLE_DEFAULT;
  IADC_SingleInput_t init_single_input = IADC_SINGLEINPUT_DEFAULT;

  /*
   * Clock Select/Enable, and reseting IADC to default state
   */
  CMU_ClockEnable (cmuClock_IADC0, true);
  IADC_reset(IADC0);
  CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_FSRCO);

  /*
   * IADC Init
   */
  init.warmup = iadcWarmupNormal; // Set this Config to Shutdown IADC After Each Reading
  init.timerCycles = 200;         // Number of ADC_CLK cycles per TIMER event
  init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, CLK_SRC_ADC_FREQ, 0);

  /*
   * Internal Reference Selection
   */
  init_all_config.configs[0].reference = iadcCfgReferenceInt1V2;                            // Selecting Internal Reference (1V2)
  init_all_config.configs[0].adcClkPrescale = IADC_calcAdcClkPrescale( IADC0,
                                                                       CLK_ADC_FREQ,
                                                                       0,
                                                                       iadcCfgModeNormal,
                                                                       init.srcClkPrescale);
  /*
   * Single Ended IADC Reading
   * Input Pin (AVDD): This pin outputs (Vcc / 4) voltage
   * So, we need to multiply by 4 when we calculate voltage from IADC Reg
   */
  init_single_input.posInput = iadcPosInputAvdd;
  init_single_input.negInput = iadcNegInputGnd | 1;      // When measuring a supply, PINNEG must be odd (1, 3, 5,...)
  init_single.triggerAction = iadcTriggerActionOnce;


  IADC_init(IADC0, &init, &init_all_config);
  IADC_initSingle(IADC0, &init_single, &init_single_input);

  /*
   * Send Command to IADC Peripheral to start Reading Battery Voltage
   */
  IADC_command (IADC0, iadcCmdStartSingle);

  /*
   * Polling until IADC finishes the capture
   */
  while ((IADC0->STATUS & (_IADC_STATUS_CONVERTING_MASK | _IADC_STATUS_SINGLEFIFODV_MASK)) != IADC_STATUS_SINGLEFIFODV);


  /*
   * Retrieve IADC Register Data
   */
  iadc_reg_data = IADC_pullSingleFifoResult(IADC0).data;

  /*
   * Calculate Final Voltage using formula :
   * final_voltage = ( Scale_Voltage_To_Full_VCC_Range ) * ( Internal_VREF ) / ( 12_BIT_ADC_MAX );
   *
   * final_voltage = ( register_data * 4 ) * ( 1.21 ) / (2^12);
   */
  double final_voltage = ((iadc_reg_data * 4 * 1.21) / (0xFFF)) ;

  /*
   * We must reset and disable the clocks to have low power consumption
   */
  IADC_reset(IADC0);
  CMU_ClockEnable(cmuClock_FSRCO, false);
  CMU_ClockEnable(cmuClock_IADC0, false);

  /*
   * TODO: Use this value to determine Battery BIT (H/L)
   */
  battery_voltage_in_mv = (uint16_t)(final_voltage * 1000);

#if defined(TAG_LOG_PRESENT)
    DEBUG_LOG(DBG_CAT_BATTERY,"Battery Voltage: %d (mV)", (int)battery_voltage_in_mv);
#endif

}

//******************************************************************************
// Non-Static Functions
//******************************************************************************

/**
 * @brief Battery Machine
 * @details Monitors battery voltage and report battery low status.
 */
void battery_machine_run(void)
{
  // Tick Battery Machine Timer
  tag_sw_timer_tick(&btm_timer);

  // Measure Battery Voltage every BTM_TIMER_PERIOD_SEC seconds and
  // Send its event to Tag Beacon Machine
  if (tag_sw_timer_is_expired(&btm_timer)) {
      tag_sw_timer_reload(&btm_timer, BTM_TIMER_PERIOD_SEC);
      read_battery_voltage();
      // TODO: Add Event
  }
}

uint32_t btm_init(void)
{
    tag_sw_timer_reload(&btm_timer, BTM_TIMER_PERIOD_SEC);
    battery_voltage_in_mv = 0;
    return 0;
}


