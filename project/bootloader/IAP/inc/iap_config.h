/******************************************************************************
****版本：1.0.0
****平台：
****日期：2020-07-29
****作者：Qitas
****版权：
*******************************************************************************/
#ifndef __IAP_CONFIG_H__
#define __IAP_CONFIG_H__

#define IAP_VERSION           (100)

#define USE_BKP_SAVE_FLAG     (0)
#define ENABLE_PUTSTR         (1)

/* Bootloader command -----------------------------------------*/
#define CMD_UPDATE_STR        "update"
#define CMD_UPLOAD_STR        "upload"
#define CMD_ERASE_STR		      "erase"
#define CMD_MENU_STR          "menu"
#define CMD_RUNAPP_STR        "runapp"
#define CMD_ERROR_STR         "error"
#define CMD_DISWP_STR         "diswp"   //禁止写保护

/* The maximum length of the command string -------------------*/
#define CMD_STRING_SIZE       (128)

#if (USE_BKP_SAVE_FLAG == 1)
  #define INIT_FLAG_DATA      0x0000    //默认标志的数据(空片子的情况)
#else
  #define INIT_FLAG_DATA      0xFFFF   //默认标志的数据(空片子的情况)
#endif

/* Define the Flsah area size ---------------------------------*/
#if defined (STM32F10X_MD) || defined (STM32F10X_MD_VL)
  #define PAGE_SIZE                         (0x400)    /* 1 Kbyte */
  #define FLASH_SIZE                        (0x20000)  /* 128 KBytes */
#elif defined STM32F10X_CL
  #define PAGE_SIZE                         (0x800)    /* 2 Kbytes */
  #define FLASH_SIZE                        (0x40000)  /* 256 KBytes */
#elif defined STM32F10X_HD || defined (STM32F10X_HD_VL)
  #define PAGE_SIZE                         (0x800)    /* 2 Kbytes */
  #define FLASH_SIZE                        (0x80000)  /* 512 KBytes */
#elif defined STM32F10X_XL
  #define PAGE_SIZE                         (0x800)    /* 2 Kbytes */
  #define FLASH_SIZE                        (0x100000) /* 1 MByte */
#else
  #error "Please select first the STM32 device to be used (in stm32f10x.h)"
#endif

/* Define the APP start address -------------------------------*/
#define ApplicationAddress      (0x8003000)
//#define IAP_FLASH_FLAG_ADDR   0x8002800
#if (USE_BKP_SAVE_FLAG == 1)
#define IAP_FLAG_ADDR         BKP_DR1
#else
#define IAP_FLAG_ADDR         (uint32_t)(ApplicationAddress - 1024 * 2) //App区域和Bootloader区域共享信息的地址(暂定大小为2K)
#endif

#define FLASH_IMAGE_SIZE      (uint32_t) (FLASH_SIZE - (ApplicationAddress - 0x08000000))



#endif

/*-------------------------(C) COPYRIGHT 2020 QITAS --------------------------*/
