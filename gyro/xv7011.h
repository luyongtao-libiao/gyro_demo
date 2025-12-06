#ifndef __XV7011_H
#define __XV7011_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================
// XV7011寄存器地址定义
// ============================================================

// 用户命令寄存器（第6章 Table 6 User command register）
#define XV7011_REG_RESERVED00    0x00  // 保留
#define XV7011_REG_DSPCTL1       0x01  // DSP设置1
#define XV7011_REG_DSPCTL2       0x02  // DSP设置2
#define XV7011_REG_DSPCTL3       0x03  // DSP设置3
#define XV7011_REG_STSRD         0x04  // 状态读取
#define XV7011_REG_SLPIN         0x05  // 睡眠模式
#define XV7011_REG_SLPOUT        0x06  // 睡眠唤醒
#define XV7011_REG_STBY          0x07  // 待机模式
#define XV7011_REG_TEMPRD        0x08  // 温度传感器数据读取
#define XV7011_REG_SWRST         0x09  // 软件复位
#define XV7011_REG_DATACCON      0x0A  // 角速度数据读取
#define XV7011_REG_OUTCTL1       0x0B  // 角速度数据读取控制

// 温度传感器数据格式寄存器（表6.15）
#define XV7011_REG_TSDATAFORMAT  0x1C

// 串行接口设置寄存器（表6.16）
#define XV7011_REG_SERIAL_CTL    0x1F

// ============================================================
// 寄存器位定义
// ============================================================

// 状态寄存器（表6.4）位定义
typedef enum {
	XV7011_STS_PROC_OK_BIT = 3, // 温度数据就绪标志
	XV7011_STS_POR_BIT = 2, // 上电后状态
	XV7011_STS_STBY_BIT = 1, // 待机状态
	XV7011_STS_SLPOUT_BIT = 0      // 睡眠唤醒状态
} XV7011_StatusBits;

// 状态组合（表6.4 Note 1）
typedef enum {
	XV7011_STATE_POWERON = 0x04, // 100: 上电后
	XV7011_STATE_STANDBY = 0x02, // 010: 待机
	XV7011_STATE_SLEEP = 0x00,
	// 000: 睡眠
	XV7011_STATE_SLEEP_OUT = 0x01   // 001: 睡眠唤醒
} XV7011_State;

// 角速度数据格式（表6.11 bit2）
typedef enum {
	XV7011_DATA_FORMAT_16BIT = 0, // 16位输出
	XV7011_DATA_FORMAT_24BIT = 1      // 24位输出
} XV7011_DataFormat;

// 角速度输出控制（表6.11 bit1-0）
typedef enum {
	XV7011_OUTCTL_RESERVED00 = 0x00,
	// 00: 保留
	XV7011_OUTCTL_ANG_RATE = 0x01,
	// 01: 角速度数据输出（默认）
	XV7011_OUTCTL_RESERVED10 = 0x02, // 10: 保留
	XV7011_OUTCTL_RESERVED11 = 0x03   // 11: 保留
} XV7011_OutputControl;

// 温度传感器数据格式（表6.15 bit6-5）
typedef enum {
	XV7011_TS_FORMAT_8BIT = 0x00, // 00: 8位输出
	XV7011_TS_FORMAT_10BIT = 0x01, // 01: 10位输出
	XV7011_TS_FORMAT_12BIT = 0x02, // 10: 12位输出（默认）
	XV7011_TS_FORMAT_RESERVED = 0x03   // 11: 不可用
} XV7011_TempDataFormat;

// SPI接口选择（表6.16 bit1）
typedef enum {
	XV7011_SPI_4WIRE = 0, // 0: 4线SPI（默认）
	XV7011_SPI_3WIRE = 1      // 1: 3线SPI
} XV7011_SPIMode;

// I2C使能（表6.16 bit0）
typedef enum {
	XV7011_I2C_DISABLE = 0, // 0: I2C禁用
	XV7011_I2C_ENABLE = 1      // 1: I2C使能（默认）
} XV7011_I2CEnable;

// ============================================================
// 比例因子定义（表3.5）
// ============================================================
#define XV7011_SCALE_FACTOR_16BIT    280.0f      // LSB/(°/s)
#define XV7011_SCALE_FACTOR_24BIT    71680.0f    // LSB/(°/s)

// ============================================================
// 采样率定义（表3.5）
// ============================================================
#define XV7011_SAMPLE_RATE_H         13770.0f    // Hz，频率码H
#define XV7011_SAMPLE_RATE_J         14160.0f    // Hz，频率码J

// ============================================================
// 操作时序定义（第3-4节）
// ============================================================
#define XV7011_T_IF_DELAY_US         1000        // 串行通信等待时间tIF
#define XV7011_T_TSEN_DELAY_US       1000        // 温度传感器数据采集等待tTSEN
#define XV7011_T_STA_DELAY_US        1000        // 角速度数据采集等待tSTA

// ============================================================
// 数据结构定义
// ============================================================
#pragma pack(push, 1)
typedef struct {
	float angular_rate; // 角速度 (°/s)
	float temperature; // 温度 (°C)
	uint32_t timestamp_ms; // 时间戳 (ms)
	uint8_t data_valid; // 数据有效标志: 0=无效, 1=有效
} XV7011_Data_t;
#pragma pack(pop)

// ============================================================
// 函数声明
// ============================================================

// 初始化函数
uint8_t XV7011_Init(void);
uint8_t XV7011_Reset(void);

// 数据读取函数
int8_t XV7011_ReadTemperature(float *temp);
int8_t XV7011_ReadAngularRate(float *rate);
int8_t XV7011_ReadAllData(XV7011_Data_t *data);

// 寄存器操作函数
uint8_t XV7011_WriteRegister(uint8_t reg_addr, uint8_t value);
uint8_t XV7011_ReadRegister(uint8_t reg_addr, uint8_t *value);

// 控制函数
uint8_t XV7011_EnterSleep(void);
uint8_t XV7011_ExitSleep(void);
uint8_t XV7011_EnterStandby(void);
uint8_t XV7011_CalibrateZeroRate(void);

// 状态检查函数
uint8_t XV7011_CheckTemperatureReady(void);
uint8_t XV7011_GetStatus(uint8_t *status);

// 配置函数
uint8_t XV7011_SetDataFormat(XV7011_DataFormat format);
uint8_t XV7011_SetTempDataFormat(XV7011_TempDataFormat format);
uint8_t XV7011_SetOutputControl(XV7011_OutputControl ctrl);

// 辅助函数
void XV7011_DelayUs(uint32_t us);
void XV7011_ChipSelect(uint8_t state);

#endif /* __XV7011_H */
