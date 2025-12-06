#include "xv7011.h"
#include "stm32f1xx_hal.h"    // STM32 HAL库
#include "cmsis_os.h"         // FreeRTOS osDelay

// 外部声明SPI2句柄（在main.c或gyro.c中定义）
extern SPI_HandleTypeDef hspi2;

// ============================================================
// 静态全局变量
// ============================================================
static uint8_t xv7011_initialized = 0;
static XV7011_DataFormat current_format = XV7011_DATA_FORMAT_16BIT;
static float scale_factor = XV7011_SCALE_FACTOR_16BIT;

// ============================================================
// 私有函数声明
// ============================================================
static uint8_t XV7011_SPI_Transfer(uint8_t tx_data);
static void XV7011_WaitIF(void);
static void XV7011_WaitTSEN(void);
static void XV7011_WaitSTA(void);
static float XV7011_ConvertTemperature(uint16_t raw_temp);

// ============================================================
// SPI读写函数
// ============================================================

/**
 * @brief 单字节SPI传输
 * @param tx_data 要发送的数据
 * @return 接收到的数据
 */
static uint8_t XV7011_SPI_Transfer(uint8_t tx_data)
{
	uint8_t rx_data = 0;
	
	// 使用HAL库进行SPI传输
	HAL_SPI_TransmitReceive(&hspi2, &tx_data, &rx_data, 1, 100);
	
	return rx_data;
}

/**
 * @brief 片选控制
 * @param state 0=取消片选, 1=选中
 */
void XV7011_ChipSelect(uint8_t state)
{
	if (state) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET); // CS低电平有效
	}
	else {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET); // CS高电平
	}
}

// ============================================================
// 等待时序函数（第3-4节）
// ============================================================

/**
 * @brief 等待串行通信准备时间tIF
 */
static void XV7011_WaitIF(void)
{

}

/**
 * @brief 等待温度传感器准备时间tTSEN
 */
static void XV7011_WaitTSEN(void)
{

}

/**
 * @brief 等待角速度数据准备时间tSTA
 */
static void XV7011_WaitSTA(void)
{

}

// ============================================================
// 寄存器操作函数
// ============================================================

/**
 * @brief 写寄存器
 * @param reg_addr 寄存器地址
 * @param value 要写入的值
 * @return 0=成功, 其他=错误
 */
uint8_t XV7011_WriteRegister(uint8_t reg_addr, uint8_t value)
{
	uint8_t address_byte;
    
	// 等待串行通信准备（第3-4节Note 1）
	XV7011_WaitIF();
    
	// 片选有效
	XV7011_ChipSelect(1);
    
	// 地址字节构造（第5-1节）
	// MSB=0表示写操作，bit[6:5]=00（多从机功能禁用），bit[4:0]=寄存器地址
	address_byte = reg_addr & 0x1F; // 只取低5位
    
	// 发送地址字节
	XV7011_SPI_Transfer(address_byte);
    
	// 发送数据字节
	XV7011_SPI_Transfer(value);
    
	// 取消片选
	XV7011_ChipSelect(0);
    
	return 0;
}

/**
 * @brief 读寄存器
 * @param reg_addr 寄存器地址
 * @param value 读取到的值
 * @return 0=成功, 其他=错误
 */
uint8_t XV7011_ReadRegister(uint8_t reg_addr, uint8_t *value)
{
	uint8_t address_byte;
	uint8_t dummy;
    
	// 等待串行通信准备
	XV7011_WaitIF();
    
	// 片选有效
	XV7011_ChipSelect(1);
    
	// 地址字节构造（第5-1节）
	// MSB=1表示读操作，bit[6:5]=00，bit[4:0]=寄存器地址
	address_byte = 0x80 | (reg_addr & 0x1F);
    
	// 发送地址字节
	XV7011_SPI_Transfer(address_byte);
    
	// 发送一个时钟周期读取数据（第5-1-2节）
	//dummy = XV7011_SPI_Transfer(0x00);
    
	// 读取数据（第5-1-2节说明）
	*value = XV7011_SPI_Transfer(0x00);
    
	// 取消片选
	XV7011_ChipSelect(0);
    
	return 0;
}

// ============================================================
// 初始化函数
// ============================================================

/**
 * @brief XV7011初始化
 * @return 0=成功, 其他=错误
 */
