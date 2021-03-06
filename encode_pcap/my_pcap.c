
#include <stdio.h>
#include "my_pcap.h"
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <rte_mbuf.h>
#define MAGIC 0xa1b2c3d4
//#define MAJOR 2
//#define MINOR 4
 
/*pktbuf是dpdk mbuf 的数据*/
void encode_pcap(struct rte_mbuf *pktbuf)
{
    
    printf("into endecode \n");
    u_short major =2;
    u_short minor =4;
    char *pbuf = (char*)pktbuf->pkt.data;
    uint16_t len =pktbuf->pkt.data_len;
    pcap_file_header pcap_file_hdr;
    timestamp pcap_timemp;
    pcap_header pcap_hdr;
 
    printf("[pcap_file_hdr]=%d\n",sizeof(pcap_file_hdr));
    printf("[pcap_hdr] = %d \n",sizeof(pcap_hdr));
    printf("[pcap_file_header]  = %d  \n",sizeof(pcap_file_header));
    printf("[pcap_header]  =  %d  \n",sizeof(pcap_header));
 
 
//初始化pcap头
    printf("----struct--\n");
    pcap_file_hdr.magic = 0xa1b2c3d4;
    printf("magic\n");
    pcap_file_hdr.major = major;
    pcap_file_hdr.minor = minor;
    pcap_file_hdr.thiszone = 0;
    pcap_file_hdr.sigfigs  = 0;
    printf("snaplen\n");
    pcap_file_hdr.snaplen  = 65535;
    printf("pcap_file_hdr\n");
    pcap_file_hdr.linktype =1;
    
 
    pcap_timemp.timestamp_s = 0;
    pcap_timemp.timestamp_ms= 0;
 
    pcap_hdr.capture_len = (uint32_t)pktbuf->pkt.pkt_len;
    pcap_hdr.len = (uint32_t)pktbuf->pkt.pkt_len;
 
    int ret =0;
 
    ret = access("/home/ly/encode.pcap",0);//只写入一个文件，首先判断文件是否存在
    if(ret == -1)
    {//不存在创建，名且写入pcap文件头，和pcap头，还有报文（tcp\ip 头和数据）
        FILE *fd;
        int ret =0;
	fd = fopen("/home/ly/encode.pcap","wb+");
	if (fd == NULL )
	{
	    printf("w+:can't open the file\n");
	    return;
	}
        ret = write_file_header(fd,&pcap_file_hdr);
	if(ret == -1 )
	{
	    printf("write file header error!\n");
	    return;
	}
	fseek(fd,0,SEEK_END);
	ret = write_header(fd,&pcap_hdr);
	if(ret == -1 )
	{
	    printf("write header error!\n");
	    return;
	}
	fseek(fd,0,SEEK_END);
	ret = write_pbuf(fd,pktbuf);
	if (ret == -1)
	{
	    printf("write pbuf error!\n");
	    return;
	}
        fclose(fd);
        return;
    }
    FILE *fd_pcap;//如果文件已存在，直接向文件末尾写入，pcap头和数据
    int ret_a =0;
    fd_pcap = fopen("/home/ly/encode.pcap","ab+");
    if (fd_pcap == NULL)
    {
	printf("a+:can't no open file !\n");
	return;
    }
    fseek(fd_pcap,0,SEEK_END);
    ret_a = write_header(fd_pcap,&pcap_hdr);
    if(ret_a == -1)
    {
	printf("write header error! \n");
	return;
    }
    fseek(fd_pcap,0,SEEK_END);
    ret_a =write_pbuf(fd_pcap,pktbuf);
    if (ret_a == -1)
    {
	printf("write pbuf error! \n");
	return;
    }
    fclose(fd_pcap);
    return;
      
}
 
int write_file_header(FILE *fd , pcap_file_header * pcap_file_hdr)
{
    int ret =0;
    if(fd <0 )
	return -1;
    ret = fwrite(pcap_file_hdr,sizeof(pcap_file_header),1,fd);
    if (ret != 1 )
	return -1;
    return 0;
}
 
int write_header(FILE *fd ,pcap_header * pcap_hdr)
{
    int ret =0;
    if(fd < 0 )
	return -1;
    ret = fwrite(pcap_hdr,sizeof(pcap_header),1,fd);
    if(ret != 1)
	return -1;
    return 0;
}
 
 
int write_pbuf(FILE *fd ,struct rte_mbuf *pktbuf)
{
    int ret =0;
    if (fd < 0 )
	return -1;
    ret = fwrite(pktbuf->pkt.data,pktbuf->pkt.data_len,1,fd);
    if (ret != 1)
	return -1;
    return 0;
}