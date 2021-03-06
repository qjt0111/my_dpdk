
#include<iostream>
#include<vector>
#include<map>
#include<string>
#include<list>
#include"dpdk_map.h"
#include<pcap.h>
#include"my_dpdk.h"

using namespace std;

map<unsigned int,vector<struct five_tuple_tmp>> hash_map;

// #ifdef __cplusplus

// extern "C" {

// #endif

int hashmap_size(unsigned int key)
{
    return hash_map[key].size();//返回该hash_map中vector中数据包的数量
}

void hashmap_insert(unsigned int key,five_tuple_tmp val)
{
    hash_map[key].push_back(val);//向hash_map中插入有限的数据报文
}
void hashmap_clear(unsigned int key)
{
    hash_map[key].clear();//清空该hash_map中缓存的数据报文
}
struct five_tuple_tmp hashmap_val(unsigned int key,int i)
{
    //char* s= hash_map[key][i];
    //return s.c_str();
    return hash_map[key][i];
}
void fore_clear(char* file_path)
{
    char *file_name;
    malloc(100);
    for(auto it = hash_map.begin();it!=hash_map.end();it++)
    {
        sprintf(file_name,"%s/%u.pcap",file_path,it->first);
        pcap_dumper_t * dumper = pcap_dump_open(pcap_open_dead(1, 1600), file_name);//DLT_EN10MB--打开pcap文件
        for(int pcap_i =0;pcap_i<it->second.size();pcap_i++)
        {
            // struct timeval tv;
            // gettimeofday(&tv,NULL);
            // printf("start write pcap\n");
            struct five_tuple_tmp temp_val = it->second.at(pcap_i);
            
            dumpFile(dumper,(const u_char*)temp_val.val,temp_val.data_len,temp_val.tv_sec,temp_val.tv_usec);
        
            //printf("end write pcap\n");
        }
        pcap_dump_close(dumper);//关闭pcap文件
        //然后清空该key值下的数据
        //hashmap_clear(it->second);
        it->second.clear();
    }
}
// #ifdef __cplusplus

// }

// #endif



