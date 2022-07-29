/*
 * modbus_rtu_slave.h
 *
 *  Created on: 2022年7月28日
 *      Author: user
 */

#ifndef MODBUS_RTU_H_
#define MODBUS_RTU_H_
#include "string.h"
//作者：小猪
//时间：20220729
//功能：实现modbus基本主从功能
typedef struct modbus_n
{
    unsigned char addr;          //本设备的地址
    unsigned char rx_buff[64];    //Modbus接收缓冲区64个字节
    unsigned char rx_buff_length; //Modbus端口已经收到的数据个数
    unsigned char tx_buff[64];    //Modbus发送缓冲区
    unsigned char madbus_tx_flag; //Modbus Master 发送状态  1 成功 0 还没返回
    unsigned short *reg;          //Modbus寄存器地址
    long timout;			      //Modbus的数据断续时间
    long set_timout;			  //设置Modbus的数据断续时间
    long (*modbus_rx)(struct modbus_n *This,unsigned char *dat,long length);
    void (*modbus_slave_loop)(struct modbus_n *This);
    void (*modbus_master_loop)(struct modbus_n *This);
    long (*modbus_master_tx)(struct modbus_n *This,unsigned char cmd,unsigned short reg_addr,unsigned short reg_num,unsigned short *dat,long length);
    void (*modbus_systick)(struct modbus_n *This);
    long (*send)(unsigned char *dat,long length);
}modbus_t;
modbus_t *CreateModbusObj(modbus_t *This);
#endif /* MODBUS_RTU_H_ */
