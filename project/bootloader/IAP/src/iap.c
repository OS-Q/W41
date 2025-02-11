/******************************************************************************
****版本：1.0.0
****平台：
****日期：2020-07-29
****作者：Qitas
****版权：
*******************************************************************************/

#include "iap_config.h"
#include "iap.h"
#include "stmflash.h"
#include "ymodem.h"

pFunction Jump_To_Application;
__IO uint32_t FlashProtection = 0;
uint32_t JumpAddress;

uint32_t BlockNbr = 0, UserMemoryMask = 0;
uint8_t tab_1024[1024] = {0};

/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
void iap_reset(void)
{
	NVIC_SystemReset();

}
/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
static void FLASH_DisableWriteProtectionPages(void)
{
	uint32_t useroptionbyte = 0, WRPR = 0;
	uint16_t var1 = OB_IWDG_SW, var2 = OB_STOP_NoRST, var3 = OB_STDBY_NoRST;
	FLASH_Status status = FLASH_BUSY;

	WRPR = FLASH_GetWriteProtectionOptionByte();

	if ((WRPR & UserMemoryMask) != UserMemoryMask)
	{
		useroptionbyte = FLASH_GetUserOptionByte();

		UserMemoryMask |= WRPR;

		status = FLASH_EraseOptionBytes();

		if (UserMemoryMask != 0xFFFFFFFF)
		{
			status = FLASH_EnableWriteProtection((uint32_t)~UserMemoryMask);
		}
		if ((useroptionbyte & 0x07) != 0x07)
		{
			if ((useroptionbyte & 0x01) == 0x0)
			{
				var1 = OB_IWDG_HW;
			}
			if ((useroptionbyte & 0x02) == 0x0)
			{
				var2 = OB_STOP_RST;
			}
			if ((useroptionbyte & 0x04) == 0x0)
			{
				var3 = OB_STDBY_RST;
			}
			FLASH_UserOptionByteConfig(var1, var2, var3);
		}

		if (status == FLASH_COMPLETE)
		{
			SerialPutString("Write Protection Close.\r\n");
			NVIC_SystemReset();
		}
		else
		{
			SerialPutString("Error: unprotection failed.\r\n");
		}
	}
	else
	{
		SerialPutString("Flash not write protected.\r\n");
	}
}


/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
void iap_writeflag(uint16_t flag)
{
#if (USE_BKP_SAVE_FLAG == 1)
	PWR->CR |= PWR_CR_DBP;
	BKP_WriteBackupRegister(IAP_FLAG_ADDR, flag);
	PWR->CR &= ~PWR_CR_DBP;
#else
	FLASH_Unlock();
	STMFLASH_Write(IAP_FLAG_ADDR, &flag, 1);
	FLASH_Lock();
#endif
}

/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/

uint16_t iap_readflag(void)
{
#if (USE_BKP_SAVE_FLAG == 1)
	return BKP_ReadBackupRegister(IAP_FLAG_ADDR);
#else
	return STMFLASH_ReadHalfWord(IAP_FLAG_ADDR);
#endif
}



/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
void IAP_USART_Init(void)
{
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    STM_EVAL_COMInit(&USART_InitStructure);
}


/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
void iap_init(void)
{
#if (USE_BKP_SAVE_FLAG == 1)
	RCC->APB1ENR |= (RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN );
#endif
	IAP_USART_Init();
}



/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
int8_t app_run(void)
{
	if (((*(__IO uint32_t*)ApplicationAddress) & 0x2FFE0000 ) == 0x20000000)
	{
		// SerialPutString("\r\n Run to app.\r\n");
		JumpAddress = *(__IO uint32_t*) (ApplicationAddress + 4);
		Jump_To_Application = (pFunction) JumpAddress;
		__set_MSP(*(__IO uint32_t*) ApplicationAddress);
		Jump_To_Application();
		return 0;
	}
	else
	{
		SerialPutString("\r\n Run app error.\r\n");
		return -1;
	}
}



