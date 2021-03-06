
#include<stdio.h>
#include<string.h>
#include<memory.h>


/**
* @brief  数据反转(高低位调换的意思，将数据最高位调到最低位,次高位调到次低位)
* @note
* @param  data:数据
*		  bitLen:数据位数
* @retval Reverse Result
*
*/
unsigned int Reverse(unsigned int data, unsigned char bitLen)
{
	unsigned int i;
	unsigned int buf=0;
	for (i = 0; i<bitLen; i++) {
		buf <<= 1;
		if (data & 0x01) {
			buf |= 1;
		}
		data >>= 1;
	}

	return buf;
}

/**
* @brief  CRC8 Calculate
* @note   
* @param  crcInit:		CRC初始值
*		  poly:			CRC多项式
*		  BlockStart:	数据段
*		  BlockLength:	数据长度(需要计算的长度)
*		  inRev:		输入数据是否反转 0:不反转 1:反转
*		  outRev:		输出数据是否反转 0:不反转 1:反转
* @retval CRC8 Calculate Result
*         
*/
unsigned char CRC8(unsigned char crcInit, unsigned char poly, unsigned char *BlockStart, unsigned char BlockLength, unsigned char inRev, unsigned char outRev)
{
	unsigned int i, j;
	unsigned char CRC_VAL = crcInit;
	unsigned char buf;

	for (j = 0; j<BlockLength; j++) {
		buf = *(BlockStart + j);

		if (inRev != 0) {
			buf = Reverse(buf,8);
		}

		CRC_VAL ^= buf;
		for (i = 0; i<8; i++) {
			if (CRC_VAL & 0x80) {
				CRC_VAL = (CRC_VAL << 1) ^ poly;
			}
			else {
				CRC_VAL <<= 1;
			}
		}
	}


	if (outRev != 0) {
		CRC_VAL = Reverse(CRC_VAL,8);
	}

	return 	CRC_VAL & 0xFF;

}

/**
* @brief  CRC16 Calculate
* @note
* @param  crcInit:		CRC初始值
*		  poly:			CRC多项式
*		  BlockStart:	数据段
*		  BlockLength:	数据长度(需要计算的长度)
*		  inRev:		输入数据是否反转 0:不反转 1:反转
*		  outRev:		输出数据是否反转 0:不反转 1:反转
* @retval CRC16 Calculate Result
*
*/
unsigned short CRC16(unsigned short crcInit, unsigned short poly, unsigned char *BlockStart, unsigned char BlockLength, unsigned char inRev, unsigned char outRev)
{
	unsigned int i, j;
	unsigned short CRC_VAL = crcInit;
	unsigned char buf;

	for (j = 0; j<BlockLength; j++) {
		buf = *(BlockStart + j);

		if (inRev != 0) {
			buf = Reverse(buf,8);
		}

		CRC_VAL ^= ((unsigned short)buf << 8);
		for (i = 0; i<8; i++) {
			if (CRC_VAL & 0x8000) {
				CRC_VAL = (CRC_VAL << 1) ^ poly;
			}
			else {
				CRC_VAL <<= 1;
			}
		}
	}

	if (outRev != 0) {
		CRC_VAL = Reverse(CRC_VAL,16);
	}

	return 	CRC_VAL & 0xFFFF;

}



/**
* @brief  CRC32 Calculate
* @note
* @param  crcInit:		CRC初始值
*		  poly:			CRC多项式
*		  BlockStart:	数据段
*		  BlockLength:	数据长度(需要计算的长度)
*		  inRev:		输入数据是否反转 0:不反转 1:反转
*		  outRev:		输出数据是否反转 0:不反转 1:反转
* @retval CRC32 Calculate Result
*
*/
unsigned int CRC32(unsigned int crcInit, unsigned int poly, unsigned char *BlockStart, unsigned char BlockLength, unsigned char inRev, unsigned char outRev)
{
	unsigned int i, j;
	unsigned int CRC_VAL = crcInit;
	unsigned char buf;

	for (j = 0; j<BlockLength; j++) {
		buf = *(BlockStart + j);

		if (inRev != 0) {
			buf = Reverse(buf, 8);
		}

		CRC_VAL ^= ((unsigned int)buf << 24);
		for (i = 0; i<8; i++) {
			if (CRC_VAL & 0x80000000) {
				CRC_VAL = (CRC_VAL << 1) ^ poly;
			}
			else {
				CRC_VAL <<= 1;
			}
		}
	}

	if (outRev != 0) {
		CRC_VAL = Reverse(CRC_VAL, 32);
	}

	return 	CRC_VAL & 0xFFFFFFFF;

}

// struct  five_t
// {
//     /* data */
//     unsigned char src_ip[4];
//     unsigned char dst_ip[4];
//     unsigned int src_port;
//     unsigned int dst_port;
//     unsigned int proto;
// };


// int main(void)
// {
// 	unsigned char test[] = { 0x01,0x03,0x00,0x00,0x00,0x01 };
// 	struct five_t test2={
//         .dst_ip={1,1,1,1},
//         .src_ip={2,2,2,2},
//         .src_port = 80,
//         .dst_port = 88,
//         .proto =6
//     } ;
//     printf("%u\n",sizeof(struct five_t));
//     unsigned char tt[20];
//     memcpy(tt,&test2,sizeof(test2));
//     unsigned char crc8_result=0;
// 	unsigned short crc16_result=0;
// 	unsigned int  crc32_result = 0;

// 	//CRC8
// 	crc8_result = CRC8(0x00, 0x07, tt, sizeof(tt), 0, 0);
// 	//CRC16 MODBUS
// 	crc16_result = CRC16(0xFFFF, 0x8005, test, sizeof(test), 1, 1);
// 	//CRC-32/MPEG-2
// 	crc32_result = CRC32(0xFFFFFFFF,0x04C11DB7, test, sizeof(test), 0, 0);

// 	printf("CRC8 result  = 0x%02X\r\n", crc8_result);
// 	printf("CRC16 result = 0x%04X\r\n", crc16_result);
// 	printf("CRC32 result = 0x%08X\r\n", crc32_result);

// 	//while (1);

// 	return 0;
// }
