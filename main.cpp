#include <stdint.h>
#include "intrinsics.h"
#include "sam3x8e_setup.h"
#include "sam3x8e_wdt.h"
#include "sam3x8e_twi.h"
#include "sam3x8e_ssc.h"
#include "sam3x8e_adc.h"
#include "sam3x8e_din.h"
#include "sam3x8e_dac.h"
#include "sam3x8e_dout.h"

using namespace std;

int32_t            dac_volt[5];
volatile uint32_t  adc_volt_a1[5];
volatile uint32_t  adc_volt_a2[5];

void init_clock_for_wm8731(void);
void measure_system(void);

static int      lrtoggle        = 0;
static int32_t  input_l         = 0;
static int32_t  input_r         = 0;

class SSC1
{
public:
    static void Handler();
    static volatile unsigned int ticks;
};

void SSC1::Handler()
{
    if(!lrtoggle)
    {
        lrtoggle = 1;
        
        input_l = (int32_t)(SSC_RHR);

        SSC_THR = input_l;
    }
    else
    {
        lrtoggle = 0;
        
        input_r = (int32_t)(SSC_RHR);

        SSC_THR = input_r;
    }
}

int main()
{
    volatile int i = 0;
    __disable_interrupt();

    SAM3X8E_WDT.disable_watchdog_timer();

    SAM3X8E_SETUP.init_clock();
    
    init_clock_for_wm8731();
    
    SAM3X8E_TWI.init_twi1();
    SAM3X8E_TWI.setup_twi1_master_transfer();
    SAM3X8E_SSC.init_ssc();

    i = SAM3X8E_TWI.setup_WM8731();

    SAM3X8E_SSC.setup_ssc_master_transfer();
    SAM3X8E_SSC.ssc_interrupt_setup();
    //SAM3X8E_DIN.enable_digital_input();
    SAM3X8E_ADC.enable_adc_input();
    SAM3X8E_ADC.reset_adc();
    //SAM3X8E_ADC.enable_ad1();
    //SAM3X8E_ADC.enable_ad2();
    //SAM3X8E_ADC.enable_ad3();
    SAM3X8E_ADC.enable_measure_adc();
    SAM3X8E_ADC.configure_adc_input();
    
    SAM3X8E_DAC.enable_dac_output();
    SAM3X8E_DAC.configure_dac_output();
    
    SAM3X8E_DOUT.enable_digital_output();
    
    SAM3X8E_DOUT.reset_relay(RELAY_ALL);
    SAM3X8E_DAC.write_dac(0x07FFU);
    
    
    SAM3X8E_DOUT.set_relay(RELAY_IN_AUDIO);
    //measure_system();

    __enable_interrupt();
    
    while(1)
    {
      
    }
}

void init_clock_for_wm8731()
{
    PIOA_WPMR = PIO_WPKEY | WPEN_0;
    PIOA_PDR = ~PIOA_PSR | (1U << 1);  // enable peripheral control
    PIOA_PER = PIOA_PSR & ~(1U << 1);
    PIOA_ABSR |= (1U << 1);  // peripherial B selected
    PIOA_WPMR = PIO_WPKEY | WPEN_1;
    
    PMC_WPMR = PMC_WPKEY | WPEN_0;
    PMC_SCER |=	0x0100U;  // page 558
    PMC_PCK0 = 0x01U;  // main crystak osc = 12Mhz
    PMC_WPMR = PMC_WPKEY | WPEN_1;
}

void measure_system()
{
    volatile int count ;
    volatile int index;
    index = 0;
    count = 0;
    
    dac_volt[0] = -1000000;
    dac_volt[1] = -500000;
    dac_volt[2] = 500000;
    dac_volt[3] = 700000;
    dac_volt[4] = 1000000;
    
    SAM3X8E_DOUT.set_relay(RELAY_MEAS_A1);
    SAM3X8E_DOUT.set_relay(RELAY_IN_CV);
    
    while(1)
    {
      while(index < 5)
      {
        while(count < 999999)
        {
          SAM3X8E_DAC.write_dac_voltage(dac_volt[index]);
          adc_volt_a1[index] = SAM3X8E_ADC.read_measure_adc();
          count++;
        }
        count = 0;
        index++;
      }
      index = 0;
      count = 0;
      SAM3X8E_DOUT.reset_relay(RELAY_ALL);
      while(count < 999999)
      {
        count++;
      }
      count = 0;
      SAM3X8E_DOUT.set_relay(RELAY_MEAS_A2);
      while(count < 999999)
      {
        count++;
      }
      count = 0;
      SAM3X8E_DOUT.set_relay(RELAY_IN_CV);
      while(count < 999999)
      {
        count++;
      }
      count = 0;
      while(index < 5)
      {
        while(count < 999999)
        {
          SAM3X8E_DAC.write_dac_voltage(dac_volt[index]);
          adc_volt_a2[index] = SAM3X8E_ADC.read_measure_adc();
          count++;
        }
        count = 0;
        index++;
      }
      while(count < 999999)
      {
        count++;
      }
      count = 0;
      SAM3X8E_DOUT.reset_relay(RELAY_ALL);
      SAM3X8E_DOUT.set_relay(RELAY_IN_AUDIO);
      break;
    }
}