/******************************************************************************
****版本：1.0.0
****平台：
****日期：2020-07-29
****作者：Qitas
****版权：
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "ymodem.h"
#include "iap_config.h"
#include "common.h"
#include "stm32f10x_flash.h"

uint8_t file_name[FILE_NAME_LENGTH];
uint32_t FlashDestination = ApplicationAddress; /* Flash user program offset */
uint16_t PageSize = PAGE_SIZE;
//uint32_t EraseCounter = 0x0;
//uint32_t NbrOfPage = 0;
//FLASH_Status FLASHStatus = FLASH_COMPLETE;
uint32_t RamSource;
extern uint8_t tab_1024[1024];

/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
static  int32_t Receive_Byte (uint8_t *c, uint32_t timeout)
{
  while (timeout-- > 0)
  {
    if (SerialKeyPressed(c) == 1)
    {
      return 0;
    }
  }
  return -1;
}

/******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
static uint32_t Send_Byte (uint8_t c)
{
  SerialPutChar(c);
  return 0;
}

/******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
static int32_t Receive_Packet (uint8_t *data, int32_t *length, uint32_t timeout)
{
  uint16_t i, packet_size;
  uint8_t c;
  *length = 0;
  if (Receive_Byte(&c, timeout) != 0)
  {
    return -1;
  }
  switch (c)
  {
	case STX_8B:
		packet_size = PACKET_8B_SIZE;
	  break;
	case STX_16B:
	  packet_size = PACKET_16B_SIZE;
	  break;
	case STX_32B:
	  packet_size = PACKET_32B_SIZE;
	  break;
	case STX_64B:
	  packet_size = PACKET_64B_SIZE;
	  break;
	case STX_128B:
	case SOH://为了兼容超级终端
      packet_size = PACKET_128B_SIZE;
      break;
	case STX_256B:
	  packet_size = PACKET_256B_SIZE;
	  break;
	case STX_512B:
	  packet_size = PACKET_512B_SIZE;
	  break;
	case STX_1KB:
	case STX://为了兼容超级终端
      packet_size = PACKET_1KB_SIZE;
      break;
	case STX_2KB:
		packet_size = PACKET_2KB_SIZE;
	  break;
    case EOT:
      return 0;
    case CA:
      if ((Receive_Byte(&c, timeout) == 0) && (c == CA))
      {
        *length = -1;
        return 0;
      }
      else
      {
        return -1;
      }
    case ABORT1:
    case ABORT2:
      return 1;
    default:
      return -1;
  }
  *data = c;
  for (i = 1; i < (packet_size + PACKET_OVERHEAD); i ++)
  {
    if (Receive_Byte(data + i, timeout) != 0)
    {
      return -1;
    }
  }
  if (data[PACKET_SEQNO_INDEX] != ((data[PACKET_SEQNO_COMP_INDEX] ^ 0xff) & 0xff))
  {
    return -1;
  }
  *length = packet_size;
  return 0;
}

/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
int32_t Ymodem_Receive(uint8_t *buf)
{
  uint8_t packet_data[PACKET_1KB_SIZE + PACKET_OVERHEAD], file_size[FILE_SIZE_LENGTH], *file_ptr, *buf_ptr;
  int32_t i, j, packet_length, session_done, file_done, packets_received, errors, session_begin, size = 0;
  /* Initialize FlashDestination variable */
  FlashDestination = ApplicationAddress;
  Send_Byte(CRC16);
  for (session_done = 0, errors = 0, session_begin = 0; ;)
  {
    for (packets_received = 0, file_done = 0, buf_ptr = buf; ;)
    {
      switch (Receive_Packet(packet_data, &packet_length, NAK_TIMEOUT))
      {
        case 0: //成功收到数据
          errors = 0;
          switch (packet_length)
          {
            /* Abort by sender */
            case - 1://接收失败
              Send_Byte(ACK);
              return 0;
            /* End of transmission */
            case 0://本次文件传送结束
              Send_Byte(ACK);
              file_done = 1;
              break;
            /* Normal packet */
            default://接收成功
              if ((packet_data[PACKET_SEQNO_INDEX] & 0xff) != (packets_received & 0xff))
              {
                Send_Byte(NAK);
              }
              else
              {
                if (packets_received == 0)//文件信息(首包)
                {
                  /* Filename packet */
                  if (packet_data[PACKET_HEADER] != 0)//文件名字
                  {
                    /* Filename packet has valid data */
                    for (i = 0, file_ptr = packet_data + PACKET_HEADER; (*file_ptr != 0) && (i < FILE_NAME_LENGTH);)
                    {
                      file_name[i++] = *file_ptr++;//保存文件名字
                    }
                    file_name[i++] = '\0';//字符串形式
                    for (i = 0, file_ptr ++; (*file_ptr != ' ') && (i < FILE_SIZE_LENGTH);)
                    {
                      file_size[i++] = *file_ptr++;//文件大小
                    }
                    file_size[i++] = '\0';
                    Str2Int(file_size, &size);

                    /* Test the size of the image to be sent */
                    /* Image size is greater than Flash size */
                    if (size > (FLASH_SIZE - 1))
                    {
                      /* End session */
                      Send_Byte(CA);
                      Send_Byte(CA);
                      return -1;
                    }

                    /* Erase the needed pages where the user application will be loaded */
                    /* Define the number of page to be erased */
					if(EraseSomePages(size, 0))
					{
					}
					else
					{
				      /* End session */
                      Send_Byte(CA);
                      Send_Byte(CA);
                      return -1;
						//Erase failed
					}
          Send_Byte(ACK);
          Send_Byte(CRC16);
          }
          /* Filename packet is empty, end session */
          else
          {
            Send_Byte(ACK);
            file_done = 1;
            session_done = 1;
            break;
          }
        }
        /* Data packet */
        else//文件信息保存完之后开始接收数据
        {
          memcpy(buf_ptr, packet_data + PACKET_HEADER, packet_length);
          RamSource = (uint32_t)buf;
          for (j = 0;(j < packet_length) && (FlashDestination <  ApplicationAddress + size);j += 4)
          {
            /* Program the data received into STM32F10x Flash */
            FLASH_Unlock();
            FLASH_ProgramWord(FlashDestination, *(uint32_t*)RamSource);
            FLASH_Lock();
            if (*(uint32_t*)FlashDestination != *(uint32_t*)RamSource)
            {
              /* End session */
              Send_Byte(CA);
              Send_Byte(CA);
              return -2;
            }
            FlashDestination += 4;
            RamSource += 4;
          }
          Send_Byte(ACK);
        }
        packets_received ++;
        session_begin = 1;
      }
          }
          break;
        case 1: //用户按下了'a'或'A'
          Send_Byte(CA);
          Send_Byte(CA);
          return -3;
        default:  //检查错误
          if (session_begin > 0)
          {
            errors ++;
          }
          if (errors > MAX_ERRORS)
          {
            Send_Byte(CA);
            Send_Byte(CA);
            return 0;
          }
          Send_Byte(CRC16);//发送校验值
          break;
      }
      if (file_done != 0)
      {
        break;
      }
    }
    if (session_done != 0)//文件发送完成
    {
      break;
    }
  }
  return (int32_t)size;
}

