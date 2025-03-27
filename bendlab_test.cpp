#include <stdio.h>
#include <cmath>
#include <xil_printf.h>
#include "xparameters.h"

#include "xiic.h"
#include "xuartlite.h"
#include "xintc.h"
#include "xtmrctr.h"
#include "xil_exception.h"
//#include "xgpio.h"
#include "xil_printf.h"

#include "i2c_axi.h"
#include "ads_two_axis.h"

#define AD7746_ADDR 0x48
#define BENDLAB_ONE_AXIS_ADDR   (0x05)
#define BENDLAB_TWO_AXIS_ADDR   (0x13)

#define ADS_RESET_PIN       (4)         // Pin number attached to ads reset line.
#define ADS_INTERRUPT_PIN   (3)         // Pin number attached to the ads data ready line. 

//#define GPIO_DEVICE_ID        XPAR_AXI_GPIO_0_BASEADDR    // ResetPinGpio BaseAddress
#define INTC_DEVICE_ID      XPAR_XINTC_0_BASEADDR       // 中断控制器BaseAddress
#define TIMER_DEVICE_ID     XPAR_XTMRCTR_0_BASEADDR     // 定时器BaseAddress
#define TIMER_INTR_ID       1                           // 定时中断ID
#define BL_R0_INTR_ID       0                           // Bendlab中断ID

XIic iic_bl_r0, iic_bl_r1;
XUartLite axi_uartlite_0;
XIntc    axi_intc;
XTmrCtr  axi_timer;
//XGpio    reset_gpio;

// function prototypes
void ads_data_callback(float * sample);
void deadzone_filter(float * sample);
void signal_filter(float * sample);
void parse_serial_port(void);

float ang[2];
volatile bool newData = false;


void ads_data_callback(float * sample)
{
  // Low pass IIR filter
  signal_filter(sample);

  // Deadzone filter
  deadzone_filter(sample);
  
  ang[0] = sample[0];
  ang[1] = sample[1];
  
  newData = true;
}

int main ()
{
    // Serial.begin(115200);

    delay(2000);

    // Serial.println("Initializing Two Axis sensor");
    xil_printf("Initializing Two Axis sensor\n\r");

    ads_t bl_r0;

    bl_r0.sps = ADS_100_HZ;
    bl_r0.ads_sample_callback = &ads_data_callback;
    bl_r0.reset_pin = ADS_RESET_PIN;                 // Pin connected to ADS reset line
    bl_r0.datardy_pin = ADS_INTERRUPT_PIN;           // Pin connected to ADS data ready interrupt
    // user defined
    bl_r0.i2c_ctrl_ptr = &iic_bl_r0;
    bl_r0.i2c_addr = BENDLAB_TWO_AXIS_ADDR;
    bl_r0.intr_ctrl_ptr = &axi_intc;
    bl_r0.intr_vec_id = 0;
    //bl_r0.reset_pins = ;
    bl_r0.reset_id = 0;
    bl_r0.intr_enabled = false;

    // Initialize ADS hardware abstraction layer, and set the sample rate
    int ret_val = ads_two_axis_init(&bl_r0);

    if(ret_val == ADS_OK)
    {
        // Serial.println("Two Axis ADS initialization succeeded");
        xil_printf("Two Axis ADS initialization succeeded\n\r");
    }
    else
    {
        // Serial.print("Two Axis ADS initialization failed with reason: ");
        // Serial.println(ret_val);
        xil_printf("Two Axis ADS initialization failed with reason: %d\n\r", ret_val);
    }

    delay(100);

/*
    // Start reading data!
    ads_two_axis_run(true);

    while (1) {
        if(newData)
        {
            newData = false;

            // Serial.print(ang[0]); 
            // Serial.print(","); 
            // Serial.println(ang[1]);
            xil_printf("%d, %d\n\r");
        }

        if(Serial.available())
        {
            parse_serial_port();
        }
    }
*/
}

void signal_filter(float * sample)
{
    static float filter_samples[2][6];

    for(uint8_t i=0; i<2; i++)
    {
      filter_samples[i][5] = filter_samples[i][4];
      filter_samples[i][4] = filter_samples[i][3];
      filter_samples[i][3] = (float)sample[i];
      filter_samples[i][2] = filter_samples[i][1];
      filter_samples[i][1] = filter_samples[i][0];
  
      // 20 Hz cutoff frequency @ 100 Hz Sample Rate
      filter_samples[i][0] = filter_samples[i][1]*(0.36952737735124147f) - 0.19581571265583314f*filter_samples[i][2] + \
        0.20657208382614792f*(filter_samples[i][3] + 2*filter_samples[i][4] + filter_samples[i][5]);   

      sample[i] = filter_samples[i][0];
    }
}

void deadzone_filter(float * sample)
{
  static float prev_sample[2];
  float dead_zone = 0.5f;

  for(uint8_t i=0; i<2; i++)
  {
    if(fabs(sample[i]-prev_sample[i]) > dead_zone)
      prev_sample[i] = sample[i];
    else
      sample[i] = prev_sample[i];
  }
}