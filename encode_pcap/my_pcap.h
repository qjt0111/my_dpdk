//  pcap.h  
//  pcaptest  
    //  
    //  Created by ly on 18-5-25.  
 
       
#ifndef pcaptest_pcap_h  
#define pcaptest_pcap_h 

//#include "comdef.h" 
       
typedef unsigned int  bpf_u_int32;  
typedef unsigned short  u_short;  
typedef int bpf_int32;  
       
    /* 
     Pcap�ļ�ͷ24B���ֶ�˵���� 
     Magic��4B��0x1A 2B 3C 4D:������ʾ�ļ��Ŀ�ʼ 
     Major��2B��0x02 00:��ǰ�ļ���Ҫ�İ汾��      
     Minor��2B��0x04 00��ǰ�ļ���Ҫ�İ汾�� 
     ThisZone��4B���صı�׼ʱ�䣻ȫ�� 
     SigFigs��4Bʱ����ľ��ȣ�ȫ�� 
     SnapLen��4B���Ĵ洢����     
     LinkType��4B��·���� 
     �������ͣ� 
     0            BSD loopback devices, except for later OpenBSD 
     1            Ethernet, and Linux loopback devices 
     6            802.5 Token Ring 
     7            ARCnet 
     8            SLIP 
     9            PPP 
     */  
typedef struct pcap_file_header {  
    bpf_u_int32 magic;  
    u_short major;  
    u_short minor;  
    bpf_int32 thiszone;      
    bpf_u_int32 sigfigs;     
    bpf_u_int32 snaplen;     
    bpf_u_int32 linktype;    
}pcap_file_header;  
       
    /* 
     Packet��ͷ��Packet������� 
     �ֶ�˵���� 
     Timestamp��ʱ�����λ����ȷ��seconds      
     Timestamp��ʱ�����λ����ȷ��microseconds 
     Caplen����ǰ�������ĳ��ȣ���ץȡ��������֡���ȣ��ɴ˿��Եõ���һ������֡��λ�á� 
     Len���������ݳ��ȣ�������ʵ������֡�ĳ��ȣ�һ�㲻����caplen����������º�Caplen��ֵ��ȡ� 
     Packet���ݣ��� Packet��ͨ��������·�������֡���������ݣ����Ⱦ���Caplen��������ȵĺ��棬���ǵ�ǰPCAP�ļ��д�ŵ���һ��Packet���ݰ���Ҳ�� ��˵��PCAP�ļ����沢û�й涨�����Packet���ݰ�֮����ʲô����ַ�������һ���������ļ��е���ʼλ�á�������Ҫ����һ��Packet��ȷ���� 
     */  
       
typedef struct  timestamp{  
    bpf_u_int32 timestamp_s;  
    bpf_u_int32 timestamp_ms;  
}timestamp;  
       
typedef struct pcap_header{  
    timestamp ts;  
    bpf_u_int32 capture_len;  
    bpf_u_int32 len;  
       
}pcap_header;  
       
       
 
void encode_pcap(struct rte_mbuf *pktbuf);
      
    #endif  
 