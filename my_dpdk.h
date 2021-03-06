#include<pcap.h>

void dumpFile(pcap_dumper_t * dumper,const u_char *pkt, int len, time_t tv_sec, suseconds_t tv_usec)
{
    struct pcap_pkthdr hdr;
    hdr.ts.tv_sec = tv_sec;
    hdr.ts.tv_usec = tv_usec;
    hdr.caplen = len;
    hdr.len = len; 
    pcap_dump((u_char*)dumper, &hdr, pkt); 
}