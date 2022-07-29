/*
 ============================================================================
 Name        : modbus_rtu_slave.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
//modbus
//作者：小猪
//时间：20220729
//功能：实现modbus基本主从功能
#include "modbus_rtu.h"

#include <stdio.h>
#include <stdlib.h>

static void modbus_systick(struct modbus_n *This)
{
	if(This->timout>0)
	{
		This->timout--;
	}
}
static long modbus_rx(struct modbus_n *This,unsigned char *dat,long length)
{
	long i=0;
	for(i=0;i<length;i++)
	{
	This->rx_buff[This->rx_buff_length++]=dat[i];
	}
	This->timout=This->set_timout;
	return 0;
}
static unsigned short crc16( unsigned char *CRC_Buf, unsigned short CRC_Leni )
{
	    unsigned long i,j;
	    unsigned short CRC_Sumx;
	    CRC_Sumx=0xFFFF;
	    for(i=0;i<CRC_Leni;i++)
	    {
	        CRC_Sumx^=*(CRC_Buf+i);//
	        for(j=0;j<8;j++)
	        {
	            if(CRC_Sumx & 0x01)
	            {
	                CRC_Sumx>>=1;
	                CRC_Sumx^=0xA001;
	            }
	            else
	            {
	                CRC_Sumx>>=1;
	            }
	        }
	    }
	    return (CRC_Sumx);
}

static void Modbud_Slave_fun3(modbus_t *This)
{
	unsigned short Regadd;  //寄存器起始地址
	unsigned short Reglen;  //寄存器个数
	unsigned short byte;
	unsigned short i,j;
	unsigned short crc;
	Regadd=This->rx_buff[2]*256+This->rx_buff[3];  //得到要读取的寄存器的首地址
	Reglen=This->rx_buff[4]*256+This->rx_buff[5];  //得到要读取的寄存器的数量
	i=0;
	This->tx_buff[i++]=This->addr;  //本设备地址
    This->tx_buff[i++]=0x03;  //功能码
    byte=Reglen*2;   //要返回的数据字节数
	This->tx_buff[i++]=byte%256;
	for(j=0;j<Reglen;j++)
	{
	  This->tx_buff[i++]=This->reg[Regadd+j]/256;
	  This->tx_buff[i++]=This->reg[Regadd+j]%256;
	}
	crc=crc16(This->tx_buff,i);
	This->tx_buff[i++]=crc/256;  //
	This->tx_buff[i++]=crc%256;
	if(This->send!=NULL)
	{
	This->send(This->tx_buff,i);
	}
}

static void Modbud_Master_fun3(modbus_t *This)
{
	unsigned short Regadd;  //寄存器起始地址
	unsigned short Reglen;  //寄存器个数
	unsigned short j,i;
	Regadd=This->tx_buff[2]*256+This->tx_buff[3];  //得到要读取的寄存器的首地址
	Reglen=This->rx_buff[2];  //得到要读取的寄存器的数量
	i=0;
	for(j=0;j<Reglen/2;j++)
	{
		This->reg[Regadd+j]= This->rx_buff[3+i];
		i++;
		This->reg[Regadd+j]= (This->reg[Regadd+j]<<8)|This->rx_buff[3+i];
		i++;


	}

}

static void Modbud_Slave_fun6(modbus_t *This)  //6号功能码处理
{
	unsigned short Regadd;
	unsigned short val;
	unsigned short i,crc;
	i=0;
    Regadd=This->rx_buff[2]*256+This->rx_buff[3];  //得到要修改的地址
	val=This->rx_buff[4]*256+This->rx_buff[5];     //修改后的值
	This->reg[Regadd]=val;  //修改本设备相应的寄存器
	//以下为回应主机
	This->tx_buff[i++]=This->addr;//本设备地址
    This->tx_buff[i++]=0x06;        //功能码
    This->tx_buff[i++]=Regadd/256;
	This->tx_buff[i++]=Regadd%256;
	This->tx_buff[i++]=val/256;
	This->tx_buff[i++]=val%256;
	crc=crc16(This->tx_buff,i);
	This->tx_buff[i++]=crc/256;  //
	This->tx_buff[i++]=crc%256;

	if(This->send!=NULL)
	{
	This->send(This->tx_buff,i);
	}

}
static void Modbud_Master_fun6(modbus_t *This)  //6号功能码处理
{
	unsigned short Regadd;
	unsigned short val;
    Regadd=This->rx_buff[2]*256+This->rx_buff[3];  //得到要修改的地址
	val=This->rx_buff[4]*256+This->rx_buff[5];     //修改后的值
	This->reg[Regadd]=val;  //修改本设备相应的寄存器

}
static void Modbud_Slave_fun10(modbus_t *This)  //6号功能码处理
{
	unsigned short Regadd;
	unsigned short val;
	unsigned short i,j,crc;
	i=0;
    Regadd=This->rx_buff[2]*256+This->rx_buff[3];  //得到要修改的地址
	val=This->rx_buff[4]*256+This->rx_buff[5];     //要操作多少个寄存器
	j=0;
	if(val*2==This->rx_buff[6])
	{
		for(i=0;i<val;i++)
		{
			This->reg[Regadd+i]=This->rx_buff[7+j];  //修改本设备相应的寄存器
			j++;
			This->reg[Regadd+i]=(This->reg[Regadd+i]<<8)|This->rx_buff[7+j];
			j++;
		}
	}

	i=0;
	//以下为回应主机
	This->tx_buff[i++]=This->addr;//本设备地址
    This->tx_buff[i++]=0x10;        //功能码
    This->tx_buff[i++]=Regadd/256;
	This->tx_buff[i++]=Regadd%256;
	This->tx_buff[i++]=val/256;
	This->tx_buff[i++]=val%256;
	crc=crc16(This->tx_buff,i);
	This->tx_buff[i++]=crc/256;  //
	This->tx_buff[i++]=crc%256;

	if(This->send!=NULL)
	{
	This->send(This->tx_buff,i);
	}

}

static void Modbud_Master_fun10(modbus_t *This)
{
     if(memcmp(This->tx_buff,This->rx_buff,6)==0)
     {
    	 This->madbus_tx_flag =1;
     }

}
static long modbus_master_tx(struct modbus_n *This,unsigned char cmd,unsigned short reg_addr,unsigned short reg_num,unsigned short *dat,long length)
{
	unsigned short i=0,crc,j=0;
	This->tx_buff[i++]=This->addr;
	This->tx_buff[i++]=cmd;
	This->tx_buff[i++]=reg_addr/256;
	This->tx_buff[i++]=reg_addr%256;
	switch(cmd)
	{
		case 0x03:
			This->tx_buff[i++]=reg_num/256;
			This->tx_buff[i++]=reg_num%256;
			break;
		case 0x06:
			This->tx_buff[i++]=dat[0]/256;
			This->tx_buff[i++]=dat[0]%256;
			break;
		case 0x10:
			This->tx_buff[i++]=reg_num/256;
			This->tx_buff[i++]=reg_num%256;
			This->tx_buff[i++]=length;
			for(j=0;j<reg_num;j++)
			{
				This->tx_buff[i++]=dat[j]/256;
				This->tx_buff[i++]=dat[j]%256;
			}
			break;
	}

	crc=crc16(This->tx_buff,i);
	This->tx_buff[i++]=crc/256;  //
	This->tx_buff[i++]=crc%256;
	if(This->send!=NULL)
	{
		This->madbus_tx_flag =0;
		This->send(This->tx_buff,i);
	}
    return 0;
}
static void modbus_slave_loop(modbus_t *This)
{
  unsigned short crc;
  unsigned short rccrc;
  if(This->timout>0&&This->rx_buff_length>2)  //没有收到Modbus的数据
  {
	  return ;
  }
  crc= crc16(&This->rx_buff[0], This->rx_buff_length-2);   //计算校验码，-2去除两位校验码
  rccrc=This->rx_buff[This->rx_buff_length-2]*256 + This->rx_buff[This->rx_buff_length-1];  //收到的校验码

  if(crc == rccrc)  //数据包符号CRC校验规则
  {
	  if(This->rx_buff[0] == This->addr)  //确认数据包是否是发给本设备的 确认接收的地址是本机地址
		{
		  switch(This->rx_buff[1])  //分析功能码
			{
			  case 0:     break;
			  case 1:     break;
		      case 2:     break;
		      case 3:     Modbud_Slave_fun3(This); break;   //3号功能码处理
		      case 4:     break;
		      case 5:     break;
		      case 6:     Modbud_Slave_fun6(This); break;   //6号功能码处理
	          case 7:     break;
	          case 0x10:  Modbud_Slave_fun10(This);break;
        //....
			}
		}
		else if(This->rx_buff[0] == 0)   //如果是广播地址则不处理
		{
		}
	}

    This->rx_buff_length=0;

}

static void modbus_master_loop(modbus_t *This)
{
  unsigned short crc;
  unsigned short rccrc;
  if(This->timout>0&&This->rx_buff_length>2)  //没有收到Modbus的数据
  {
	  return ;
  }

  crc= crc16(&This->rx_buff[0], This->rx_buff_length-2);   //计算校验码，-2去除两位校验码
  rccrc=This->rx_buff[This->rx_buff_length-2]*256 + This->rx_buff[This->rx_buff_length-1];  //收到的校验码
  if(crc == rccrc)  //数据包符号CRC校验规则
	{

	  if(This->rx_buff[0] == This->addr)  //确认数据包是否是发给本设备的 确认接收的地址是本机地址
		{
		  switch(This->rx_buff[1])  //分析功能码
			{
			  case 0:     break;
			  case 1:     break;
		      case 2:     break;
		      case 3:     Modbud_Master_fun3(This);  break;   //3号功能码处理
		      case 4:     break;
		      case 5:     break;
		      case 6:     Modbud_Master_fun6(This); break;   //6号功能码处理
	          case 7:     break;
	          case 0x10:  Modbud_Master_fun10(This);break;
        //....
			}
		}
		else if(This->rx_buff[0] == 0)   //如果是广播地址则不处理
		{
		}
	}
  This->rx_buff_length=0;
}
//demo 测试
#define demo_debug  1
#if demo_debug
unsigned short SlaveReg[]={0x0000,   //本设备寄存器中的值
           0x0001,
           0x0002,
           0x0003,
           0x0004,
           0x0005,
           0x0006,
           0x0007,
           0x0008,
           0x0009,
           0x000A,
          };
unsigned short MasterReg[]={0x0000,   //本设备寄存器中的值
           0x0001,
           0x0002,
           0x0003,
           0x0004,
           0x0005,
           0x0006,
           0x0007,
           0x0008,
           0x0009,
           0x000A,
          };
modbus_t modbus_master[1];
modbus_t modbus_slave[1];
long modbus_master_send(unsigned char *dat,long length)
{
	printf("\r\n%s:",__func__);
	modbus_slave->modbus_rx(modbus_slave,dat,length);
	for(int i=0;i<length;i++)
	{
	printf("%02x",dat[i]);
	}
	return 0;
}
long modbus_slave_send(unsigned char *dat,long length)
{
	printf("\r\n%s:",__func__);
	modbus_master->modbus_rx(modbus_master,dat,length);
	for(int i=0;i<length;i++)
	{
	printf("%02x",dat[i]);
	}
	return 0;
}
/******************************************************************
功能: Modbus函数初始化
******************************************************************/
modbus_t *CreateModbusObj(modbus_t *This)
{
	This->modbus_rx=modbus_rx;
	This->modbus_systick=modbus_systick;
	This->modbus_master_tx=modbus_master_tx;
	This->modbus_slave_loop=modbus_slave_loop;
	This->modbus_master_loop=modbus_master_loop;
	return This;
}
int main(void) {
	puts("!!!Hello World!!!"); /* prints !!!Hello World!!! */
    unsigned short dat[]={0x0101,0x0202,0x0303};
    CreateModbusObj(modbus_master);
    CreateModbusObj(modbus_slave);
    modbus_master->reg=MasterReg;   //寄存器
    modbus_master->addr=1;			//设备地址
	modbus_master->send=modbus_master_send;


	modbus_slave->reg=SlaveReg;
	modbus_slave->addr=1;
	modbus_slave->send=modbus_slave_send;

	//读
	modbus_master->modbus_master_tx(modbus_master,0x03,0001,0003,dat,6);
	modbus_slave->modbus_slave_loop(modbus_slave);
	printf("\r\nSlaveReg:");
	for(int i=0;i<sizeof(SlaveReg)/2;i++)
	{
	printf("%04x ",SlaveReg[i]);
	}
	modbus_master->modbus_master_loop(modbus_master);
	printf("\r\nMasterReg");
	for(int i=0;i<sizeof(MasterReg)/2;i++)
	{
	printf("%04x ",MasterReg[i]);
	}

	//写多个寄存器
	modbus_master->modbus_master_tx(modbus_master,0x10,0001,0003,dat,6);
	modbus_slave->modbus_slave_loop(modbus_slave);
	printf("\r\nSlaveReg:");
	for(int i=0;i<sizeof(SlaveReg)/2;i++)
	{
		printf("%04x ",SlaveReg[i]);
	}
	modbus_master->modbus_master_loop(modbus_master);
	printf("\r\nMasterReg");
	for(int i=0;i<sizeof(MasterReg)/2;i++)
	{
		printf("%04x ",MasterReg[i]);
	}
    //读
	modbus_master->modbus_master_tx(modbus_master,0x03,0001,0003,dat,6);
	modbus_slave->modbus_slave_loop(modbus_slave);
	printf("\r\nSlaveReg:");
	for(int i=0;i<sizeof(SlaveReg)/2;i++)
	{
		printf("%04x ",SlaveReg[i]);
	}
	modbus_master->modbus_master_loop(modbus_master);
	printf("\r\nMasterReg:");
	for(int i=0;i<sizeof(MasterReg)/2;i++)
	{
		printf("%04x ",MasterReg[i]);
	}

	//写单个寄存器
	dat[0]=0xFF;
	modbus_master->modbus_master_tx(modbus_master,0x06,0004,0003,dat,6);
	modbus_slave->modbus_slave_loop(modbus_slave);
	printf("\r\nSlaveReg:");
	for(int i=0;i<sizeof(SlaveReg)/2;i++)
	{
		printf("%04x ",SlaveReg[i]);
	}
	modbus_master->modbus_master_loop(modbus_master);
	printf("\r\nMasterReg");
	for(int i=0;i<sizeof(MasterReg)/2;i++)
	{
		printf("%04x ",MasterReg[i]);
	}

	modbus_master->modbus_master_tx(modbus_master,0x03,0001,0003,dat,6);
	modbus_slave->modbus_slave_loop(modbus_slave);
	printf("\r\nSlaveReg:");
	for(int i=0;i<sizeof(SlaveReg)/2;i++)
	{
		printf("%04x ",SlaveReg[i]);
	}
	modbus_master->modbus_master_loop(modbus_master);
	printf("\r\nMasterReg:");
	for(int i=0;i<sizeof(MasterReg)/2;i++)
	{
		printf("%04x ",MasterReg[i]);
	}
	return EXIT_SUCCESS;
}
#endif
