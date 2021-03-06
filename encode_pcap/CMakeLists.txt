
#aux_source_directory(. SOURCE_FILES )

set(SOURCE_FILES my_pcap.c)
include_directories( /home/qjt/my_dpdk/encode_pcap )

# 生成目标库   SHARED动态库  STATIC 静态库
add_library(my_pcap ${SOURCE_FILES} )