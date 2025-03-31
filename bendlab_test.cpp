#include "xparameters.h"
#include <cmath>
#include <stdio.h>
#include <xil_printf.h>

// #include "xgpio.h"
#include "xiic.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xintc.h"
#include "xtmrctr.h"
#include "xuartlite.h"

#include "ads_two_axis.h"
#include "i2c_axi.h"

#include "ads_two_axis_hal.h" //  for test purpose

#define AD7746_ADDR 0x48
#define BENDLAB_ONE_AXIS_ADDR (0x05)
#define BENDLAB_TWO_AXIS_ADDR (0x13)

#define ADS_RESET_PIN (4)     // Pin number attached to ads reset line.
#define ADS_INTERRUPT_PIN (3) // Pin number attached to the ads data ready line.

#define GPIO_DEVICE_ID XPAR_AXI_GPIO_1_BASEADDR // ResetPinGpio BaseAddress
#define INTC_DEVICE_ID XPAR_XINTC_0_BASEADDR    // 中断控制器BaseAddress
#define TIMER_DEVICE_ID XPAR_XTMRCTR_0_BASEADDR // 定时器BaseAddress
#define TIMER_INTR_ID 1                         // 定时中断ID
#define BL_R0_INTR_ID 0                         // Bendlab中断ID

XIic iic_bl_r0, iic_bl_r1;
XUartLite axi_uartlite_0;
XIntc axi_intc;
XTmrCtr axi_timer;
XGpio reset_gpio;
isr_t isr_;

#define LED_DEV_ID XPAR_AXI_GPIO_0_BASEADDR // LED ID
XGpio led_gpio, intr_gpio;                  // LED实例

// function prototypes
void ads_data_callback(float *sample);
void deadzone_filter(float *sample);
void signal_filter(float *sample);
void parse_serial_port(void);

int abs(int x) {
    return (x < 0) ? -x : x;
}

float ang[2];
volatile bool newData = false;

void ads_data_callback(float *sample) {
  // Low pass IIR filter
  signal_filter(sample);

  // Deadzone filter
  deadzone_filter(sample);

  ang[0] = sample[0];
  ang[1] = sample[1];

  newData = true;
}

uint8_t read_buffer[ADS_TRANSFER_SIZE];

/*
void ads_hal_interrupt(void *isr) {
isr_t *isr_ = (isr_t *)isr;
// if (ads_hal_read_buffer(isr_->InstancePtr, isr_->I2cAddr, read_buffer,
// ADS_TRANSFER_SIZE) == ADS_OK) {
int recv_num = I2C_Read_axi(isr_->InstancePtr->BaseAddress, isr_->I2cAddr, -1,
                          ADS_TRANSFER_SIZE, read_buffer);
if (recv_num == ADS_TRANSFER_SIZE)
xil_printf("read OK\n\r");
else
xil_printf("read ERROR\n\r");
// ads_read_callback(read_buffer);
}
*/

void ads_hal_interrupt(void *CallbackRef) {
  //    xil_printf("\n\r==============\n\rInterrupt
  //    Triggered!\n\r=============\n\r");
  XIntc_Acknowledge(&axi_intc, 1); // Acknowledge the interrupt
  isr_t *isr_ = (isr_t *)CallbackRef;
  // if (ads_hal_read_buffer(isr_->InstancePtr, isr_->I2cAddr, read_buffer,
  // ADS_TRANSFER_SIZE) == ADS_OK) {
  int recv_num = I2C_Read_axi(isr_->InstancePtr->BaseAddress, isr_->I2cAddr, -1,
                              ADS_TRANSFER_SIZE, read_buffer);
  //    int recv_num = I2C_Read_axi(iic_bl_r0.BaseAddress,
  //    BENDLAB_TWO_AXIS_ADDR, -1,
  //                                ADS_TRANSFER_SIZE, read_buffer);

  if (read_buffer[0] == ADS_SAMPLE) {
    float sample[2];

    int16_t temp = ads_int16_decode(&read_buffer[1]);
    sample[0] = (float)temp / 32.0f;
    //xil_printf("0: %d", temp);
    temp = ads_int16_decode(&read_buffer[3]);
    sample[1] = (float)temp / 32.0f;
    //xil_printf("1: %d", temp);

    int whole[2];
    int thousandths[2];
    whole[0] = (int)sample[0];
    thousandths[0] = abs(int((sample[0] - whole[0]) * 1000));
    whole[1] = (int)sample[1];
    thousandths[1] = abs(int((sample[1] - whole[1]) * 1000));
    xil_printf("%d.%03d, %d.%03d\n\r", whole[0], thousandths[0], whole[1], thousandths[1]);
  }

  //     if (recv_num == ADS_TRANSFER_SIZE)
  //         xil_printf("read OK\n\r");
  //     else
  //         xil_printf("read ERROR\n\r");
}

