
#aux_source_directory(. SOURCE_FILES )

set(SOURCE_FILES cuckoo.c hash.c)
include_directories( /home/qjt/my_dpdk/hash )

# 生成目标库   SHARED动态库  STATIC 静态库
add_library(hash ${SOURCE_FILES} )