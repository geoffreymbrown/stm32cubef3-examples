/*------------------------------------------------------------------------/
/  Bitbanging MMCv3/SDv1/SDv2 (in SPI mode) control module
/-------------------------------------------------------------------------/
/
/  Copyright (C) 2012, ChaN, all right reserved.
/
/ * This software is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/--------------------------------------------------------------------------*/

#include "diskio.h"   
#include "stm32f3xx_hal.h"

extern SPI_HandleTypeDef hspi2;

/*--------------------------------------------------------------------------

   Module Private Functions

---------------------------------------------------------------------------*/

/* MMC/SD command (SPI mode) */
#define CMD0	(0)		/* GO_IDLE_STATE */
#define CMD1	(1)		/* SEND_OP_COND */
#define	ACMD41	(0x80+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(8)		/* SEND_IF_COND */
#define CMD9	(9)		/* SEND_CSD */
#define CMD10	(10)		/* SEND_CID */
#define CMD12	(12)		/* STOP_TRANSMISSION */
#define CMD13	(13)		/* SEND_STATUS */
#define ACMD13	(0x80+13)	/* SD_STATUS (SDC) */
#define CMD16	(16)		/* SET_BLOCKLEN */
#define CMD17	(17)		/* READ_SINGLE_BLOCK */
#define CMD18	(18)		/* READ_MULTIPLE_BLOCK */
#define CMD23	(23)		/* SET_BLOCK_COUNT */
#define	ACMD23	(0x80+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(24)		/* WRITE_BLOCK */
#define CMD25	(25)		/* WRITE_MULTIPLE_BLOCK */
#define CMD41	(41)		/* SEND_OP_COND (ACMD) */
#define CMD55	(55)		/* APP_CMD */
#define CMD58	(58)		/* READ_OCR */

/* Card type flags (CardType) */
#define CT_MMC		0x01	/* MMC ver 3 */
#define CT_SD1		0x02	/* SD ver 1 */
#define CT_SD2		0x04	/* SD ver 2 */
#define CT_SDC		(CT_SD1|CT_SD2) /* SD */
#define CT_BLOCK	0x08	/* Block addressing */


static DSTATUS Stat = STA_NOINIT;	/* Disk status */

static BYTE CardType;	/* b0:MMC, b1:SDv1, b2:SDv2, b3:Block addressing */




static int spiReadWrite(uint8_t *rbuf,
		 const uint8_t *tbuf, int cnt) {
  int i;
  volatile uint8_t  *DR = (uint8_t *) &(hspi2.Instance->DR);
  volatile uint32_t *SR = &(hspi2.Instance->SR);
  static volatile uint8_t trash;
 
  for (i = 0; i < cnt; i++){
    while((*SR & SPI_FLAG_TXE) != SPI_FLAG_TXE);
    if (tbuf) {
       *DR = *tbuf++;
    }
    else {
      *DR = 0xff;
    }
    while((*SR & SPI_FLAG_RXNE) != SPI_FLAG_RXNE);
    if (rbuf) {
      *rbuf++ = *DR;
    }
    else {
      trash = *DR;
    }
  }
  return i;
}

/*-----------------------------------------------------------------------*/
/* Transmit bytes to the card                                            */
/*-----------------------------------------------------------------------*/

static void xmit_mmc (const BYTE* buff, UINT bc) {
  // buff: pointer to data buffer to send 
  // bc: byte count of data in the tranmit buffer
  spiReadWrite(NULL, buff, bc);
}

/*-----------------------------------------------------------------------*/
/* Receive bytes from the card                                           */
/*-----------------------------------------------------------------------*/

static void rcvr_mmc (BYTE *buff, UINT bc) {
  // buff: pointer to the receive buffer
  // bc: byte count of data in the receive buffer
  spiReadWrite(buff, NULL, bc);
}

/*-----------------------------------------------------------------------*/
/* Wait for card ready                                                   */
/*-----------------------------------------------------------------------*/