/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
void iap_menu(void)
{
	uint8_t cmdStr[CMD_STRING_SIZE] = {0};
	BlockNbr = (FlashDestination - STM32_FLASH_BASE) >> 12;

#if defined (STM32F10X_MD) || defined (STM32F10X_MD_VL)
    UserMemoryMask = ((uint32_t)~((1 << BlockNbr) - 1));
#else
	if (BlockNbr < 62)
	{
		UserMemoryMask = ((uint32_t)~((1 << BlockNbr) - 1));
	}
	else
	{
		UserMemoryMask = ((uint32_t)STM32_FLASH_BASE);
	}
#endif

	if ((FLASH_GetWriteProtectionOptionByte() & UserMemoryMask) != UserMemoryMask)
	{
		FlashProtection = 1;
	}
	else
	{
		FlashProtection = 0;
	}
	while (1)
	{
		SerialPutString("\r\n\r\n IAP MEMU (V0.5.0)\r\n");
		SerialPutString(" update\r\n");
		SerialPutString(" upload\r\n");
		SerialPutString(" delete\r\n");
		SerialPutString(" config\r\n");
		SerialPutString(" reboot\r\n");
		if(FlashProtection != 0)	//There is write protected
		{
			SerialPutString(" diswp\r\n");
		}
		GetInputString(cmdStr);

		if(strcmp((char *)cmdStr, CMD_UPDATE_STR) == 0)
		{
			iap_writeflag(UPDATE_FLAG_DATA);
			return;
		}
		else if(strcmp((char *)cmdStr, CMD_UPLOAD_STR) == 0)
		{
			iap_writeflag(UPLOAD_FLAG_DATA);
			return;
		}
		else if(strcmp((char *)cmdStr, CMD_ERASE_STR) == 0)
		{
			iap_writeflag(ERASE_FLAG_DATA);
			return;
		}
		else if(strcmp((char *)cmdStr, CMD_MENU_STR) == 0)
		{
			iap_writeflag(INIT_FLAG_DATA);
		}
		else if(strcmp((char *)cmdStr, CMD_RUNAPP_STR) == 0)
		{
			iap_writeflag(APPRUN_FLAG_DATA);
			return;
		}
		else if(strcmp((char *)cmdStr, CMD_DISWP_STR) == 0)
		{
			FLASH_DisableWriteProtectionPages();
		}
		else
		{
			SerialPutString(" Invalid CMD !\r\n");
		}
	}
}


/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
int8_t iap_update(void)
{
	uint8_t Number[10] = "";
	int32_t Size = 0;
	Size = Ymodem_Receive(&tab_1024[0]);
	if (Size > 0)
	{
		SerialPutString("\r\n\r\nFirmware Update Success!\r\n");
		SerialPutString("\r\nFirmware Name: ");
		SerialPutString(file_name);
		Int2Str(Number, Size);
		SerialPutString("\r\nFirmware Size: ");
		SerialPutString(Number);
		SerialPutString(" Bytes.\r\n");
		return 0;
	}
	else if (Size == -1)
	{
		SerialPutString("\r\n Image size Too Big!\r\n");
		return -1;
	}
	else if (Size == -2)
	{
		SerialPutString("\r\n Update failed!\r\n");
		return -2;
	}
	else if (Size == -3)
	{
		SerialPutString("\r\n Aborted by user.\r\n");
		return -3;
	}
	else
	{
		SerialPutString(" Receive Filed.\r\n");
		return -4;
	}
}


/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
int8_t iap_upload(void)
{
	uint32_t status = 0;
	SerialPutString("\n\n\rSelect Receive File ... (press any key to abort)\n\r");
	if (GetKey() == CRC16)
	{
		status = Ymodem_Transmit((uint8_t*)ApplicationAddress, (const uint8_t*)"UploadedFlashImage.bin", FLASH_IMAGE_SIZE);
		if (status != 0)
		{
			SerialPutString("\n\rError Occured while Transmitting File\n\r");
			return -1;
		}
		else
		{
			SerialPutString("\n\rFile Trasmitted Successfully \n\r");
			return -2;
		}
	}
	else
	{
		SerialPutString("\r\n\nAborted by user.\n\r");
		return 0;
	}
}


/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
int8_t iap_erase(void)
{
	uint8_t erase_cont[3] = {0};
	Int2Str(erase_cont, FLASH_IMAGE_SIZE/PAGE_SIZE);
	SerialPutString(" @");
	SerialPutString(erase_cont);
	SerialPutString("@");
	if(EraseSomePages(FLASH_IMAGE_SIZE, 1)) return 0;
	else return -1;
}

/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
int8_t iap_task(void)
{
	switch(iap_readflag())
	{
		case APPRUN_FLAG_DATA:
			if(app_run()) iap_writeflag(INIT_FLAG_DATA);
			break;
		case INIT_FLAG_DATA:
			iap_menu();
			break;
		case UPDATE_FLAG_DATA:
			if(!iap_update()) iap_writeflag(APPRUN_FLAG_DATA);
			else iap_writeflag(INIT_FLAG_DATA);
			break;
		case UPLOAD_FLAG_DATA:
			if(!iap_upload()) iap_writeflag(APPRUN_FLAG_DATA);
			else iap_writeflag(INIT_FLAG_DATA);
			break;
		case ERASE_FLAG_DATA:
			iap_erase();
			iap_writeflag(INIT_FLAG_DATA);
			break;
		default:
			break;
	}
	return 0;
}

/*-------------------------(C) COPYRIGHT 2020 QITAS --------------------------*/