/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
int32_t Ymodem_CheckResponse(uint8_t c)
{
  return 0;
}

/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
void Ymodem_PrepareIntialPacket(uint8_t *data, const uint8_t* fileName, uint32_t *length)
{
  uint16_t i, j;
  uint8_t file_ptr[10];

  /* Make first three packet */
  data[0] = SOH;
  data[1] = 0x00;
  data[2] = 0xff;

  /* Filename packet has valid data */
  for (i = 0; (fileName[i] != '\0') && (i < FILE_NAME_LENGTH);i++)
  {
    data[i + PACKET_HEADER] = fileName[i];
  }

  data[i + PACKET_HEADER] = 0x00;

  Int2Str (file_ptr, *length);
  for (j =0, i = i + PACKET_HEADER + 1; file_ptr[j] != '\0' ; )
  {
    data[i++] = file_ptr[j++];
  }

  for (j = i; j < PACKET_128B_SIZE + PACKET_HEADER; j++)
  {
    data[j] = 0;
  }
}

/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
void Ymodem_PreparePacket(uint8_t *SourceBuf, uint8_t *data, uint8_t pktNo, uint32_t sizeBlk)
{
  uint16_t i, size, packetSize;
  uint8_t* file_ptr;

  /* Make first three packet */
  packetSize = sizeBlk >= PACKET_1KB_SIZE ? PACKET_1KB_SIZE : PACKET_128B_SIZE;
  size = sizeBlk < packetSize ? sizeBlk :packetSize;
  if (packetSize == PACKET_1KB_SIZE)
  {
     data[0] = STX;
  }
  else
  {
     data[0] = SOH;
  }
  data[1] = pktNo;
  data[2] = (~pktNo);
  file_ptr = SourceBuf;

  /* Filename packet has valid data */
  for (i = PACKET_HEADER; i < size + PACKET_HEADER;i++)
  {
     data[i] = *file_ptr++;
  }
  if ( size  <= packetSize)
  {
    for (i = size + PACKET_HEADER; i < packetSize + PACKET_HEADER; i++)
    {
      data[i] = 0x1A; /* EOF (0x1A) or 0x00 */
    }
  }
}