// Need to create a 100uS Delay function

static int wait_ready (void) {
  BYTE d;
  UINT tmr;

  /* Wait for ready in timeout of 500ms */

  while(1) {
    //  for (tmr = 500; tmr; tmr--) {
    rcvr_mmc(&d, 1);
    if (d == 0xFF) break;
    //    HAL_Delay(1);
  }
  return  1; //tmr ? 1 : 0;
}

/*-----------------------------------------------------------------------*/
/* Deselect the card and release SPI bus                                 */
/*-----------------------------------------------------------------------*/

static void deselect (void) {
  BYTE d;
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, 1);  //   SD_CS_HIGH();
  rcvr_mmc(&d, 1);	/* Dummy clock (force DO hi-z for multiple slave SPI) */
}

/*-----------------------------------------------------------------------*/
/* Select the card and wait for ready                                    */
/*-----------------------------------------------------------------------*/

static int select (void) {
  BYTE d;

  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, 0);  //   SD_CS_LOW();

  rcvr_mmc(&d, 1);	        /* Dummy clock (force DO enabled) */
  
  if (wait_ready()) return 1;	/* OK */

  deselect();
  return 0;			/* Failed */
}

/*-----------------------------------------------------------------------*/
/* Receive a data packet from the card                                   */
/*-----------------------------------------------------------------------*/

static int rcvr_datablock (BYTE *buff, UINT btr) {
  BYTE d[2];
  UINT tmr;

  /* Wait for data packet in timeout of 100ms */
  while (1) { // for (tmr = 100; tmr; tmr--) {
    rcvr_mmc(d, 1);
    if (d[0] != 0xFF) break;
    //    HAL_Delay(1);
  }

  /* If not valid data token, return with error */
  
  if (d[0] != 0xFE) return 0;	
  
  rcvr_mmc(buff, btr);		/* Receive the data block into buffer */
  rcvr_mmc(d, 2);		/* Discard CRC */
  
  return 1;			/* Return with success */
}



/*-----------------------------------------------------------------------*/
/* Send a data packet to the card                                        */
/*-----------------------------------------------------------------------*/

static
int xmit_datablock (	/* 1:OK, 0:Failed */
    const BYTE *buff,	/* 512 byte data block to be transmitted */
    BYTE token		/* Data/Stop token */
			){
  BYTE d[2];
  
  if (!wait_ready()) return 0;
  
  d[0] = token;
  xmit_mmc(d, 1);				/* Xmit a token */
  if (token != 0xFD) {		/* Is it data token? */
    xmit_mmc(buff, 512);	/* Xmit the 512 byte data block to MMC */
    rcvr_mmc(d, 2);			/* Xmit dummy CRC (0xFF,0xFF) */
    rcvr_mmc(d, 1);			/* Receive data response */
    if ((d[0] & 0x1F) != 0x05)	/* If not accepted, return with error */
      return 0;
  }
  return 1;
}

/*-----------------------------------------------------------------------*/
/* Send a command packet to the card                                     */
/*-----------------------------------------------------------------------*/

static
BYTE send_cmd (		/* Returns command response (bit7==1:Send failed)*/
	       BYTE cmd,		/* Command byte */
	       DWORD arg		/* Argument */
			)
{
  BYTE n, d, buf[6];
  
  if (cmd & 0x80) {	/* ACMD<n> is the command sequense of CMD55-CMD<n> */
    cmd &= 0x7F;
    n = send_cmd(CMD55, 0);
    if (n > 1) return n;
  }
  
  /* Select the card and wait for ready */
  //  deselect();
  if (!select()) return 0xFF;
  
  /* Send a command packet */
  buf[0] = 0x40 | cmd;			/* Start + Command index */
  buf[1] = (BYTE)(arg >> 24);		/* Argument[31..24] */
  buf[2] = (BYTE)(arg >> 16);		/* Argument[23..16] */
  buf[3] = (BYTE)(arg >> 8);		/* Argument[15..8] */
  buf[4] = (BYTE)arg;			/* Argument[7..0] */
  n = 0x01;				/* Dummy CRC + Stop */
  if (cmd == CMD0) n = 0x95;		/* (valid CRC for CMD0(0)) */
  if (cmd == CMD8) n = 0x87;		/* (valid CRC for CMD8(0x1AA)) */
  buf[5] = n;
  xmit_mmc(buf, 6);
  
  /* Receive command response */
  if (cmd == CMD12) rcvr_mmc(&d, 1); /* Skip a stuff byte when stop reading */

  /* Wait for a valid response in timeout of 10 attempts */
  n = 10;
  do
    rcvr_mmc(&d, 1);
  while ((d & 0x80) && --n);
  
  return d;		/* Return with the response value */
}

