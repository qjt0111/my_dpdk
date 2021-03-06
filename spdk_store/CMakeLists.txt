set(SOURCE_FILES spdk_store.c)
include_directories( /home/qjt/my_dpdk/spdk_store)

set(SPDKLIBS
    -Wl,--whole-archive
    #spdk
    spdk_accel
    spdk_accel_ioat
    spdk_bdev
    #spdk_bdev_aio
    spdk_bdev_delay
    spdk_bdev_error
    spdk_bdev_ftl
    spdk_bdev_gpt
    spdk_bdev_lvol
    spdk_bdev_malloc
    spdk_bdev_null
    spdk_bdev_nvme
    spdk_bdev_passthru
    spdk_bdev_raid
    spdk_bdev_split
    spdk_bdev_virtio
    spdk_bdev_zone_block
    spdk_blob
    spdk_blob_bdev
    spdk_blobfs
    spdk_blobfs_bdev
    spdk_conf
    spdk_env_dpdk
    spdk_env_dpdk_rpc
    spdk_event
    spdk_event_accel
    spdk_event_bdev
    #spdk_event_iscsi
    spdk_event_nbd
    spdk_event_net
    spdk_event_nvmf
    spdk_event_scsi
    spdk_event_sock
    spdk_event_vmd
    spdk_ftl
    spdk_ioat
    #spdk_iscsi
    spdk_json
    spdk_jsonrpc
    spdk_log
    spdk_lvol
    spdk_nbd
    spdk_net
    spdk_notify
    spdk_nvme
    spdk_nvmf
    spdk_rpc
    spdk_scsi
    spdk_sock
    spdk_sock_posix
    spdk_thread
    spdk_trace
    spdk_util
    #/usr/local/lib/libspdk_util.a
    #spdk_ut_mock
    spdk_virtio
    spdk_vmd
    -Wl,--no-whole-archive
)
#set(SPDK_include)
#include_directories($(SPDK_include))
#link_directories($(SPDK_libs))

link_libraries(
        #${DPDKLIBS}
        ${SPDKLIBS}
        pthread        
        rt
        dl
        m
        #mlx5
        #mlx4
        numa
        uuid #关键
        pcap)

# 生成目标库   SHARED动态库  STATIC 静态库
add_library(spdk_store ${SOURCE_FILES})

#target_link_libraries(spdk_store ${SPDKLIBS})