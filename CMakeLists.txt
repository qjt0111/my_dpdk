cmake_minimum_required(VERSION 3.0)
project(dpdk_hello)


#set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_FLAGS "-std=c99")

set(RTE_SDK /home/qjt/spdk/dpdk/dpdk-19.05)
set(RTE_TARGET x86_64-native-linuxapp-gcc)

set(SOURCE_FILE my_dpdk.c)

set(WERROR_FLAGS
        -W
        -Wall
#        -Wstrict-prototypes
#        -Wmissing-prototypes
        -Wmissing-declarations
#        -Wold-style-definition
        -Wpointer-arith
        -Wcast-align
#        -Wnested-externs
        -Wcast-qual
        -Wformat-nonliteral
        -Wformat-security
        -Wundef
        -Wwrite-strings
        -Wdeprecated)

set(CFLAGS
        -m64
        -pthread
        -g
        -march=native
        -include ${RTE_SDK}/${RTE_TARGET}/include/rte_config.h
        -O3
        ${WERROR_FLAGS})

set(DPDKLIBS
        -Wl,--whole-archive
#        dpdk
        rte_acl 
        rte_bbdev 
        rte_bitratestats 
        rte_bpf 
        rte_bus_dpaa 
        rte_bus_fslmc 
        rte_bus_ifpga 
        rte_bus_pci 
        rte_bus_vdev 
        rte_bus_vmbus 
        rte_cfgfile 
        rte_cmdline 
        rte_common_cpt 
        rte_common_dpaax 
        rte_common_octeontx 
        rte_compressdev 
        rte_cryptodev 
        rte_distributor 
        rte_eal 
        rte_efd 
        rte_ethdev 
        rte_eventdev 
        rte_flow_classify 
        rte_gro 
        rte_gso 
        rte_hash 
        rte_ip_frag 
        rte_jobstats 
        rte_kni 
        rte_kvargs 
        rte_latencystats 
        rte_lpm 
        rte_mbuf 
        rte_member 
        rte_mempool 
        rte_mempool_bucket 
        rte_mempool_dpaa2 
        rte_mempool_dpaa 
        rte_mempool_octeontx 
        rte_mempool_ring 
        #rte_mempool_stack 
        rte_meter 
        rte_metrics 
        rte_net 
        rte_pci 
        rte_pdump 
        rte_pipeline 
        rte_pmd_af_packet 
        rte_pmd_ark 
        rte_pmd_atlantic 
        #rte_pmd_avf 
        rte_pmd_avp 
        rte_pmd_axgbe 
        rte_pmd_bbdev_null 
        rte_pmd_bnxt 
        rte_pmd_bond 
        rte_pmd_caam_jr 
        rte_pmd_crypto_scheduler 
        rte_pmd_cxgbe 
        rte_pmd_dpaa2 
        rte_pmd_dpaa2_cmdif 
        rte_pmd_dpaa2_event 
        rte_pmd_dpaa2_qdma 
        rte_pmd_dpaa2_sec 
        rte_pmd_dpaa 
        rte_pmd_dpaa_event 
        rte_pmd_dpaa_sec 
        rte_pmd_dsw_event 
        rte_pmd_e1000 
        rte_pmd_ena 
        rte_pmd_enetc 
        rte_pmd_enic 
        rte_pmd_failsafe 
        rte_pmd_fm10k 
        rte_pmd_i40e 
        rte_pmd_ifc 
        rte_pmd_ifpga_rawdev 
        rte_pmd_ixgbe 
        rte_pmd_kni 
        rte_pmd_lio 
        rte_pmd_netvsc 
        rte_pmd_nfp 
        rte_pmd_null 
        rte_pmd_null_crypto 
        rte_pmd_octeontx 
        rte_pmd_octeontx_crypto 
        rte_pmd_octeontx_ssovf 
        rte_pmd_octeontx_zip 
        rte_pmd_opdl_event 
        #rte_pmd_pcap 
        rte_pmd_qat 
        rte_pmd_qede 
        rte_pmd_ring 
        rte_pmd_sfc_efx 
        rte_pmd_skeleton_event 
        rte_pmd_skeleton_rawdev 
        rte_pmd_softnic 
        rte_pmd_sw_event 
        rte_pmd_tap 
        rte_pmd_thunderx_nicvf 
        rte_pmd_vdev_netvsc 
        rte_pmd_vhost 
        rte_pmd_virtio 
        rte_pmd_virtio_crypto 
        rte_pmd_vmxnet3_uio 
        rte_port
        rte_power
        rte_rawdev
        rte_reorder
        rte_ring
        rte_sched
        rte_security
        rte_table
        rte_timer
        rte_vhost
        #rte_pmd_mlx5
        -Wl,--no-whole-archive)


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

add_definitions(
        -DRTE_MACHINE_CPUFLAG_SSE
        -DRTE_MACHINE_CPUFLAG_SSE2
        -DRTE_MACHINE_CPUFLAG_SSE3
        -DRTE_MACHINE_CPUFLAG_SSSE3
        -DRTE_MACHINE_CPUFLAG_SSE4_1
        -DRTE_MACHINE_CPUFLAG_SSE4_2
        -DRTE_MACHINE_CPUFLAG_AES
        -DRTE_MACHINE_CPUFLAG_PCLMULQDQ
        -DRTE_MACHINE_CPUFLAG_AVX
        -DRTE_MACHINE_CPUFLAG_RDRAND
        -DRTE_MACHINE_CPUFLAG_FSGSBASE
        -DRTE_MACHINE_CPUFLAG_F16C
        -DRTE_MACHINE_CPUFLAG_AVX2
        -D_GNU_SOURCE)

add_compile_options(${CFLAGS})


include_directories(${RTE_SDK}/${RTE_TARGET}/include)

link_directories(${RTE_SDK}/${RTE_TARGET}/lib)


include_directories($(SPDK_include))
link_directories($(SPDK_libs))

include_directories(/home/qjt/my_dpdk/hash)
include_directories(/home/qjt/my_dpdk/dpdk_map)

link_libraries(
        ${DPDKLIBS}
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
add_subdirectory(hash)
add_subdirectory(crc32)
#add_subdirectory(encode_pcap)
add_subdirectory(dpdk_map)
add_executable(dpdk_hello ${SOURCE_FILE})

target_link_libraries(dpdk_hello librte_pmd_mlx5.so hash crc_key dpdk_map)#链接mlx网卡驱动的动态库