uint8_t XV7011_Init(void)
{
	uint8_t status;
	uint8_t reg_value;
	
	// CS引脚初始化（PB12）
	// 注意：GPIO时钟和基本配置应该在SPI2初始化时已完成
	// 这里只确保CS为高电平
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
	
	// 等待电源稳定
	osDelay(10); // 10ms
	
	// 读取状态寄存器验证通信
	if (XV7011_ReadRegister(XV7011_REG_STSRD, &reg_value) != 0) {
		return 1;  // 通信失败
	}
	
	// 设置角速度数据格式为16位（默认）
	current_format = XV7011_DATA_FORMAT_16BIT;
	scale_factor = XV7011_SCALE_FACTOR_16BIT;
	XV7011_SetDataFormat(current_format);
	
	// 设置角速度输出控制为角速度数据输出（默认01）
	XV7011_SetOutputControl(XV7011_OUTCTL_ANG_RATE);
	
	// 设置温度传感器数据格式为12位（默认10）
	XV7011_SetTempDataFormat(XV7011_TS_FORMAT_12BIT);
	
	// 确保在正常操作模式（如果不是）
	XV7011_ExitSleep();
	
	xv7011_initialized = 1;
	return 0;
}

/**
 * @brief 软件复位
 * @return 0=成功, 其他=错误
 */
uint8_t XV7011_Reset(void)
{
	// 发送软件复位命令（第6-9节）
	XV7011_WaitIF();
	XV7011_ChipSelect(1);
	XV7011_SPI_Transfer(XV7011_REG_SWRST); // 地址0x09
	XV7011_ChipSelect(0);
	
	// 等待复位完成
	osDelay(10); // 10ms
	
	// 重新初始化配置
	XV7011_Init();
	
	return 0;
}

// ============================================================
// 温度传感器相关函数
// ============================================================

/**
 * @brief 转换原始温度值为实际温度
 * @param raw_temp 原始温度值（12位）
 * @return 实际温度（°C）
 */
static float XV7011_ConvertTemperature(uint16_t raw_temp)
{
	// 根据文档，温度传感器输出为2的补码格式
	// 需要转换为实际温度值
	// 具体转换公式需要根据传感器特性确定，这里提供通用方法
    
	// 如果是12位有符号数，先进行符号扩展
	int16_t signed_temp;
	if (raw_temp & 0x0800) {
		// 检查符号位
		signed_temp = (int16_t)(raw_temp | 0xF000); // 符号扩展
	}
	else {
		signed_temp = (int16_t)raw_temp;
	}
    
	// 转换为温度值（需要根据具体传感器特性调整）
	// 假设典型转换系数：1LSB = 0.1°C，零点在25°C
	return 25.0f + ((float)signed_temp - 400.0f) / 16.0f;
}

/**
 * @brief 检查温度数据是否就绪
 * @return 0=未就绪, 1=就绪
 */
uint8_t XV7011_CheckTemperatureReady(void)
{
	uint8_t status_reg;
    
	if (XV7011_ReadRegister(XV7011_REG_STSRD, &status_reg) != 0) {
		return 0;
	}
    
	// 检查ProcOK位（第6-4节）
	return (status_reg >> XV7011_STS_PROC_OK_BIT) & 0x01;
}

/**
 * @brief 读取温度值
 * @param temp 温度值输出（°C）
 * @return 0=成功, 1=未初始化, 2=数据未就绪, 3=读取失败
 */
int8_t XV7011_ReadTemperature(float *temp)
{
	uint8_t temp_high, temp_low;
	uint16_t raw_temp;
    
	if (!xv7011_initialized) {
		return 1;
	}
    
	// 检查温度数据是否就绪（可选）
	// if (!XV7011_CheckTemperatureReady()) {
	//     return 2;
	// }
    
	// 等待温度传感器准备（第3-4节Note 2）
	XV7011_WaitTSEN();
    
	// 读取温度数据（第5-6节）
	XV7011_WaitIF();
	XV7011_ChipSelect(1);
    
	// 发送读温度命令（地址0x08，MSB=1表示读）
	XV7011_SPI_Transfer(0x80 | XV7011_REG_TEMPRD);
    
	// 读取第一个字节（对于12位格式：D[11:4]）
	temp_high = XV7011_SPI_Transfer(0x00);
    
	// 读取第二个字节（对于12位格式：D[3:0]在低4位）
	temp_low = XV7011_SPI_Transfer(0x00);
    
	XV7011_ChipSelect(0);
    
	// 组合12位数据（默认格式为12位）
	raw_temp = ((uint16_t)temp_high << 4) | (temp_low >> 4);
    
	// 转换为实际温度
	*temp = XV7011_ConvertTemperature(raw_temp);
    
	return 0;
}

