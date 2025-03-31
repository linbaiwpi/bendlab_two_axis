/**
 * Created by nrudh on 1/15/2025.
 *
 * This software is provided "as is", without any warranty of any kind, express
 * or implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose, and noninfringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages, or other
 * liability, whether in an action of contract, tort, or otherwise, arising
 * from, out of, or in connection with the software or the use or other dealings
 * in the software.
 */

#include "ads_two_axis_hal.h"
#include "i2c_axi.h"
#include "sleep.h"
#include "xiic.h"
#include "xil_printf.h"
#include <cstdint>

/* Hardware Specific Includes */
// #include "Arduino.h"
// #include <Wire.h>

static void (*ads_read_callback)(uint8_t *);

static uint8_t read_buffer[ADS_TRANSFER_SIZE];

#define ADS_DEFAULT_ADDR (0x13) // Default I2C address of the ADS

static uint32_t ADS_RESET_PIN = 0;
static uint32_t ADS_INTERRUPT_PIN = 0;

static uint8_t _address = ADS_DEFAULT_ADDR;

volatile bool _ads_int_enabled = false;

/* Device I2C address array. Use ads_hal_update_addr() to
 * populate this array. */
static uint8_t ads_addrs[ADS_COUNT] = {
    ADS_DEFAULT_ADDR,
};

/************************************************************************/
/*                        HAL Stub Functions                            */
/************************************************************************/
static inline void ads_hal_gpio_pin_write(uint8_t pin, uint8_t val);
static void ads_hal_pin_int_init(ads_t *ads);

/**
 * @brief ADS data ready interrupt. Reads out packet from ADS and fires callback
 * in ads.c
 *
 * @param buffer[in]    Write buffer
 * @param len           Length of buffer.
 * @return  ADS_OK if successful ADS_ERR_IO if failed
 */
/*
void ads_hal_interrupt(void *isr) {
  isr_t *isr_ = (isr_t *)isr;
  // if (ads_hal_read_buffer(isr_->InstancePtr, isr_->I2cAddr, read_buffer,
  // ADS_TRANSFER_SIZE) == ADS_OK) {
  int recv_num = I2C_Read_axi(isr_->InstancePtr->BaseAddress, isr_->I2cAddr, -1,
                              ADS_TRANSFER_SIZE, read_buffer);
  if (recv_num == ADS_TRANSFER_SIZE)
    xil_printf("read OK");
  else
    xil_printf("read ERROR");
  // ads_read_callback(read_buffer);
}
*/

static void ads_hal_pin_int_init(ads_t *ads) {
  // TODO
  /*
pinMode(ADS_INTERRUPT_PIN, INPUT_PULLUP);
ads_hal_pin_int_enable(true);
*/
  ads_hal_pin_int_enable(ads, true);
}

static inline void ads_hal_gpio_pin_write(uint8_t pin, uint8_t val) {
    (void)pin;
    (void)val;
  // TODO
  /*
digitalWrite(pin, val);
*/
}

void ads_hal_delay(uint16_t delay_ms) {
  usleep(delay_ms * 1000);

  /* commented since this won't be used on FPGA
  delay(delay_ms);
  */
}

void delay(uint16_t delay_ms) { ads_hal_delay(delay_ms); }

void ads_hal_pin_int_enable(ads_t *ads, bool enable) {
  // _ads_int_enabled = enable;

  if (enable) {
    // attachInterrupt(digitalPinToInterrupt(ADS_INTERRUPT_PIN),
    // ads_hal_interrupt, FALLING);
    XIntc_Enable(ads->intr_ctrl_ptr, ads->intr_vec_id);
  } else {
    // detachInterrupt(digitalPinToInterrupt(ADS_INTERRUPT_PIN));
    XIntc_Disable(ads->intr_ctrl_ptr, ads->intr_vec_id);
  }
}

/**
 * @brief Configure I2C bus, 7 bit address, 400kHz frequency enable clock
 * stretching if available.
 */
void ads_hal_i2c_init(ads_t *ads) {
  int Status;
  XIic_Config *ConfigPtr;

  ConfigPtr = XIic_LookupConfig(ads->i2c_ctrl_baseaddr);
  if (ConfigPtr == NULL) {
    xil_printf("[ERROR] XIic_LookupConfig Failed!\r\n");
  }

  Status =
      XIic_CfgInitialize(ads->i2c_ctrl_ptr, ConfigPtr, ConfigPtr->BaseAddress);
  if (Status != XST_SUCCESS) {
    xil_printf("[ERROR] XIic_CfgInitialize Failed!\r\n");
  }
  xil_printf("[INFO] I2C device initialized Successfully\r\n");

  /* commented since this won't be used on FPGA
  Wire.begin();
  Wire.setClock(400000);
  */
}

