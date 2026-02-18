#include "bmp280.h"
#include <stdint.h>
#include <stdio.h>

// ------------------------------
// BMP280 I2C address
// ------------------------------
#define BMP280_ADDR_7BIT      (0x76)
#define BMP280_ADDR_8BIT      (BMP280_ADDR_7BIT << 1)   // STM32 HAL uses 8-bit address

// ------------------------------
// Registers
// ------------------------------
#define BMP280_REG_ID         (0xD0)
#define BMP280_REG_RESET      (0xE0)
#define BMP280_REG_STATUS     (0xF3)
#define BMP280_REG_CTRL_MEAS  (0xF4)
#define BMP280_REG_CONFIG     (0xF5)
#define BMP280_REG_PRESS_MSB  (0xF7)   // 0xF7..0xF9
#define BMP280_REG_TEMP_MSB   (0xFA)   // 0xFA..0xFC
#define BMP280_REG_CALIB00    (0x88)   // 0x88..0x9F

// ------------------------------
// Status bits
// ------------------------------
#define BMP280_STATUS_MEASURING   (1U << 3)
#define BMP280_STATUS_IM_UPDATE   (1U << 0)

// ------------------------------
// Calibration (temperature only)
// ------------------------------
static I2C_HandleTypeDef *s_hi2c = NULL;

static uint16_t dig_T1;
static int16_t  dig_T2;
static int16_t  dig_T3;
static int32_t  t_fine;

// ------------------------------
// Low-level I2C helpers
// ------------------------------
static int read_bytes(uint8_t reg, uint8_t *buf, uint16_t len)
{
    return (HAL_I2C_Mem_Read(s_hi2c, BMP280_ADDR_8BIT, reg,
                             I2C_MEMADD_SIZE_8BIT, buf, len, 100) == HAL_OK) ? 0 : -1;
}

static int write_byte(uint8_t reg, uint8_t val)
{
    return (HAL_I2C_Mem_Write(s_hi2c, BMP280_ADDR_8BIT, reg,
                              I2C_MEMADD_SIZE_8BIT, &val, 1, 100) == HAL_OK) ? 0 : -1;
}

static int read_u8(uint8_t reg, uint8_t *val)
{
    return read_bytes(reg, val, 1);
}

static uint16_t u16_le(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[1] << 8 | p[0]);
}

static int16_t s16_le(const uint8_t *p)
{
    return (int16_t)((uint16_t)p[1] << 8 | p[0]);
}

// ------------------------------
// Datasheet compensate T (int32)
// Returns T in 0.01°C
// ------------------------------
static int32_t bmp280_compensate_T_int32(int32_t adc_T)
{
    int32_t var1, var2, T;

    var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) *
              ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) *
            ((int32_t)dig_T3)) >> 14;

    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;

    return T;
}

// ------------------------------
// Wait until not measuring and no NVM copy
// ------------------------------
static int bmp280_wait_ready(uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start) < timeout_ms) {
        uint8_t st = 0;
        if (read_u8(BMP280_REG_STATUS, &st) != 0) return -1;

        if ((st & BMP280_STATUS_MEASURING) == 0 &&
            (st & BMP280_STATUS_IM_UPDATE) == 0) {
            return 0;
        }
        HAL_Delay(1);
    }
    return -2;
}