// ============================================================
// 角速度相关函数
// ============================================================

/**
 * @brief 设置角速度数据格式
 * @param format 数据格式：16位或24位
 * @return 0=成功, 其他=错误
 */
uint8_t XV7011_SetDataFormat(XV7011_DataFormat format)
{
	uint8_t reg_value;
    
	// 读取当前寄存器值
	if (XV7011_ReadRegister(XV7011_REG_OUTCTL1, &reg_value) != 0) {
		return 1;
	}
    
	// 设置DataFormat位（bit2）
	reg_value &= ~(1 << 2); // 清除bit2
	reg_value |= (format << 2); // 设置bit2
    
	// 写回寄存器
	if (XV7011_WriteRegister(XV7011_REG_OUTCTL1, reg_value) != 0) {
		return 2;
	}
    
	// 更新当前格式和比例因子
	current_format = format;
	if (format == XV7011_DATA_FORMAT_16BIT) {
		scale_factor = XV7011_SCALE_FACTOR_16BIT;
	}
	else {
		scale_factor = XV7011_SCALE_FACTOR_24BIT;
	}
    
	return 0;
}

/**
 * @brief 读取角速度值
 * @param rate 角速度值输出（°/s）
 * @return 0=成功, 1=未初始化, 2=读取失败
 */
int8_t XV7011_ReadAngularRate(float *rate)
{
	uint8_t data_high, data_low, data_extra;
	int16_t raw_rate_16;
	int32_t raw_rate_24;
    
	if (!xv7011_initialized) {
		return 1;
	}
    
	// 等待角速度数据准备（第3-4节Note 3）
	XV7011_WaitSTA();
    
	// 读取角速度数据（第5-5节）
	XV7011_WaitIF();
	XV7011_ChipSelect(1);
    
	// 发送读角速度命令（地址0x0A，MSB=1表示读）
	XV7011_SPI_Transfer(0x80 | XV7011_REG_DATACCON);
    
	if (current_format == XV7011_DATA_FORMAT_16BIT) {
		// 16位模式：读取2个字节
		data_high = XV7011_SPI_Transfer(0x00);
		data_low = XV7011_SPI_Transfer(0x00);
        
		// 组合16位有符号数
		raw_rate_16 = (int16_t)((data_high << 8) | data_low);
        
		// 转换为角速度（°/s）
		*rate = (float)raw_rate_16 / scale_factor;
        
	}
	else {
		// 24位模式：读取3个字节
		data_high = XV7011_SPI_Transfer(0x00);
		data_extra = XV7011_SPI_Transfer(0x00);
		data_low = XV7011_SPI_Transfer(0x00);
        
		// 组合24位有符号数（需要符号扩展到32位）
		raw_rate_24 = (int32_t)((data_high << 16) | (data_extra << 8) | data_low);
        
		// 如果是负数，进行符号扩展
		if (data_high & 0x80) {
			raw_rate_24 |= 0xFF000000;
		}
        
		// 转换为角速度（°/s）
		*rate = (float)raw_rate_24 / scale_factor;
	}
    
	XV7011_ChipSelect(0);
    
	return 0;
}

// ============================================================
// 组合读取函数
// ============================================================

/**
 * @brief 同时读取温度和角速度
 * @param data 数据输出结构体
 * @return 0=成功, 其他=错误
 */
int8_t XV7011_ReadAllData(XV7011_Data_t *data)
{
	int8_t ret;
	
	// 获取时间戳（使用HAL库滴答计数）
	data->timestamp_ms = HAL_GetTick();
	
	// 读取温度
	ret = XV7011_ReadTemperature(&data->temperature);
	if (ret != 0) {
		data->data_valid = 0;
		return ret;
	}
	
	// 读取角速度
	ret = XV7011_ReadAngularRate(&data->angular_rate);
	if (ret != 0) {
		data->data_valid = 0;
		return ret;
	}
	
	// 设置有效标志
	data->data_valid = 1;
	
	return 0;
}

