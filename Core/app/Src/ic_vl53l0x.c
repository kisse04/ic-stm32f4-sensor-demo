/**
 * VL53L0X time-of-flight sensor driver.
 *
 * This implementation provides a minimal single-shot ranging flow, with
 * ic_status_t error mapping and logging.
 */

#include "ic_vl53l0x.h"
#include "ic_logger.h"

/* ---------- Register addresses ---------- */

#define IC_VL53L0X_REG_SYSRANGE_START                 0x00U
#define IC_VL53L0X_REG_SYSTEM_INTERRUPT_CONFIG_GPIO   0x0AU
#define IC_VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR         0x0BU
#define IC_VL53L0X_REG_RESULT_INTERRUPT_STATUS        0x13U
#define IC_VL53L0X_REG_RESULT_RANGE_STATUS            0x14U
#define IC_VL53L0X_REG_IDENTIFICATION_MODEL_ID        0xC0U

/** RESULT_RANGE_STATUS + 10 contains a 16-bit range result in millimeters. */
#define IC_VL53L0X_REG_RESULT_DISTANCE_MILLI_HI \
    (IC_VL53L0X_REG_RESULT_RANGE_STATUS + 10U)

/** Expected model ID value (from datasheet / reference code). */
#define IC_VL53L0X_MODEL_ID_EXPECTED                  0xEEU

/** Global VL53L0X handle for convenience APIs. */
ic_vl53l0x_handle_t g_ic_vl53l0x = {
    .hi2c          = NULL,
    .i2c_addr      = IC_VL53L0X_I2C_ADDR_DEFAULT,
    .is_initialized = 0U
};

/* ---------- Internal I2C helpers ---------- */

static ic_status_t
ic_vl53l0x_i2c_read_reg (ic_vl53l0x_handle_t* dev,
                         uint8_t reg,
                         uint8_t* buf,
                         uint16_t len)
{
    if ((dev == NULL) || (dev->hi2c == NULL)) {
        return IC_STATUS_BOARD_CONFIG_ERROR;
    }

    HAL_StatusTypeDef st = HAL_I2C_Mem_Read (dev->hi2c,
                                             dev->i2c_addr,
                                             reg,
                                             I2C_MEMADD_SIZE_8BIT,
                                             buf,
                                             len,
                                             100U);
    switch (st)
    {
        case HAL_OK:
            return IC_STATUS_OK;
        case HAL_BUSY:
            return IC_STATUS_I2C_BUSY;
        case HAL_TIMEOUT:
            return IC_STATUS_I2C_TIMEOUT;
        case HAL_ERROR:
        default:
            return IC_STATUS_BUS_ERROR;
    }
}

static ic_status_t
ic_vl53l0x_i2c_write_reg (ic_vl53l0x_handle_t* dev,
                          uint8_t reg,
                          const uint8_t* buf,
                          uint16_t len)
{
    if ((dev == NULL) || (dev->hi2c == NULL)) {
        return IC_STATUS_BOARD_CONFIG_ERROR;
    }

    HAL_StatusTypeDef st = HAL_I2C_Mem_Write (dev->hi2c,
                                              dev->i2c_addr,
                                              reg,
                                              I2C_MEMADD_SIZE_8BIT,
                                              (uint8_t*) buf,
                                              len,
                                              100U);
    switch (st)
    {
        case HAL_OK:
            return IC_STATUS_OK;
        case HAL_BUSY:
            return IC_STATUS_I2C_BUSY;
        case HAL_TIMEOUT:
            return IC_STATUS_I2C_TIMEOUT;
        case HAL_ERROR:
        default:
            return IC_STATUS_BUS_ERROR;
    }
}

/* ---------- Public APIs ---------- */