/*--------------------------------------------------------------------------
  
  Public Functions
  
  ---------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS USER_status (
		     BYTE pdrv			/* Drive number (always 0) */
		     )
{
  DSTATUS s;
  BYTE d;
  
  
  if (pdrv) return STA_NOINIT;
  
  /* Check if the card is kept initialized */
  s = Stat;
  if (!(s & STA_NOINIT)) {
    if (send_cmd(CMD13, 0))	/* Read card status */
      s = STA_NOINIT;
    rcvr_mmc(&d, 1);		/* Receive following half of R2 */
    deselect();
  }
  Stat = s;
  
  return s;
}



/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS USER_initialize (BYTE pdrv) {
  BYTE n, ty, cmd, buf[4];
  UINT tmr;
  DSTATUS s;
  
  
  if (pdrv) return RES_NOTRDY;

  __HAL_SPI_ENABLE(&hspi2);

  uint16_t oldspeed = (hspi2.Instance->CR1&SPI_BAUDRATEPRESCALER_256);
  hspi2.Instance->CR1 = (hspi2.Instance->CR1&~SPI_BAUDRATEPRESCALER_256)|
    SPI_BAUDRATEPRESCALER_256;


  for (n = 10; n; n--) rcvr_mmc(buf, 1);	/* 80 dummy clocks */

  hspi2.Instance->CR1 = (hspi2.Instance->CR1&~SPI_BAUDRATEPRESCALER_256)
    | oldspeed;
  
  ty = 0;
  if (send_cmd(CMD0, 0) == 1) {			/* Enter Idle state */
    if (0&&(send_cmd(CMD8, 0x1AA) == 1)) {	/* SDv2? */
      rcvr_mmc(buf, 4);			/* Get trailing return value of R7 resp */
      /* The card can work at vdd range of 2.7-3.6V */
      if (buf[2] == 0x01 && buf[3] == 0xAA) {	
	/* Wait for leaving idle state (ACMD41 with HCS bit) */
	for (tmr = 1000; tmr; tmr--) {		
	  if (send_cmd(ACMD41, 1UL << 30) == 0) break;
	  HAL_Delay(1);
	}
	if (tmr && send_cmd(CMD58, 0) == 0) {	/* Check CCS bit in the OCR */
	  rcvr_mmc(buf, 4);
	  ty = (buf[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;	/* SDv2 */
	}
      }
    } else {				/* SDv1 or MMCv3 */
      if (send_cmd(ACMD41, 0) <= 1) 	{
	ty = CT_SD1; cmd = ACMD41;	/* SDv1 */
      } else {
	ty = CT_MMC; cmd = CMD1;	/* MMCv3 */
      }
      for (tmr = 1000; tmr; tmr--) {	/* Wait for leaving idle state */
	if (send_cmd(cmd, 0) == 0) break;
	HAL_Delay(1);
      }
      if (!tmr || send_cmd(CMD16, 512) != 0)	/* Set R/W block length to 512 */
	ty = 0;
    }
  }
  CardType = ty;
  s = ty ? 0 : STA_NOINIT;
  Stat = s;
  
  deselect();
  
  return s;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT USER_read (
		   BYTE pdrv,			/* Physical drive nmuber (0) */
		   BYTE *buff,			/* Pointer to the data buffer to store read data */
		   DWORD sector,		/* Start sector number (LBA) */
		   UINT count			/* Sector count (1..128) */
		   )
{
  if (disk_status(pdrv) & STA_NOINIT) return RES_NOTRDY;
  if (!count) return RES_PARERR;
  if (!(CardType & CT_BLOCK)) sector *= 512;	/* Convert LBA to byte address if needed */
  
  if (count == 1) {	/* Single block read */
    if ((send_cmd(CMD17, sector) == 0)	/* READ_SINGLE_BLOCK */
	&& rcvr_datablock(buff, 512))
      count = 0;
  }
  else {				/* Multiple block read */
    if (send_cmd(CMD18, sector) == 0) {	/* READ_MULTIPLE_BLOCK */
      do {
	if (!rcvr_datablock(buff, 512)) break;
	buff += 512;
      } while (--count);
      send_cmd(CMD12, 0);				/* STOP_TRANSMISSION */
    }
  }
  deselect();
  
  return count ? RES_ERROR : RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT USER_write (
		    BYTE pdrv,		/* Physical drive nmuber (0) */
		    const BYTE *buff,	/* Pointer to the data to be written */
		    DWORD sector,	/* Start sector number (LBA) */
		    UINT count		/* Sector count (1..128) */
		    )
{
  if (disk_status(pdrv) & STA_NOINIT) return RES_NOTRDY;
  if (!count) return RES_PARERR;
  if (!(CardType & CT_BLOCK)) sector *= 512;	/* Convert LBA to byte address if needed */
  
  if (count == 1) {	/* Single block write */
    if ((send_cmd(CMD24, sector) == 0)	/* WRITE_BLOCK */
	&& xmit_datablock(buff, 0xFE))
      count = 0;
  }
  else {				/* Multiple block write */
    if (CardType & CT_SDC) send_cmd(ACMD23, count);
    if (send_cmd(CMD25, sector) == 0) {	/* WRITE_MULTIPLE_BLOCK */
      do {
	if (!xmit_datablock(buff, 0xFC)) break;
	buff += 512;
      } while (--count);
      if (!xmit_datablock(0, 0xFD))	/* STOP_TRAN token */
	count = 1;
    }
  }
  deselect();
  
  return count ? RES_ERROR : RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT USER_ioctl (
		    BYTE pdrv,		/* Physical drive nmuber (0) */
		    BYTE cmd,		/* Control code */
		    void *buff		/* Buffer to send/receive control data */
		    )
{
  DRESULT res;
  BYTE n, csd[16];
  DWORD cs;
  
  
  if (disk_status(pdrv) & STA_NOINIT) return RES_NOTRDY;	/* Check if card is in the socket */
  
  res = RES_ERROR;
  switch (cmd) {
  case CTRL_SYNC :		/* Make sure that no pending write process */
    if (select()) {
      deselect();
      res = RES_OK;
    }
    break;
    
  case GET_SECTOR_COUNT :	/* Get number of sectors on the disk (DWORD) */
    if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {
      if ((csd[0] >> 6) == 1) {	/* SDC ver 2.00 */
	cs = csd[9] + ((WORD)csd[8] << 8) + ((DWORD)(csd[7] & 63) << 8) + 1;
	*(DWORD*)buff = cs << 10;
      } else {					/* SDC ver 1.XX or MMC */
	n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
	cs = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
	*(DWORD*)buff = cs << (n - 9);
      }
      res = RES_OK;
    }
    break;
    
  case GET_BLOCK_SIZE :	/* Get erase block size in unit of sector (DWORD) */
    *(DWORD*)buff = 128;
    res = RES_OK;
    break;
    
  default:
    res = RES_PARERR;
  }
  
  deselect();
  
  return res;
}