void ads_hal_gpio_init(XGpio *ads_gpio, UINTPTR baseAddr, bool is_output) {
  int Status;
  XGpio_Config *ConfigPtr;
  //初始化LED
  ConfigPtr = XGpio_LookupConfig(baseAddr);
  if (ConfigPtr == NULL) {
    xil_printf("[ERROR] XGpio_LookupConfig Failed!\r\n");
  }

  Status = XGpio_CfgInitialize(ads_gpio, ConfigPtr, ConfigPtr->BaseAddress);
  if (Status != XST_SUCCESS) {
    xil_printf("[ERROR] XIic_CfgInitialize Failed!\r\n");
  }
  //为指定的GPIO信道设置所有独立信号的输出方向
  if (is_output)
    XGpio_SetDataDirection(ads_gpio, 1, 0);
  else
    XGpio_SetDataDirection(ads_gpio, 1, 1);
}
/*
void ads_hal_intc_init(ads_t *ads, const int8_t ads_size) {
  //中断控制器初始化
  XIntc_Initialize(ads->intr_ctrl_ptr, ads->intr_ctrl_baseaddr);
  for (int i = 0; i < ads_size; ++i) {
    isr_t isr_ = {ads[i].i2c_ctrl_ptr, ads[i].i2c_addr};
    //关联中断源和中断处理函数
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
  xil_printf("[INFO] Interrupt controller initialization done!\n\r");
}
*/

/**
 * @brief Write buffer of data to the Angular Displacement Sensor
 *
 * @param buffer[in]    Write buffer
 * @param len           Length of buffer.
 * @return  ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_hal_write_buffer(ads_t *ads, uint8_t *buffer, uint8_t len) {
  // Disable the interrupt
//  if (ads->intr_enabled)
//    XIntc_Disable(ads->intr_ctrl_ptr, ads->intr_vec_id);

  // Write the the buffer to the ADS sensor
  unsigned char *buffer_uch = reinterpret_cast<unsigned char *>(buffer);
  int bytes = I2C_Write_axi(ads->i2c_ctrl_ptr->BaseAddress, ads->i2c_addr, -1,
                            len, buffer_uch);
  xil_printf("[INFO] write %d bytes!\n\r", bytes);

//   // Enable the interrupt
//   if (ads->intr_enabled) {
//     XIntc_Enable(ads->intr_ctrl_ptr, ads->intr_vec_id);
//     // TODO ignored currently
//     // need check will this situation happen or not
//     /*
//     // Read data packet if interrupt was missed
//     if (digitalRead(ADS_INTERRUPT_PIN) == 0) {
//       if (ads_hal_read_buffer(read_buffer, ADS_TRANSFER_SIZE) == ADS_OK) {
//         ads_read_callback(read_buffer);
//       }
//     }
//     */
//   }

  if (bytes == len)
    return ADS_OK;
  else
    return ADS_ERR_IO;
  /*
  // Disable the interrupt
  if(_ads_int_enabled)
      detachInterrupt(digitalPinToInterrupt(ADS_INTERRUPT_PIN));

  Wire.beginTransmission(_address);
  uint8_t nb_written = Wire.write(buffer, len);
  Wire.endTransmission();

  // Enable the interrupt
  if(_ads_int_enabled)
  {
      attachInterrupt(digitalPinToInterrupt(ADS_INTERRUPT_PIN),
  ads_hal_interrupt, FALLING);

      // Read data packet if interrupt was missed
      if(digitalRead(ADS_INTERRUPT_PIN) == 0)
      {
          if(ads_hal_read_buffer(read_buffer, ADS_TRANSFER_SIZE) == ADS_OK)
          {
              ads_read_callback(read_buffer);
          }
      }
  }

  if(nb_written == len)
      return ADS_OK;
  else
      return ADS_ERR_IO;
  */
}

/**
 * @brief Read buffer of data from the Angular Displacement Sensor
 *
 * @param buffer[out]   Read buffer
 * @param len           Length of buffer.
 * @return  ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_hal_read_buffer(ads_t *ads, uint8_t *buffer, uint8_t len) {
  // unsigned char* buffer_uch = reinterpret_cast<unsigned char*>(buffer);
  // int recv_num = I2C_Read_axi(InstancePtr->BaseAddress, I2cAddr, -1, len,
  // buffer_uch);
  int recv_num = I2C_Read_axi(ads->i2c_ctrl_ptr->BaseAddress, ads->i2c_addr, -1,
                              len, buffer);
  if (recv_num == len)
    return ADS_OK;
  else
    return ADS_ERR_IO;

  /* commented since this won't be used on FPGA
  Wire.requestFrom(_address, len);

  uint8_t i = 0;

  while(Wire.available())
  {
      buffer[i] = Wire.read();
      i++;
  }

  if(i == len)
      return ADS_OK;
  else
      return ADS_ERR_IO;
  */
}