// ============================================================
// 其他配置函数
// ============================================================

/**
 * @brief 设置温度传感器数据格式
 * @param format 温度数据格式
 * @return 0=成功, 其他=错误
 */
uint8_t XV7011_SetTempDataFormat(XV7011_TempDataFormat format)
{
	uint8_t reg_value;
    
	// 读取当前寄存器值
	if (XV7011_ReadRegister(XV7011_REG_TSDATAFORMAT, &reg_value) != 0) {
		return 1;
	}
    
	// 设置TsDataFormat位（bit6-5）
	reg_value &= ~(0x03 << 5); // 清除bit6-5
	reg_value |= (format << 5); // 设置bit6-5
    
	// 写回寄存器
	if (XV7011_WriteRegister(XV7011_REG_TSDATAFORMAT, reg_value) != 0) {
		return 2;
	}
    
	return 0;
}

/**
 * @brief 设置输出控制
 * @param ctrl 输出控制模式
 * @return 0=成功, 其他=错误
 */
uint8_t XV7011_SetOutputControl(XV7011_OutputControl ctrl)
{
	uint8_t reg_value;
    
	// 读取当前寄存器值
	if (XV7011_ReadRegister(XV7011_REG_OUTCTL1, &reg_value) != 0) {
		return 1;
	}
    
	// 设置OutCtl位（bit1-0）
	reg_value &= ~0x03; // 清除bit1-0
	reg_value |= ctrl; // 设置bit1-0
    
	// 写回寄存器
	if (XV7011_WriteRegister(XV7011_REG_OUTCTL1, reg_value) != 0) {
		return 2;
	}
    
	return 0;
}

// ============================================================
// 控制函数
// ============================================================

/**
 * @brief 进入睡眠模式
 * @return 0=成功, 其他=错误
 */
uint8_t XV7011_EnterSleep(void)
{
	// 发送睡眠命令（第6-5节）
	XV7011_WaitIF();
	XV7011_ChipSelect(1);
	XV7011_SPI_Transfer(XV7011_REG_SLPIN); // 地址0x05
	XV7011_ChipSelect(0);
    
	return 0;
}

/**
 * @brief 退出睡眠模式
 * @return 0=成功, 其他=错误
 */
uint8_t XV7011_ExitSleep(void)
{
	// 发送睡眠唤醒命令（第6-6节）
	XV7011_WaitIF();
	XV7011_ChipSelect(1);
	XV7011_SPI_Transfer(XV7011_REG_SLPOUT); // 地址0x06
	XV7011_ChipSelect(0);
    
	return 0;
}

/**
 * @brief 进入待机模式
 * @return 0=成功, 其他=错误
 */
uint8_t XV7011_EnterStandby(void)
{
	// 发送待机命令（第6-7节）
	XV7011_WaitIF();
	XV7011_ChipSelect(1);
	XV7011_SPI_Transfer(XV7011_REG_STBY); // 地址0x07
	XV7011_ChipSelect(0);
    
	return 0;
}

/**
 * @brief 零速率校准
 * @return 0=成功, 其他=错误
 */
uint8_t XV7011_CalibrateZeroRate(void)
{
	// 发送校准命令（第6-12节，地址0x0C）
	XV7011_WaitIF();
	XV7011_ChipSelect(1);
	XV7011_SPI_Transfer(0x0C); // AutoC命令
	XV7011_ChipSelect(0);
	
	// 等待校准完成
	osDelay(100); // 100ms
	
	return 0;
}

/**
 * @brief 获取设备状态
 * @param status 状态输出
 * @return 0=成功, 其他=错误
 */
uint8_t XV7011_GetStatus(uint8_t *status)
{
	return XV7011_ReadRegister(XV7011_REG_STSRD, status);
}

/**
 * @brief 微秒级延时（使用osDelay，最小1ms）
 * @param us 微秒数
 * @note FreeRTOS的osDelay最小单位为1ms，所以us < 1000时延时1ms
 */
void XV7011_DelayUs(uint32_t us)
{
	uint32_t ms = us / 1000;
	if (ms == 0) {
		ms = 1; // 最小延时1ms
	}
	osDelay(ms);
}