int main() {
  delay(2000);

  // Serial.println("Initializing Two Axis sensor");
  xil_printf("Initializing Two Axis sensor\n\r");

  ads_t bl_r0;
  ads_init_t bl_r0_init;

  bl_r0_init.sps = ADS_100_HZ;
  bl_r0_init.ads_sample_callback = &ads_data_callback;
  bl_r0_init.reset_pin = ADS_RESET_PIN; // Pin connected to ADS reset line
  bl_r0_init.datardy_pin =
      ADS_INTERRUPT_PIN; // Pin connected to ADS data ready interrupt
  // user defined
  bl_r0.i2c_ctrl_baseaddr = XPAR_IIC_BENDLAB_BASEADDR;
  bl_r0.i2c_ctrl_ptr = &iic_bl_r0;
  bl_r0.i2c_addr = BENDLAB_TWO_AXIS_ADDR;
  bl_r0.intr_ctrl_baseaddr = XPAR_AXI_INTC_0_BASEADDR;
  bl_r0.intr_ctrl_ptr = &axi_intc;
  bl_r0.intr_vec_id = 1;
  // bl_r0.reset_pins = ;
  bl_r0.reset_id = 0;
  bl_r0.intr_enabled = false;

  // //初始化LED
  // XGpio_Initialize(&led_gpio, LED_DEV_ID);
  // //为指定的GPIO信道设置所有独立信号的输入/输出方向
  // XGpio_SetDataDirection(&led_gpio, 1, 0);
  // //设置LED初始值
  // XGpio_DiscreteWrite(&led_gpio, 1, 0xEF);

  //中断控制器初始化
  int ads_size = 1;
  ads_t *ads = &bl_r0;
  ads_init_t *ads_init = &bl_r0_init;
  XIntc_Initialize(ads->intr_ctrl_ptr, ads->intr_ctrl_baseaddr);
  for (int i = 0; i < ads_size; ++i) {
    isr_.InstancePtr = ads[i].i2c_ctrl_ptr;
    isr_.I2cAddr = ads[i].i2c_addr;
    //关联中断源和中断处理函数
    // XIntc_Connect(ads->intr_ctrl_ptr, ads[i].intr_vec_id,
    //               (XInterruptHandler)ads_hal_interrupt, NULL);
    XIntc_Connect(ads->intr_ctrl_ptr, ads[i].intr_vec_id,
                  (XInterruptHandler)ads_hal_interrupt, (void *)&isr_);
    //使能中断控制器
    XIntc_Enable(ads->intr_ctrl_ptr, ads[i].intr_vec_id);
  }
  //开启中断控制器
  XIntc_Start(ads->intr_ctrl_ptr, XIN_REAL_MODE);
  //设置并打开中断异常处理
  Xil_ExceptionInit();
  Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                               (Xil_ExceptionHandler)XIntc_InterruptHandler,
                               ads->intr_ctrl_ptr);
  Xil_ExceptionEnable();
  xil_printf("AXI interrupt controller initilization succeeded\n\r");

  // Initialize ADS hardware abstraction layer, and set the sample rate
  // int ret_val = ads_two_axis_init(&bl_r0_init, &bl_r0);
  /////////////////////////////////////////////////////////////////////////////////////////
  ads_hal_init(ads);

  // Check that the device id matched ADS_TWO_AXIS
  // Check that the device type is a one axis
  ADS_DEV_TYPE_T ads_dev_type;
  if (ads_get_dev_type(ads, &ads_dev_type) != ADS_OK)
    return ADS_ERR_DEV_ID;

  switch (ads_dev_type) {
  case ADS_DEV_TWO_AXIS_V1:
  case ADS_DEV_TWO_AXIS_V2:
    break;
  default:
    return ADS_ERR_DEV_ID;
  }

  ads_hal_delay(2);

  if (ads_two_axis_set_sample_rate(ads, ads_init->sps)) {
    xil_printf("ads_two_axis_init ERROR\n\r");
    return ADS_ERR;
  }

  int ret_val = ADS_OK;

  ads_hal_delay(2);

  xil_printf("before ads_two_axis_enable_interrupt\n\r");
  // XGpio_DiscreteWrite(&led_gpio, 1, 0xA5);

  if (ret_val == ADS_OK) {
    // Serial.println("Two Axis ADS initialization succeeded");
    xil_printf("Two Axis ADS initialization succeeded\n\r");
  } else {
    // Serial.print("Two Axis ADS initialization failed with reason: ");
    // Serial.println(ret_val);
    xil_printf("Two Axis ADS initialization failed with reason: %d\n\r",
               ret_val);
  }

  delay(100);

  // Start reading data!
  xil_printf("before ads_two_axis_run\n\r");
  // ads_two_axis_run(&bl_r0, true);
  uint8_t buffer[ADS_TRANSFER_SIZE];

  buffer[0] = ADS_RUN;
  buffer[1] = true;

  ads_hal_write_buffer(ads, buffer, ADS_TRANSFER_SIZE);
  xil_printf("ads_two_axis_run\n\r");

  while (1) {
  }

  return 0;
}

void signal_filter(float *sample) {
  static float filter_samples[2][6];

  for (uint8_t i = 0; i < 2; i++) {
    filter_samples[i][5] = filter_samples[i][4];
    filter_samples[i][4] = filter_samples[i][3];
    filter_samples[i][3] = (float)sample[i];
    filter_samples[i][2] = filter_samples[i][1];
    filter_samples[i][1] = filter_samples[i][0];

    // 20 Hz cutoff frequency @ 100 Hz Sample Rate
    filter_samples[i][0] = filter_samples[i][1] * (0.36952737735124147f) -
                           0.19581571265583314f * filter_samples[i][2] +
                           0.20657208382614792f * (filter_samples[i][3] +
                                                   2 * filter_samples[i][4] +
                                                   filter_samples[i][5]);

    sample[i] = filter_samples[i][0];
  }
}

void deadzone_filter(float *sample) {
  static float prev_sample[2];
  float dead_zone = 0.5f;

  for (uint8_t i = 0; i < 2; i++) {
    if (fabs(sample[i] - prev_sample[i]) > dead_zone)
      prev_sample[i] = sample[i];
    else
      sample[i] = prev_sample[i];
  }
}