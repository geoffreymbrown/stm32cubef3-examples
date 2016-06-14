#ifndef __CDC_VCOM_H_
#define __CDC_VCOM_H_

#define VCOM_RX_BUF_SZ 256

uint32_t vcom_read(uint8_t *, uint32_t);
uint32_t vcom_read_cnt(void);
uint32_t vcom_write(uint8_t *, uint32_t);
uint32_t vcom_connected(void);

#endif /* __CDC_VCOM_H_ */
