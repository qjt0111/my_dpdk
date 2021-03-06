
#aux_source_directory(. SOURCE_FILES )

set(SOURCE_FILES crc_key.c)
include_directories( /home/qjt/my_dpdk/crc32 )

# 生成目标库   SHARED动态库  STATIC 静态库
add_library(crc_key ${SOURCE_FILES} )