# 公园盒子嵌入式系统

此系统基于FreeRTOS。

## 目录结构说明：
* common --- 基础类型定义
* hardware/board --- MCU BSP
* hal --- 对不同硬件外设驱动的封装
* middleware --- 对RTOS接口的封装
* modules/drivers --- 设备驱动
* modules/crypto --- 加解密库
* project --- ide工程文件
* thirdparty --- 第三方开源库

## 配置说明:
* board_config.h 板级引脚定义，功能开关
* os_config.h 封装系统功能开关