// ------------------------------
// Init (official style): reset -> read calib -> config -> ctrl_meas
// Keep in sleep; we'll do forced one-shot per read
// ------------------------------
void BMP280_Init(I2C_HandleTypeDef *hi2c)
{
    s_hi2c = hi2c;

    // Soft reset
    (void)write_byte(BMP280_REG_RESET, 0xB6);
    HAL_Delay(100);

    // Wait for NVM copy done (im_update=0)
    (void)bmp280_wait_ready(500);

    // ★ 先確認 chip ID
    uint8_t id = 0;
    int ret = read_u8(BMP280_REG_ID, &id);
    printf("[BMP280] chip_id read ret=%d, id=0x%02X (expect 0x58 or 0x60)\r\n", ret, id);

    // Read calibration block (0x88..0x9F = 24 bytes)
    uint8_t calib[24];

    int cret = read_bytes(BMP280_REG_CALIB00, calib, sizeof(calib));
    printf("[BMP280] calib read ret=%d\r\n", cret);
    printf("[BMP280] calib raw: %02X%02X %02X%02X %02X%02X\r\n",
           calib[0],calib[1],calib[2],calib[3],calib[4],calib[5]);



    if (read_bytes(BMP280_REG_CALIB00, calib, sizeof(calib)) == 0) {
        dig_T1 = u16_le(&calib[0]);   // 0x88/0x89
        dig_T2 = s16_le(&calib[2]);   // 0x8A/0x8B
        dig_T3 = s16_le(&calib[4]);   // 0x8C/0x8D
    }

    printf("[BMP280] T1=%u T2=%d T3=%d\r\n", dig_T1, dig_T2, dig_T3);

    // Put device into sleep before writing config/ctrl (recommended)
    (void)write_byte(BMP280_REG_CTRL_MEAS, 0x00);

    // CONFIG: t_sb=62.5ms(001), filter=off(000), spi3w=0
    // config = (t_sb<<5) | (filter<<2) | spi3w
    (void)write_byte(BMP280_REG_CONFIG, (uint8_t)(0x01 << 5));

    // Keep sleep; set oversampling defaults:
    // CTRL_MEAS: osrs_t=x1 (001), osrs_p=x1 (001), mode=sleep (00)
    (void)write_byte(BMP280_REG_CTRL_MEAS, (uint8_t)((0x01 << 5) | (0x01 << 2) | 0x00));


    // ★ 重試寫入 ctrl_meas，直到成功
    uint8_t readback = 0;
    for (int retry = 0; retry < 10; retry++) {
        write_byte(BMP280_REG_CTRL_MEAS, (uint8_t)((0x01 << 5) | (0x01 << 2) | 0x00));
        HAL_Delay(10);
        read_u8(BMP280_REG_CTRL_MEAS, &readback);
        printf("[BMP280] ctrl_meas try %d: 0x%02X\r\n", retry, readback);
        if (readback == 0x24) break;
    }

    if (readback != 0x24) {
        printf("[BMP280] Init FAILED!\r\n");
    }


    HAL_I2C_DeInit(s_hi2c);
    HAL_I2C_Init(s_hi2c);

    //uint8_t readback = 0;
    read_u8(BMP280_REG_CTRL_MEAS, &readback);
    printf("[BMP280] ctrl_meas readback: 0x%02X\r\n", readback);

    uint8_t chk[6];
    read_bytes(BMP280_REG_PRESS_MSB, chk, 6);
    printf("[BMP280] sleep regs: %02X %02X %02X %02X %02X %02X\r\n",
       chk[0],chk[1],chk[2],chk[3],chk[4],chk[5]);


}


int32_t BMP280_ReadTemperature(void)
{
    if (s_hi2c == NULL) return 0;

    if (write_byte(BMP280_REG_CTRL_MEAS,
                   (uint8_t)((0x01 << 5) | (0x01 << 2) | 0x01)) != 0)
        return 0;

    HAL_Delay(15);

    // 分開讀溫度，不要 burst read 跨壓力暫存器
    uint8_t t[3];
    if (read_bytes(BMP280_REG_TEMP_MSB, t, 3) != 0)  // 只讀 0xFA, 0xFB, 0xFC
        return 0;

    printf("[BMP280] temp raw: %02X %02X %02X\r\n", t[0], t[1], t[2]);

    int32_t adc_T = ((int32_t)t[0] << 12) |
                    ((int32_t)t[1] << 4)  |
                    ((int32_t)t[2] >> 4);

    return bmp280_compensate_T_int32(adc_T);
}