/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
uint16_t UpdateCRC16(uint16_t crcIn, uint8_t byte)
{
  uint32_t crc = crcIn;
  uint32_t in = byte|0x100;
  do
  {
  crc <<= 1;
  in <<= 1;
  if(in&0x100)
  ++crc;
  if(crc&0x10000)
  crc ^= 0x1021;
  }
  while(!(in&0x10000));
  return crc&0xffffu;
}

/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
uint16_t Cal_CRC16(const uint8_t* data, uint32_t size)
{
  uint32_t crc = 0;
  const uint8_t* dataEnd = data+size;
  while(data<dataEnd) crc = UpdateCRC16(crc,*data++);
  crc = UpdateCRC16(crc,0);
  crc = UpdateCRC16(crc,0);
  return crc&0xffffu;
}
/*******************************************************************************
**函数信息 ：
**功能描述 ：Cal Check sum for YModem Packet
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/

uint8_t CalChecksum(const uint8_t* data, uint32_t size)
{
  uint32_t sum = 0;
  const uint8_t* dataEnd = data+size;
  while(data < dataEnd) sum += *data++;
  return sum&0xffu;
}

/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
void Ymodem_SendPacket(uint8_t *data, uint16_t length)
{
  uint16_t i;
  i = 0;
  while (i < length)
  {
    Send_Byte(data[i]);
    i++;
  }
}

/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
uint8_t Ymodem_Transmit(uint8_t *buf, const uint8_t* sendFileName, uint32_t sizeFile)
{

  uint8_t packet_data[PACKET_1KB_SIZE + PACKET_OVERHEAD];
  uint8_t FileName[FILE_NAME_LENGTH];
  uint8_t *buf_ptr, tempCheckSum ;
  uint16_t tempCRC, blkNumber;
  uint8_t receivedC[2], CRC16_F = 0, i;
  uint32_t errors= 0, ackReceived= 0, size = 0, pktSize;

  for (i = 0; i < (FILE_NAME_LENGTH - 1); i++)
  {
    FileName[i] = sendFileName[i];
  }
  CRC16_F = 1;
  /* Prepare first block */
  Ymodem_PrepareIntialPacket(&packet_data[0], FileName, &sizeFile);
  do
  {
    /* Send Packet */
    Ymodem_SendPacket(packet_data, PACKET_128B_SIZE + PACKET_HEADER);
    /* Send CRC or Check Sum based on CRC16_F */
    if (CRC16_F)
    {
        tempCRC = Cal_CRC16(&packet_data[3], PACKET_128B_SIZE);
        Send_Byte(tempCRC >> 8);
        Send_Byte(tempCRC & 0xFF);
    }
    else
    {
        tempCheckSum = CalChecksum (&packet_data[3], PACKET_128B_SIZE);
        Send_Byte(tempCheckSum);
    }

    /* Wait for Ack and 'C' */
    if (Receive_Byte(&receivedC[0], 10000) == 0)
    {
      if (receivedC[0] == ACK)
      {
        /* Packet transfered correctly */
        ackReceived = 1;
      }
    }
    else
    {
        errors++;
    }
  }while (!ackReceived && (errors < 0x0A));

  if (errors >=  0x0A)
  {
    return errors;
  }
  buf_ptr = buf;
  size = sizeFile;
  blkNumber = 0x01;
  /* Here 1024 bytes package is used to send the packets */
  while (size)
  {
    /* Prepare next packet */
    Ymodem_PreparePacket(buf_ptr, &packet_data[0], blkNumber, size);
    ackReceived = 0;
    receivedC[0]= 0;
    errors = 0;
    do
    {
      /* Send next packet */
      if (size >= PACKET_1KB_SIZE)
      {
        pktSize = PACKET_1KB_SIZE;

      }
      else
      {
        pktSize = PACKET_128B_SIZE;
      }
      Ymodem_SendPacket(packet_data, pktSize + PACKET_HEADER);
      /* Send CRC or Check Sum based on CRC16_F */
      /* Send CRC or Check Sum based on CRC16_F */
      if (CRC16_F)
      {
          tempCRC = Cal_CRC16(&packet_data[3], pktSize);
          Send_Byte(tempCRC >> 8);
          Send_Byte(tempCRC & 0xFF);
      }
      else
      {
        tempCheckSum = CalChecksum (&packet_data[3], pktSize);
        Send_Byte(tempCheckSum);
      }
      /* Wait for Ack */
      if ((Receive_Byte(&receivedC[0], 100000) == 0)  && (receivedC[0] == ACK))
      {
        ackReceived = 1;
        if (size > pktSize)
        {
          buf_ptr += pktSize;
          size -= pktSize;
          if (blkNumber == (FLASH_IMAGE_SIZE/1024))
          {
            return 0xFF; /*  error */
          }
          else
          {
              blkNumber++;
          }
        }
        else
        {
          buf_ptr += pktSize;
          size = 0;
        }
      }
      else
      {
        errors++;
      }
    }while(!ackReceived && (errors < 0x0A));
    /* Resend packet if NAK  for a count of 10 else end of commuincation */

    if (errors >=  0x0A)
    {
      return errors;
    }

  }
  ackReceived = 0;
  receivedC[0] = 0x00;
  errors = 0;
  do
  {
    Send_Byte(EOT);
    /* Send (EOT); */
    /* Wait for Ack */
    if ((Receive_Byte(&receivedC[0], 10000) == 0)  && receivedC[0] == ACK)
    {
      ackReceived = 1;
    }
    else
    {
      errors++;
    }
  }while (!ackReceived && (errors < 0x0A));

  if (errors >=  0x0A)
  {
    return errors;
  }

  /* Last packet preparation */
  ackReceived = 0;
  receivedC[0] = 0x00;
  errors = 0;

  packet_data[0] = SOH;
  packet_data[1] = 0;
  packet_data [2] = 0xFF;

  for (i = PACKET_HEADER; i < (PACKET_128B_SIZE + PACKET_HEADER); i++)
  {
      packet_data [i] = 0x00;
  }

  do
  {
    /* Send Packet */
    Ymodem_SendPacket(packet_data, PACKET_128B_SIZE + PACKET_HEADER);
    /* Send CRC or Check Sum based on CRC16_F */
    tempCRC = Cal_CRC16(&packet_data[3], PACKET_128B_SIZE);
    Send_Byte(tempCRC >> 8);
    Send_Byte(tempCRC & 0xFF);

    /* Wait for Ack and 'C' */
    if (Receive_Byte(&receivedC[0], 10000) == 0)
    {
      if (receivedC[0] == ACK)
      {
        /* Packet transfered correctly */
        ackReceived = 1;
      }
    }
    else
    {
        errors++;
    }

  }while (!ackReceived && (errors < 0x0A));
  /* Resend packet if NAK  for a count of 10  else end of commuincation */
  if (errors >=  0x0A)
  {
    return errors;
  }

  do
  {
    Send_Byte(EOT);
    /* Send (EOT); */
    /* Wait for Ack */
    if ((Receive_Byte(&receivedC[0], 10000) == 0)  && receivedC[0] == ACK)
    {
      ackReceived = 1;
    }
    else
    {
      errors++;
    }
  }while(!ackReceived && (errors < 0x0A));

  if (errors >=  0x0A)
  {
    return errors;
  }
  return 0; /* file trasmitted successfully */
}

/*-------------------------(C) COPYRIGHT 2020 QITAS --------------------------*/
