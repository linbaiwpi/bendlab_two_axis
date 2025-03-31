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

#ifndef ADS_TWO_AXIS_HAL_
#define ADS_TWO_AXIS_HAL_

#include "ads_two_axis_err.h"
#include <stdint.h>

#include "xgpio.h"
#include "xiic.h"
#include "xintc.h"

typedef struct {
  // user added
  XIic *i2c_ctrl_ptr;   // pointer of i2c controller on microblaze
  u32 i2c_ctrl_baseaddr;
  uint8_t i2c_addr;     // i2c device address
  XIntc *intr_ctrl_ptr; // pointer of axi interrupt controller
  u32 intr_ctrl_baseaddr;
  uint8_t intr_vec_id;  // interrupt vector ID
  XGpio *reset_pins;      // pointer of gpio to connect all reset
  uint8_t reset_id;  // reset pin index of current
  bool intr_enabled; // indicate interrupt mode is enabled or not
} ads_t;

typedef struct {
    XIic *InstancePtr;
    u32 I2cAddr;
} isr_t;

#define ADS_TRANSFER_SIZE (5)

#define ADS_COUNT (10) // Number of ADS devices attached to bus

void ads_hal_delay(uint16_t delay_ms);
void delay(uint16_t delay_ms);

void ads_hal_pin_int_enable(ads_t *ads_init, bool enable);

/**
 * @brief Write buffer of data to the Angular Displacement Sensor
 *
 * @param buffer[in]	Write buffer
 * @param len			Length of buffer.
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_hal_write_buffer(ads_t *ads, uint8_t *buffer, uint8_t len);

/**
 * @brief Read buffer of data from the Angular Displacement Sensor
 *
 * @param buffer[out]	Read buffer
 * @param len			Length of buffer.
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_hal_read_buffer(ads_t *ads, uint8_t *buffer, uint8_t len);

/**
 * @brief Reset the Angular Displacement Sensor
 */
void ads_hal_reset(ads_t *ads_init);

/**
 * @brief Initializes the hardware abstraction layer
 *
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_hal_init(ads_t *ads_init);

/**
 * @brief Selects the current device address the driver is communicating with
 *
 * @param device select device 0 - ADS_COUNT
 * @return	ADS_OK if successful ADS_ERR_BAD_PARAM if invalid device number
 */
int ads_hal_select_device(uint8_t device);

/**
 * @brief Updates the I2C address in the ads_addrs[] array. Updates the current
 *		  selected address.
 *
 * @param	device	device number of the device that is being updated
 * @param	address	new address of the ADS
 * @return	ADS_OK if successful ADS_ERR_BAD_PARAM if failed
 */
int ads_hal_update_device_addr(uint8_t device, uint8_t address);

/**
 * @brief Gets the current i2c address that the hal layer is addressing.
 *				Used by device firmware update (dfu)
 * @return	uint8_t _address
 */
uint8_t ads_hal_get_address(ads_t *ads);

/**
 * @brief Sets the current i2c address that the hal layer is addressing.
 *				Used by device firmware update (dfu)
 */
void ads_hal_set_address(ads_t *ads, uint8_t address);


void ads_hal_i2c_init(ads_t *ads);
void ads_hal_intc_init(ads_t *ads, const int8_t ads_size);


#endif /* ADS_TWO_AXIS_HAL_ */