ic_status_t
ic_vl53l0x_init (ic_vl53l0x_handle_t* dev,
                 I2C_HandleTypeDef* hi2c,
                 uint8_t i2c_addr)
{
    if ((dev == NULL) || (hi2c == NULL)) {
        return IC_STATUS_BOARD_CONFIG_ERROR;
    }

    dev->hi2c           = hi2c;
    dev->i2c_addr       = i2c_addr;
    dev->is_initialized = 0U;

    /* Read and validate model ID. */
    uint8_t     id = 0U;
    ic_status_t st = ic_vl53l0x_i2c_read_reg (dev,
                                              IC_VL53L0X_REG_IDENTIFICATION_MODEL_ID,
                                              &id,
                                              1U);
    if (!IC_STATUS_IS_OK (st)) {
        ic_log_printf ("[VL53L0X] read model ID failed (status=%d)\r\n", st);
        return st;
    }

    ic_log_printf ("[VL53L0X] model id = 0x%02X (expect 0x%02X)\r\n",
                   id, IC_VL53L0X_MODEL_ID_EXPECTED);

    if (id != IC_VL53L0X_MODEL_ID_EXPECTED) {
        return IC_STATUS_VL53L0X_NOT_DETECTED;
    }

    /* Configure interrupt behavior and clear any pending interrupt.
     * This matches the original working implementation.
     */
    uint8_t cfg = 0x04U;
    st = ic_vl53l0x_i2c_write_reg (dev,
                                   IC_VL53L0X_REG_SYSTEM_INTERRUPT_CONFIG_GPIO,
                                   &cfg,
                                   1U);
    if (!IC_STATUS_IS_OK (st)) {
        ic_log_printf ("[VL53L0X] write INT_CONFIG failed (status=%d)\r\n", st);
        return st;
    }

    uint8_t clr = 0x01U;
    st = ic_vl53l0x_i2c_write_reg (dev,
                                   IC_VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR,
                                   &clr,
                                   1U);
    if (!IC_STATUS_IS_OK (st)) {
        ic_log_printf ("[VL53L0X] clear INT failed (status=%d)\r\n", st);
        return st;
    }

    dev->is_initialized = 1U;
    ic_log_printf ("[VL53L0X] init OK\r\n");
    return IC_STATUS_OK;
}

ic_status_t
ic_vl53l0x_read_distance_mm (ic_vl53l0x_handle_t* dev,
                             uint16_t* distance_mm)
{
    if ((dev == NULL) || (distance_mm == NULL)) {
        return IC_STATUS_BOARD_CONFIG_ERROR;
    }
    if (dev->is_initialized == 0U) {
        return IC_STATUS_VL53L0X_INIT_FAILED;
    }

    uint8_t  buf[2]     = { 0U };
    uint8_t  status_reg = 0U;
    uint8_t  cmd        = 0U;
    uint32_t start_tick = 0U;

    /* Start single-shot ranging. */
    cmd = 0x01U;
    ic_status_t st = ic_vl53l0x_i2c_write_reg (dev,
                                               IC_VL53L0X_REG_SYSRANGE_START,
                                               &cmd,
                                               1U);
    if (!IC_STATUS_IS_OK (st)) {
        ic_log_printf ("[VL53L0X] start ranging failed (status=%d)\r\n", st);
        return st;
    }

    /* Poll interrupt status until measurement is done or timeout. */
    start_tick = HAL_GetTick ();
    for (;;)
    {
        st = ic_vl53l0x_i2c_read_reg (dev,
                                      IC_VL53L0X_REG_RESULT_INTERRUPT_STATUS,
                                      &status_reg,
                                      1U);
        if (!IC_STATUS_IS_OK (st)) {
            return st;
        }

        if ((status_reg & 0x07U) != 0U) {
            /* Measurement ready. */
            break;
        }

        if ((HAL_GetTick () - start_tick) > IC_VL53L0X_RANGE_TIMEOUT_MS) {
            ic_log_printf ("[VL53L0X] range timeout\r\n");
            return IC_STATUS_VL53L0X_READ_FAILED;
        }
    }

    /* Read distance (two bytes, mm).  Offset matches original working code:
     * RESULT_RANGE_STATUS + 10.
     */
    st = ic_vl53l0x_i2c_read_reg (dev,
                                  IC_VL53L0X_REG_RESULT_DISTANCE_MILLI_HI,
                                  buf,
                                  2U);
    if (!IC_STATUS_IS_OK (st)) {
        ic_log_printf ("[VL53L0X] read distance failed (status=%d)\r\n", st);
        return IC_STATUS_VL53L0X_READ_FAILED;
    }

    *distance_mm = (uint16_t) ((((uint16_t) buf[0]) << 8) | (uint16_t) buf[1]);

    /* Optionally check range and map to OUT_OF_RANGE, if desired. */
#if 0
    if (*distance_mm > 2000U) {  /* Example: treat >2 m as out-of-range. */
        return IC_STATUS_VL53L0X_OUT_OF_RANGE;
    }
#endif

    /* Best-effort clear interrupt flag. */
    cmd = 0x01U;
    (void) ic_vl53l0x_i2c_write_reg (dev,
                                     IC_VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR,
                                     &cmd,
                                     1U);

    return IC_STATUS_OK;
}

ic_status_t
ic_vl53l0x_get_distance_mm (uint16_t* distance_mm)
{
    return ic_vl53l0x_read_distance_mm (&g_ic_vl53l0x, distance_mm);
}