/**
 * @brief Reset the Angular Displacement Sensor
 *
 * @param dfuMode   Resets ADS into firmware update mode if true
 */
void ads_hal_reset(ads_t *ads) {
  /*
// Configure reset line as an output
pinMode(ADS_RESET_PIN, OUTPUT);

ads_hal_gpio_pin_write(ADS_RESET_PIN, 0);
ads_hal_delay(10);
ads_hal_gpio_pin_write(ADS_RESET_PIN, 1);

pinMode(ADS_RESET_PIN, INPUT_PULLUP);
  */

  // set reset to output
  u32 gpio_dir = XGpio_GetDataDirection(ads->reset_pins, 1);
  u32 mask = 1 << ads->reset_id;
  gpio_dir = gpio_dir & ~(mask);
  XGpio_SetDataDirection(ads->reset_pins, 1, gpio_dir);

  // write 0 to specific reset
  u32 gpio_val = XGpio_DiscreteRead(ads->reset_pins, 1);
  gpio_val = gpio_val & ~(mask);
  XGpio_DiscreteWrite(ads->reset_pins, 1, gpio_val);

  ads_hal_delay(10);

  // write 0 to specific reset
  gpio_val = XGpio_DiscreteRead(ads->reset_pins, 1);
  gpio_val = gpio_val | mask;
  XGpio_DiscreteWrite(ads->reset_pins, 1, gpio_val);

  // set reset to input
  gpio_dir = gpio_dir | mask;
  XGpio_SetDataDirection(ads->reset_pins, 1, gpio_dir);
}

/**
 * @brief Initializes the hardware abstraction layer
 *
 * @return  ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_hal_init(ads_t *ads) {
  // Reset the ads
  // ads_hal_reset(ads); // TODO

  // Wait for ads to initialize
  ads_hal_delay(2000);

  ads_hal_i2c_init(ads);
  // ads_hal_intc_init(ads, 1);

  // Configure and enable interrupt pin
  // ads_hal_pin_int_init(ads);

  /* commented since this is fixed in FPGA
// Configure I2C bus
Wire.begin();
Wire.setClock(400000);
  */

  return ADS_OK;
}

/**
 * @brief Selects the current device address of the ADS driver is communicating
 * with
 *
 * @param device select device 0 - ADS_COUNT
 * @return  ADS_OK if successful ADS_ERR_BAD_PARAM if invalid device number
 */
int ads_hal_select_device(uint8_t device) {
  /* commented since this won't be used on FPGA
  if (device < ADS_COUNT)
    _address = ads_addrs[device];
  else
    return ADS_ERR_BAD_PARAM;

  return ADS_OK;
  */
  (void)device;
  return ADS_ERR_BAD_PARAM;
}

/**
 * @brief Updates the I2C address in the ads_addrs[] array. Updates the current
 *        selected address.
 *
 * @param   device  device number of the device that is being updated
 * @param   address new address of the ADS
 * @return  ADS_OK if successful ADS_ERR_BAD_PARAM if failed
 */
int ads_hal_update_device_addr(uint8_t device, uint8_t address) {

  /* commented since this won't be used on FPGA
  if (device < ADS_COUNT)
    ads_addrs[device] = address;
  else
    return ADS_ERR_BAD_PARAM;

  _address = address;

  return ADS_OK;
  */
  (void)device;
  (void)address;
  return ADS_ERR_BAD_PARAM;
}

/**
 * @brief Gets the current i2c address that the hal layer is addressing.
 *              Used by device firmware update (dfu)
 * @return  uint8_t _address
 */
uint8_t ads_hal_get_address(ads_t *ads) {
  return XIic_GetAddress(ads->i2c_ctrl_ptr, XII_ADDR_TO_RESPOND_TYPE);

  /* commented since this won't be used on FPGA
  return _address;
  */
}

/**
 * @brief Sets the i2c address that the hal layer is addressing *
 *              Used by device firmware update (dfu)
 */
void ads_hal_set_address(ads_t *ads, uint8_t address) {
  int Status =
      XIic_SetAddress(ads->i2c_ctrl_ptr, XII_ADDR_TO_SEND_TYPE, address);
  if (Status != XST_SUCCESS)
    xil_printf("[ERROR] XIic_SetAddress failed!\r\n");
  /* commented since this won't be used on FPGA
  _address = address;
  */
}
