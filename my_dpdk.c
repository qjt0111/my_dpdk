/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2016 Intel Corporation
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>

#include <rte_common.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_eal.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>

#include "spdk/stdinc.h"
#include "spdk/nvme.h"
#include "spdk/vmd.h"
#include "spdk/nvme_zns.h"
#include "spdk/env.h"
#include <sys/time.h>

#include "dpdk_map.h"
#include <pcap.h>
#include "cuckoo.h"
#include "my_dpdk.h"
struct cuckoo *c ;


//原子锁

static int lock;
#define atomic_cas(dst, old, new) __sync_bool_compare_and_swap((dst), (old), (new))
#define atomic_lock(ptr)\
while(!atomic_cas(ptr,0,1))
#define atomic_unlock(ptr)\
while(!atomic_cas(ptr,1,0))

#define FILEPATH_MAX (80)

#define data_size 128
#define ring_size  1024*1024*8

struct rte_mempool * mydpdk_pktmbuf_pool = NULL;  //指向内存池结构的指针变量
struct rte_ring *spdk_ring; //无锁队列
static volatile bool force_quit;


struct metadata_info//用于包级别索引
{
	/* data */
	uint8_t ip_server_type;//ip层的服务类型
	uint8_t ip_version;//1字节，ipv4或者ipv6
	uint16_t mac_next_proto;//ICMP,IGMMP,IP
	uint16_t ip_next_proto;//ip层下一层的协议类型
	
	//uint8_t SMB_header;//头部
	uint8_t src_mac[6];
	uint8_t dst_mac[6];
	//uint16_t SMB_port;//使用的端口
	uint16_t src_port;
	uint16_t dst_port;
	uint32_t src_ip;
	uint32_t dst_ip;

	//uint32_t SMB_command_header;//命令头部

	uint64_t pkt_len;//数据包长
	time_t timestamp;//时间戳---8字节
	//char timestamp[20];
	//64-56=8
	char str[16];//描述信息
};
struct  five_tuple//用于hash
{
	/* data */
	uint8_t ip_next_proto;//传输层的的协议
	uint32_t src_ip;
	uint32_t dst_ip;
	uint32_t src_port;
	uint32_t dst_port;
};

 static char *ip_transform(long int ip_addr)//将大端序列转为小端序序列
{   
    char *buf = (char *)malloc(128);
    long int *ip = & ip_addr;
    unsigned char *ptr_uc = (unsigned char *)ip;
    snprintf(buf,128,"%u.%u.%u.%u",ptr_uc[3], ptr_uc[2], ptr_uc[1], ptr_uc[0]);//将ptr_uc[3], ptr_uc[2], ptr_uc[1], ptr_uc[0]按"%u.%u.%u.%u"的格式放到buf里，128为buf大小
    static char ip_adr[20];
    strcpy(ip_adr,buf);
    free(buf);
    return ip_adr;
}

static void en_ring_queue(struct metadata_info *info)
{
	void *data;
	if(rte_mempool_get(mydpdk_pktmbuf_pool,&data)<0)//每次获取的data的长度是可以变化的的
	{
		printf("get mbuf_pool fail!--698\n");
		return ;
	}
	memcpy(data,info,sizeof(info));
	if(rte_ring_enqueue(spdk_ring,&data)<0)
	{
		rte_mempool_put(mydpdk_pktmbuf_pool,&data);
	}
} 

static struct metadata_info* de_ring_queue(void)
{
	void *data;
	// if(rte_mempool_get(mydpdk_pktmbuf_pool,&data)<0)//每次获取的data的长度是可以变化的的
	// {
	// 	printf("get mbuf_pool fail!--712\n");
	// 	return ;
	// }
	if(rte_ring_dequeue(spdk_ring,&data)<0)
	{
		printf("ring_dequeue fail!--717\n");
		return NULL;
	}
	return (struct metadata_info*)data;
}

// static void en_queue(char * strng_val)
// {
//     if (rte_ring_enqueue(spdk_ring, strng_val) < 0) {
//             rte_mempool_put(mydpdk_pktmbuf_pool, strng_val);
//         }    
// }

// static void de_queue()
// {
//     void *recv_msg;
//     unsigned vv,core_id;
//     //rte_mempool_get(mydpdk_pktmbuf_pool, &recv_msg);//每次获取的recv_msg的长度是可以变化的的
//     if (rte_ring_dequeue(spdk_ring, &recv_msg) < 0){
//     }
//      sscanf(recv_msg,"hello from core %u:---data:%u\n",&core_id,&vv);

//     printf("Received:%u  -- : '%s'",strlen(recv_msg),  (char *)recv_msg);
//     rte_mempool_put(mydpdk_pktmbuf_pool, recv_msg);
// }


struct ctrlr_entry {
	struct spdk_nvme_ctrlr		*ctrlr;
	TAILQ_ENTRY(ctrlr_entry)	link;//尾队列
	char				name[1024];
};

struct ns_entry {
	struct spdk_nvme_ctrlr	*ctrlr;
	struct spdk_nvme_ns	*ns;
	TAILQ_ENTRY(ns_entry)	link;//尾队列
	struct spdk_nvme_qpair	*qpair;
};

static TAILQ_HEAD(, ctrlr_entry) g_controllers = TAILQ_HEAD_INITIALIZER(g_controllers);//初始化控制器队列头部
static TAILQ_HEAD(, ns_entry) g_namespaces = TAILQ_HEAD_INITIALIZER(g_namespaces);//初始化namespace队列头部

static bool g_vmd = false;//是否使用vmd(卷管理设备)
static bool g_external_init = true;

static void
register_ns(struct spdk_nvme_ctrlr *ctrlr, struct spdk_nvme_ns *ns)
{
	struct ns_entry *entry;

	if (!spdk_nvme_ns_is_active(ns)) {
		return;
	}

	entry =(struct ns_entry*)malloc(sizeof(struct ns_entry));
	if (entry == NULL) {
		perror("ns_entry malloc error");
		exit(1);
	}

	entry->ctrlr = ctrlr;
	entry->ns = ns;
	TAILQ_INSERT_TAIL(&g_namespaces, entry, link);//插入到队列尾部

	printf("  Namespace ID: %d size: %juGB\n", spdk_nvme_ns_get_id(ns),
	       spdk_nvme_ns_get_size(ns) / 1000000000);
}

struct hello_world_sequence {
	struct ns_entry	*ns_entry;
	void		*buf;
	unsigned        using_cmb_io;
	int		is_completed;
};

static void
read_complete(void *arg, const struct spdk_nvme_cpl *completion)
{
	struct hello_world_sequence *sequence = arg;

	/* Assume the I/O was successful */
	sequence->is_completed = 1;//假设该read操作已经完成
	/* See if an error occurred. If so, display information
	 * about it, and set completion value so that I/O
	 * caller is aware that an error occurred.
	 */
	if (spdk_nvme_cpl_is_error(completion)) {//检查nvme完成是否错误
		//打印nvme完成队列条目的内容
		spdk_nvme_qpair_print_completion(sequence->ns_entry->qpair, (struct spdk_nvme_cpl *)completion);
		fprintf(stderr, "I/O error status: %s\n", spdk_nvme_cpl_get_status_string(&completion->status));
		fprintf(stderr, "Read I/O failed, aborting run\n");
		sequence->is_completed = 2;
		exit(1);
	}

	/*
	 * The read I/O has completed.  Print the contents of the
	 *  buffer, free the buffer, then mark the sequence as
	 *  completed.  This will trigger the hello_world() function
	 *  to exit its polling loop.
	 */
	printf("%zu\n", strlen(sequence->buf));
	printf("%s", sequence->buf);
	spdk_free(sequence->buf);
}

static void
write_complete(void *arg, const struct spdk_nvme_cpl *completion)
{
	struct hello_world_sequence	*sequence = arg;
	struct ns_entry			*ns_entry = sequence->ns_entry;
	int				rc;

	/* See if an error occurred. If so, display information
	 * about it, and set completion value so that I/O
	 * caller is aware that an error occurred.
	 */
	if (spdk_nvme_cpl_is_error(completion)) {
		spdk_nvme_qpair_print_completion(sequence->ns_entry->qpair, (struct spdk_nvme_cpl *)completion);
		fprintf(stderr, "I/O error status: %s\n", spdk_nvme_cpl_get_status_string(&completion->status));
		fprintf(stderr, "Write I/O failed, aborting run\n");
		sequence->is_completed = 2;
		exit(1);
	}
	sequence->is_completed=1;
	/*
	 * The write I/O has completed.  Free the buffer associated with
	 *  the write I/O and allocate a new zeroed buffer for reading
	 *  the data back from the NVMe namespace.
	 */
	// if (sequence->using_cmb_io) {
	// 	spdk_nvme_ctrlr_unmap_cmb(ns_entry->ctrlr);
	// } else {
	// 	spdk_free(sequence->buf);
	// }
	//sequence->buf = spdk_zmalloc(0x1000, 0x1000, NULL, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);

	// rc = spdk_nvme_ns_cmd_read(ns_entry->ns, ns_entry->qpair, sequence->buf,
	// 			   2, /* LBA start 起始扇区 */
	// 			   1, /* number of LBAs 要是用的扇区个数 */
	// 			   read_complete, (void *)sequence, 0);
	// if (rc != 0) {
	// 	fprintf(stderr, "starting read I/O failed\n");
	// 	exit(1);
	// }
}


static void
reset_zone_complete(void *arg, const struct spdk_nvme_cpl *completion)
{
	struct hello_world_sequence *sequence = arg;

	/* Assume the I/O was successful */
	sequence->is_completed = 1;
	/* See if an error occurred. If so, display information
	 * about it, and set completion value so that I/O
	 * caller is aware that an error occurred.
	 */
	if (spdk_nvme_cpl_is_error(completion)) {//
		spdk_nvme_qpair_print_completion(sequence->ns_entry->qpair, (struct spdk_nvme_cpl *)completion);
		fprintf(stderr, "I/O error status: %s\n", spdk_nvme_cpl_get_status_string(&completion->status));
		fprintf(stderr, "Reset zone I/O failed, aborting run\n");
		sequence->is_completed = 2;
		exit(1);
	}
}

static void
reset_zone_and_wait_for_completion(struct hello_world_sequence *sequence)
{
	//重置操作提交到nvme的命名空间
	if (spdk_nvme_zns_reset_zone(sequence->ns_entry->ns, sequence->ns_entry->qpair,
				     0, /* starting LBA of the zone to reset */
				     false, /* don't reset all zones */
				     reset_zone_complete,
				     sequence)) {
		fprintf(stderr, "starting reset zone I/O failed\n");
		exit(1);
	}
	while (!sequence->is_completed) {
		spdk_nvme_qpair_process_completions(sequence->ns_entry->qpair, 0);
	}
	sequence->is_completed = 0;
}


static void
hello_world(int n,void*data)
{
	struct ns_entry			*ns_entry;
	struct hello_world_sequence	sequence;
	int				rc;
	size_t				sz;

	TAILQ_FOREACH(ns_entry, &g_namespaces, link) {//遍历尾队列中的每一个元素
		/*
		 * Allocate an I/O qpair that we can use to submit read/write requests
		 *  to namespaces on the controller.  NVMe controllers typically support
		 *  many qpairs per controller.  Any I/O qpair allocated for a controller
		 *  can submit I/O to any namespace on that controller.
		 *
		 * The SPDK NVMe driver provides no synchronization for qpair accesses -
		 *  the application must ensure only a single thread submits I/O to a
		 *  qpair, and that same thread must also check for completions on that
		 *  qpair.  This enables extremely efficient I/O processing by making all
		 *  I/O operations completely lockless.
		 */
		//根据控制器信息为I/O申请提交/完成qpair队列
		ns_entry->qpair = spdk_nvme_ctrlr_alloc_io_qpair(ns_entry->ctrlr, NULL, 0);
		if (ns_entry->qpair == NULL) {
			printf("ERROR: spdk_nvme_ctrlr_alloc_io_qpair() failed\n");
			return;
		}

		/*
		 * Use spdk_dma_zmalloc to allocate a 4KB zeroed buffer.  This memory
		 * will be pinned, which is required for data buffers used for SPDK NVMe
		 * I/O operations.
		 */
		sequence.using_cmb_io = 1;
		sequence.buf = spdk_nvme_ctrlr_map_cmb(ns_entry->ctrlr, &sz);//从控制器中申请内存
		//如果ctrlr中的buf不够则从主机内存中去分配
		if (sequence.buf == NULL || sz < 0x200) {
			sequence.using_cmb_io = 0;
			sequence.buf = spdk_zmalloc(0x200, 0x200, NULL, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);
		}
		if (sequence.buf == NULL) {
			printf("ERROR: write buffer allocation failed\n");
			return;
		}
		if (sequence.using_cmb_io) {
			printf("INFO: using controller memory buffer for IO\n");
		} else {
			printf("INFO: using host memory buffer for IO\n");
		}
		sequence.is_completed = 0;
		sequence.ns_entry = ns_entry;

		/*
		 * If the namespace is a Zoned Namespace, rather than a regular
		 * NVM namespace, we need to reset the first zone, before we
		 * write to it. This not needed for regular NVM namespaces.
		 */
		//若是分区的namespace则需要在写入第一个区域之前对其进行重置
		if (spdk_nvme_ns_get_csi(ns_entry->ns) == SPDK_NVME_CSI_ZNS) {//判断是否是分区的namespace
			reset_zone_and_wait_for_completion(&sequence);
		}

		/*
		 * Print "Hello world!" to sequence.buf.  We will write this data to LBA
		 *  0 on the namespace, and then later read it back into a separate buffer
		 *  to demonstrate the full I/O path.
		 */
		int ns_nsize = spdk_nvme_ns_get_size(ns_entry->ns);//获取命名空间ns的最大size
		int ctrlr_max_size = spdk_nvme_ctrlr_get_max_xfer_size(ns_entry->ctrlr);//(128M)NVMe控制器的最大数据传输大小（以字节为单位）
		int sector_size = spdk_nvme_ns_get_sector_size(ns_entry->ns);//获取指定命名空间扇区大小
		//char * str = "Hello world_qjt!\n";
		
		//snprintf(sequence.buf, 0x1000, "string:%s, ns_nsize:%d, ctrlr_max_size:%d ,sector_size:%d\n", str,ns_nsize,ctrlr_max_size,sector_size);//4K一个扇区大小
		printf("str_len:%u\n",strlen(sequence.buf));
		//snprintf(sequence.buf, 0x1000, "%s",sizeof(sequence.buf));
		/*
		 * Write the data buffer to LBA 0 of this namespace.  "write_complete" and
		 *  "&sequence" are specified as the completion callback function and
		 *  argument respectively.  write_complete() will be called with the
		 *  value of &sequence as a parameter when the write I/O is completed.
		 *  This allows users to potentially specify different completion
		 *  callback routines for each I/O, as well as pass a unique handle
		 *  as an argument so the application knows which I/O has completed.
		 *
		 * Note that the SPDK NVMe driver will only check for completions
		 *  when the application calls spdk_nvme_qpair_process_completions().
		 *  It is the responsibility of the application to trigger the polling
		 *  process.
		 */

		// int n = 1;
		// size_t metadata_size = sizeof(struct metadata_info);
		// int n_sec = 512/metadata_size;
		// //char data_buf[512];
		// while (!force_quit)
		// {
		// 	/* code */
		// 	while (rte_ring_count(spdk_ring)>=8)
		// 	{
		// 		/* code */
		// 		for(int ring_i=0;ring_i<n_sec;ring_i++)
		// 		{
		// 			struct metadata_info* info  = de_ring_queue();
		// 			memcpy(sequence.buf+metadata_size*ring_i,info,sizeof(struct metadata_info));
		// 			rte_mempool_put(mydpdk_pktmbuf_pool,&info);
		// 		}
		// 		//sequence.buf = data_buf;
		// 		rc = spdk_nvme_ns_cmd_write(ns_entry->ns, ns_entry->qpair, sequence.buf,
		// 		n++, /* LBA start 扇区的起始位置 */
		// 		1, /* number of LBAs 写入的扇区个数 */
		// 		write_complete, &sequence, 0);//该命令将此i/o请求提交给上一步申请的qpair-----   //暂时只有单纯的写到spdk中
		// 		printf("write---position:%d\n",n);
		// 		if (rc != 0) {
		// 			fprintf(stderr, "starting write I/O failed\n");
		// 			exit(1);
		// 		}
		// 	}
			
		// }
		// while (rte_ring_count(spdk_ring)>=8)
		// {
		// 	/* code */
		// 	for(int ring_i=0;ring_i<n_sec;ring_i++)
		// 	{
		// 		struct metadata_info* info  = de_ring_queue();
		// 		memcpy(sequence.buf+metadata_size*ring_i,info,sizeof(struct metadata_info));
		// 		rte_mempool_put(mydpdk_pktmbuf_pool,&info);
		// 	}
		// 	//sequence.buf = data_buf;
		// 	rc = spdk_nvme_ns_cmd_write(ns_entry->ns, ns_entry->qpair, sequence.buf,
		// 	n++, /* LBA start 扇区的起始位置 */
		// 	1, /* number of LBAs 写入的扇区个数 */
		// 	write_complete, &sequence, 0);//该命令将此i/o请求提交给上一步申请的qpair-----   //暂时只有单纯的写到spdk中
		// 	if (rc != 0) {
		// 		fprintf(stderr, "starting write I/O failed\n");
		// 		exit(1);
		// 	}
		// }
		// int ring_i=0;
		// while (!rte_ring_empty(spdk_ring))
		// {
		// 	struct metadata_info* info  = de_ring_queue();
		// 	memcpy(sequence.buf+metadata_size*ring_i,info,sizeof(struct metadata_info));
		// 	rte_mempool_put(mydpdk_pktmbuf_pool,&info);
		// 	ring_i++;
		// }
		memcpy(sequence.buf,data,512);
		rc = spdk_nvme_ns_cmd_write(ns_entry->ns, ns_entry->qpair, sequence.buf,
			n, /* LBA start 扇区的起始位置 */
			1, /* number of LBAs 写入的扇区个数 */
			write_complete, &sequence, 0);//该命令将此i/o请求提交给上一步申请的qpair-----   //暂时只有单纯的写到spdk中


		// for(i=0;i<n;i++)
		// {
		// 	//功能：将写入的I/O提交到指定的NVMe命名空间
		// 	rc = spdk_nvme_ns_cmd_write(ns_entry->ns, ns_entry->qpair, sequence.buf,
		// 		i/10000, /* LBA start 扇区的起始位置 */
		// 		3, /* number of LBAs 写入的扇区个数 */
		// 		write_complete, &sequence, 0);//该命令将此i/o请求提交给上一步申请的qpair-----
		// }

		// rc = spdk_nvme_ns_cmd_write(ns_entry->ns, ns_entry->qpair, sequence.buf,
		// 			    0, /* LBA start */
		// 			    1, /* number of LBAs */
		// 			    write_complete, &sequence, 0);
		if (rc != 0) {
			fprintf(stderr, "starting write I/O failed\n");
			exit(1);
		}

		/*
		 * Poll for completions.  0 here means process all available completions.
		 *  In certain usage models, the caller may specify a positive integer
		 *  instead of 0 to signify the maximum number of completions it should
		 *  process.  This function will never block - if there are no
		 *  completions pending on the specified qpair, it will return immediately.
		 *
		 * When the write I/O completes, write_complete() will submit a new I/O
		 *  to read LBA 0 into a separate buffer, specifying read_complete() as its
		 *  completion routine.  When the read I/O completes, read_complete() will
		 *  print the buffer contents and set sequence.is_completed = 1.  That will
		 *  break this loop and then exit the program.
		 */
		while (!sequence.is_completed) {//处理在队列对上提交的未完成的i/o的所有未完成的操作
			spdk_nvme_qpair_process_completions(ns_entry->qpair, 0);//参数1：队列对，参数2：限制一次呼叫要处理的玩完成次数(0为无限制)
		}

		/*
		 * Free the I/O qpair.  This typically is done when an application exits.
		 *  But SPDK does support freeing and then reallocating qpairs during
		 *  operation.  It is the responsibility of the caller to ensure all
		 *  pending I/O are completed before trying to free the qpair.
		 */
		spdk_nvme_ctrlr_free_io_qpair(ns_entry->qpair);//释放申请的qpair
	}
}


static void spdk_write(void)
{
		int n = 1;
		size_t metadata_size = sizeof(struct metadata_info);
		int n_sec = 512/metadata_size;
		int hello_num=1;
		char data_buf[512];
		while (!force_quit)
		{
			/* code */
			while (rte_ring_count(spdk_ring)>=n_sec)
			{
				/* code */
				int ring_i=0;
				for(;ring_i<n_sec;ring_i++)
				{
					struct metadata_info* info  = de_ring_queue();
					memcpy(data_buf+metadata_size*ring_i,info,sizeof(struct metadata_info));
					rte_mempool_put(mydpdk_pktmbuf_pool,&info);
				}
				if(ring_i==n_sec)
				{
					hello_world(hello_num++,data_buf);
					printf("write---position:%d\n",hello_num);
				}
			}
			
		}
		while (rte_ring_count(spdk_ring)>=n_sec)
		{
			/* code */
			int ring_i=0;
			for(;ring_i<n_sec;ring_i++)
			{
				struct metadata_info* info  = de_ring_queue();
				memcpy(data_buf+metadata_size*ring_i,info,sizeof(struct metadata_info));
				rte_mempool_put(mydpdk_pktmbuf_pool,&info);
			}
			if(ring_i==n_sec)
			{
				hello_world(hello_num++,data_buf);
				printf("write---position:%d\n",hello_num);
			}
		}
		int ring_i=0;
		while (!rte_ring_empty(spdk_ring))
		{
			struct metadata_info* info  = de_ring_queue();
			memcpy(data_buf+metadata_size*ring_i,info,sizeof(struct metadata_info));
			rte_mempool_put(mydpdk_pktmbuf_pool,&info);
			ring_i++;
		}
		hello_world(hello_num++,data_buf);
		printf("write---position:%d\n",hello_num);
}


static bool
probe_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
	 struct spdk_nvme_ctrlr_opts *opts)
{
	printf("Attaching to %s\n", trid->traddr);

	return true;
}

static void
attach_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
	  struct spdk_nvme_ctrlr *ctrlr, const struct spdk_nvme_ctrlr_opts *opts)
{
	int nsid, num_ns;
	struct ctrlr_entry *entry;
	struct spdk_nvme_ns *ns;
	const struct spdk_nvme_ctrlr_data *cdata;

	entry = malloc(sizeof(struct ctrlr_entry));
	if (entry == NULL) {
		perror("ctrlr_entry malloc");
		exit(1);
	}

	printf("Attached to %s\n", trid->traddr);

	/*
	 * spdk_nvme_ctrlr is the logical abstraction in SPDK for an NVMe
	 *  controller.  During initialization, the IDENTIFY data for the
	 *  controller is read using an NVMe admin command, and that data
	 *  can be retrieved using spdk_nvme_ctrlr_get_data() to get
	 *  detailed information on the controller.  Refer to the NVMe
	 *  specification for more details on IDENTIFY for NVMe controllers.
	 */
	cdata = spdk_nvme_ctrlr_get_data(ctrlr);//得到nvme控制器中的数据

	snprintf(entry->name, sizeof(entry->name), "%-20.20s (%-20.20s)", cdata->mn, cdata->sn);

	entry->ctrlr = ctrlr;
	TAILQ_INSERT_TAIL(&g_controllers, entry, link);

	/*
	 * Each controller has one or more namespaces.  An NVMe namespace is basically
	 *  equivalent to a SCSI LUN.  The controller's IDENTIFY data tells us how
	 *  many namespaces exist on the controller.  For Intel(R) P3X00 controllers,
	 *  it will just be one namespace.
	 *
	 * Note that in NVMe, namespace IDs start at 1, not 0.
	 */
	//每个ctrlr中的namespace的起始是从1开始的
	num_ns = spdk_nvme_ctrlr_get_num_ns(ctrlr);//获取namespace的数量
	printf("Using controller %s with %d namespaces.\n", entry->name, num_ns);
	for (nsid = 1; nsid <= num_ns; nsid++) {
		ns = spdk_nvme_ctrlr_get_ns(ctrlr, nsid);//获取相应的namespace
		if (ns == NULL) {
			continue;
		}
		register_ns(ctrlr, ns);
	}
}


static void
cleanup(void)
{
	struct ns_entry *ns_entry, *tmp_ns_entry;
	struct ctrlr_entry *ctrlr_entry, *tmp_ctrlr_entry;
	struct spdk_nvme_detach_ctx *detach_ctx = NULL;

	TAILQ_FOREACH_SAFE(ns_entry, &g_namespaces, link, tmp_ns_entry) {
		TAILQ_REMOVE(&g_namespaces, ns_entry, link);
		free(ns_entry);
	}

	TAILQ_FOREACH_SAFE(ctrlr_entry, &g_controllers, link, tmp_ctrlr_entry) {
		TAILQ_REMOVE(&g_controllers, ctrlr_entry, link);
		spdk_nvme_detach_async(ctrlr_entry->ctrlr, &detach_ctx);
		free(ctrlr_entry);
	}

	while (detach_ctx && spdk_nvme_detach_poll_async(detach_ctx) == -EAGAIN) {
		;
	}
}

// static void
// usage(const char *program_name)
// {
// 	printf("%s [options]", program_name);
// 	printf("\n");
// 	printf("options:\n");
// 	printf(" -V         enumerate VMD\n");
// }

// static int
// parse_args(int argc, char **argv)
// {
// 	int op;

// 	while ((op = getopt(argc, argv, "V")) != -1) {
// 		switch (op) {
// 		case 'V':
// 			g_vmd = true;
// 			break;
// 		default:
// 			usage(argv[0]);
// 			return 1;
// 		}
// 	}

// 	return 0;
// }


//static volatile bool force_quit;

/* MAC updating enabled by default */
// static int mac_updating = 1;

//#define RTE_LOGTYPE_L2FWD RTE_LOGTYPE_USER1

int count = 0;



// struct five_tuple_tmp
// {
// 	/* data */
// 	char val[1500];
// 	time_t tv_sec;
// 	suseconds_t tv_usec;
// };



#define MAX_PKT_BURST 32
#define BURST_TX_DRAIN_US 100 /* TX drain every ~100us */
#define MEMPOOL_CACHE_SIZE 256

/*
 * Configurable number of RX/TX ring descriptors
 */
#define RTE_TEST_RX_DESC_DEFAULT 1024
#define RTE_TEST_TX_DESC_DEFAULT 1024
static uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
static uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;

/* ethernet addresses of ports */
static struct ether_addr mydpdk_ports_eth_addr[RTE_MAX_ETHPORTS];

/* mask of enabled ports */
static uint32_t mydpdk_enabled_port_mask = 0;

/* list of enabled ports */
static uint32_t mydpdk_dst_ports[RTE_MAX_ETHPORTS];

static unsigned int mydpdk_rx_queue_per_lcore = 1;

#define MAX_RX_QUEUE_PER_LCORE 16
#define MAX_TX_QUEUE_PER_PORT 16
struct lcore_queue_conf {
	unsigned n_rx_port;
	unsigned rx_port_list[MAX_RX_QUEUE_PER_LCORE];
} __rte_cache_aligned;
struct lcore_queue_conf lcore_queue_conf[RTE_MAX_LCORE];

// static struct rte_eth_dev_tx_buffer *tx_buffer[RTE_MAX_ETHPORTS];//转发队列内存区

static struct rte_eth_conf port_conf = {
	.rxmode = {
		.split_hdr_size = 0,
	},
	.txmode = {
		.mq_mode = ETH_MQ_TX_NONE,
	},
};

//struct rte_mempool * mydpdk_pktmbuf_pool = NULL;

/* Per-port statistics struct */
// struct l2fwd_port_statistics {
// 	uint64_t tx;
// 	uint64_t rx;
// 	uint64_t dropped;
// } __rte_cache_aligned;
// struct l2fwd_port_statistics port_statistics[RTE_MAX_ETHPORTS];

#define MAX_TIMER_PERIOD 86400 /* 1 day max */
/* A tsc-based timer responsible for triggering statistics printout */
static uint64_t timer_period = 10; /* default period is 10 seconds */

/* Print out statistics on packets dropped */
// static void
// print_stats(void)
// {
// 	uint64_t total_packets_dropped, total_packets_tx, total_packets_rx;
// 	unsigned portid;

// 	total_packets_dropped = 0;
// 	total_packets_tx = 0;
// 	total_packets_rx = 0;

// 	const char clr[] = { 27, '[', '2', 'J', '\0' };
// 	const char topLeft[] = { 27, '[', '1', ';', '1', 'H','\0' };

// 		/* Clear screen and move to top left */
// 	printf("%s%s", clr, topLeft);

// 	printf("\nPort statistics ====================================");

// 	for (portid = 0; portid < RTE_MAX_ETHPORTS; portid++) {
// 		/* skip disabled ports */
// 		if ((mydpdk_enabled_port_mask & (1 << portid)) == 0)
// 			continue;
// 		printf("\nStatistics for port %u ------------------------------"
// 			   "\nPackets sent: %24"PRIu64
// 			   "\nPackets received: %20"PRIu64
// 			   "\nPackets dropped: %21"PRIu64,
// 			   portid,
// 			   port_statistics[portid].tx,
// 			   port_statistics[portid].rx,
// 			   port_statistics[portid].dropped);

// 		total_packets_dropped += port_statistics[portid].dropped;
// 		total_packets_tx += port_statistics[portid].tx;
// 		total_packets_rx += port_statistics[portid].rx;
// 	}
// 	printf("\nAggregate statistics ==============================="
// 		   "\nTotal packets sent: %18"PRIu64
// 		   "\nTotal packets received: %14"PRIu64
// 		   "\nTotal packets dropped: %15"PRIu64,
// 		   total_packets_tx,
// 		   total_packets_rx,
// 		   total_packets_dropped);
// 	printf("\n====================================================\n");
// }

// static void
// l2fwd_mac_updating(struct rte_mbuf *m, unsigned dest_portid)
// {
// 	struct ether_hdr *eth;
// 	void *tmp;

// 	eth = rte_pktmbuf_mtod(m, struct ether_hdr *);

// 	/* 02:00:00:00:00:xx */
// 	tmp = &eth->d_addr.addr_bytes[0];
// 	*((uint64_t *)tmp) = 0x000000000002 + ((uint64_t)dest_portid << 40);

// 	/* src addr */
// 	ether_addr_copy(&mydpdk_ports_eth_addr[dest_portid], &eth->s_addr);
// }

// static void
// l2fwd_simple_forward(struct rte_mbuf *m, unsigned portid)
// {
// 	unsigned dst_port;
// 	int sent;
// 	struct rte_eth_dev_tx_buffer *buffer;

// 	dst_port = mydpdk_dst_ports[portid];

// 	if (mac_updating)
// 		l2fwd_mac_updating(m, dst_port);

// 	buffer = tx_buffer[dst_port];
// 	sent = rte_eth_tx_buffer(dst_port, 0, buffer, m);
// 	if (sent)
// 		port_statistics[dst_port].tx += sent;
// }




/* main processing loop */
static void
mydpdk_main_loop(void)
{
	struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
	struct rte_mbuf *m;
	//int sent;
	unsigned lcore_id;
	//uint64_t prev_tsc, diff_tsc, cur_tsc, timer_tsc;
	unsigned i, j, portid, nb_rx,mac_addr_n;
	struct lcore_queue_conf *qconf;
	 //const uint64_t drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S *
	// 		BURST_TX_DRAIN_US;
	//struct rte_eth_dev_tx_buffer *buffer;

	// prev_tsc = 0;
	// timer_tsc = 0;

	lcore_id = rte_lcore_id();//获取当前lcore
	qconf = &lcore_queue_conf[lcore_id];

	if (qconf->n_rx_port == 0) {
		//RTE_LOG(INFO, L2FWD, "lcore %u has nothing to do\n", lcore_id);
        printf("MY_DPDK: lcore %u has nothing to do\n", lcore_id);
		return;
	}

	//RTE_LOG(INFO, L2FWD, "entering main loop on lcore %u\n", lcore_id);
    printf("MY_DPDK: entering main loop on lcore %u\n", lcore_id);
	for (i = 0; i < qconf->n_rx_port; i++) {

		portid = qconf->rx_port_list[i];
		//RTE_LOG(INFO, L2FWD, " -- lcoreid=%u portid=%u\n", lcore_id,portid);
        printf("MY_DPDK: -- lcoreid=%u portid=%u\n", lcore_id,portid);
	}


    struct ipv4_hdr * ipv4_header;
    struct tcp_hdr * tcp_header;
    struct udp_hdr * udp_header;
    struct ether_hdr * ether_header;
	
	unsigned int crc32_result=0;
	unsigned char file_name[100];
	while (!force_quit) {

		// cur_tsc = rte_rdtsc();

		// /*
		//  * TX burst queue drain
		//  */
		// diff_tsc = cur_tsc - prev_tsc;
		// if (unlikely(diff_tsc > drain_tsc)) {

		// 	for (i = 0; i < qconf->n_rx_port; i++) {

		// 		portid = mydpdk_dst_ports[qconf->rx_port_list[i]];
		// 		buffer = tx_buffer[portid];

		// 		sent = rte_eth_tx_buffer_flush(portid, 0, buffer);
		// 		if (sent)
		// 			port_statistics[portid].tx += sent;

		// 	}

		// 	/* if timer is enabled */
		// 	if (timer_period > 0) {

		// 		/* advance the timer */
		// 		timer_tsc += diff_tsc;

		// 		/* if timer has reached its timeout */
		// 		if (unlikely(timer_tsc >= timer_period)) {

		// 			/* do this only on master core */
		// 			if (lcore_id == rte_get_master_lcore()) {
		// 				print_stats();
		// 				/* reset the timer */
		// 				timer_tsc = 0;
		// 			}
		// 		}
		// 	}

		// 	prev_tsc = cur_tsc;
		// }

		/*
		 * Read packet from RX queues
		 */

		for (i = 0; i < qconf->n_rx_port; i++) {

			portid = qconf->rx_port_list[i];
			nb_rx = rte_eth_rx_burst(portid, 0,
						 pkts_burst, MAX_PKT_BURST);
			if (unlikely(nb_rx == 0))
				continue;
			count+=nb_rx;

			//uint64_t timestamp = rte_get_timer_cycles();
			//uint64_t timestamp = rte_get_tsc_cycles();
            uint64_t timestamp = time(NULL);
			//printf("core:%u_port:%u receive packets nums :%d    count:%d    time:%u\n",lcore_id,portid,nb_rx,count,timestamp);
            
            

			//port_statistics[portid].rx += nb_rx;

			// for (j = 0; j < nb_rx; j++) {
			// 	m = pkts_burst[j];
			// 	rte_prefetch0(rte_pktmbuf_mtod(m, void *));
			// 	l2fwd_simple_forward(m, portid);
			// }

            //提取特征
			struct  metadata_info temp_metadata;
			//hash 五元组信息
			struct five_tuple key;
			memset(&temp_metadata,0,sizeof(struct metadata_info));
            
			for( j = 0; j < nb_rx; j++)
            {
				
                m = pkts_burst[j];
                m->buf_addr = (char*)m+sizeof(struct rte_mbuf);
				temp_metadata.pkt_len = m->pkt_len;
                printf("pkt_buf_addr:%s\n",(char *)m->buf_addr);//mbuf地址
                printf("pkt_len:%d\n",m->pkt_len);//一条数据报文的是长度
				printf("data_off:%d\n",m->data_off);//数据包的数据区域
                ether_header = (struct ether_hdr *)((char *)m->buf_addr+m->data_off);//m->buf_addr指向rte_mbuf头部，data_off为数据部分偏移位。

				struct ether_addr dst_addr = ether_header->d_addr;
				struct ether_addr src_addr = ether_header->s_addr;
				for(mac_addr_n =0; mac_addr_n<6; mac_addr_n++)
				{
					temp_metadata.src_mac[mac_addr_n] = src_addr.addr_bytes[mac_addr_n];
					temp_metadata.dst_mac[mac_addr_n] = dst_addr.addr_bytes[mac_addr_n];
				}
				temp_metadata.mac_next_proto = ether_header->ether_type;
				//printf("%u\n");
				if(ether_header->ether_type!=8)
				{
					rte_pktmbuf_free(m);
					continue;
				}
				// printf("src_mac : %02x-%02x-%02x-%02x-%02x-%02x\n", dst_addr.addr_bytes[0], dst_addr.addr_bytes[1], dst_addr.addr_bytes[2], dst_addr.addr_bytes[3], dst_addr.addr_bytes[4], dst_addr.addr_bytes[5]);
				// printf("dst_mac : %02x-%02x-%02x-%02x-%02x-%02x\n", src_addr.addr_bytes[0], src_addr.addr_bytes[1], src_addr.addr_bytes[2], src_addr.addr_bytes[3], src_addr.addr_bytes[4], src_addr.addr_bytes[5]);
				// printf("ethernet type : %u\n", ether_header->ether_type);

                ipv4_header = (struct ipv4_hdr *)((char *)m->buf_addr+m->data_off +sizeof(struct ether_hdr));
                 //ipv4_header = (struct ipv4_hdr *) rte_pktmbuf_adj(m, sizeof(struct ether_hdr));
				//  printf("IP src_addr:%s\n",ip_transform(rte_be_to_cpu_32(ipv4_header->src_addr)));
				//  printf("IP dst_addr:%s\n",ip_transform(rte_be_to_cpu_32(ipv4_header->dst_addr)));
				temp_metadata.src_ip = ipv4_header->src_addr;
				temp_metadata.dst_ip = ipv4_header->dst_addr;
				temp_metadata.ip_next_proto = ipv4_header->next_proto_id;

				key.src_ip = ipv4_header->src_addr;
				key.dst_ip = ipv4_header->dst_addr;
				key.ip_next_proto = ipv4_header->next_proto_id;
                if(ipv4_header->next_proto_id==1)
                {
                    printf("this is a icmp proto!\n");
                    printf("---------------------------------------------");
					rte_pktmbuf_free(m);
					continue;
                }else if(ipv4_header->next_proto_id ==6)
                {
                    /* code */
                    tcp_header = (struct tcp_hdr *)((char *)m->buf_addr+m->data_off +sizeof(struct ether_hdr)+sizeof(struct ipv4_hdr));
					//tcp_header = (struct tcp_hdr *) rte_pktmbuf_adj(m, (uint16_t)sizeof(struct ipv4_hdr));
					// printf("Tcp src_port:%d\n",ntohs(tcp_header->src_port));
					// printf("Tcp dst_port:%d\n",ntohs(tcp_header->dst_port));
					temp_metadata.src_port = ntohs(tcp_header->src_port);
					temp_metadata.dst_port = ntohs(tcp_header->dst_port);

					key.src_port = ntohs(tcp_header->src_port);
					key.dst_port = ntohs(tcp_header->dst_port);

				} else if(ipv4_header->next_proto_id == 17)
                {
                    udp_header = (struct udp_hdr *)((char *)m->buf_addr+m->data_off +sizeof(struct ether_hdr)+sizeof(struct ipv4_hdr));
					//udp_header = (struct udp_hdr *)rte_pktmbuf_adj(m, (uint16_t)sizeof(struct ipv4_hdr));
					// printf("Udp src_port:%d\n", ntohs(udp_header->src_port));
					// printf("Udp dst_port:%d\n", ntohs(udp_header->dst_port));
					temp_metadata.src_port = ntohs(udp_header->src_port);
					temp_metadata.dst_port = ntohs(udp_header->dst_port);

					key.src_port = ntohs(udp_header->src_port);
					key.dst_port = ntohs(udp_header->dst_port);
                }else
				{
					rte_pktmbuf_free(m);
					continue;
				}
				
				//temp_metadata.timestamp = rte_get_timer_cycles();
				
				temp_metadata.timestamp = time(NULL);
				char * ccc = "hello qjt----";
				memcpy(temp_metadata.str,ccc,strlen(ccc));

				//多线程插入无锁队列-----此数据将在完全k叉树中使用
				en_ring_queue(&temp_metadata);
				// void *msg;
				// if(rte_mempool_get(mydpdk_pktmbuf_pool, &msg)<0)
				// {
				// 	exit(-1);
				// }
				// en_queue(msg);
				// de_queue();

				//设置时间戳
				//struct timeval tv;
            	//gettimeofday(&tv, NULL);
				
				//将数据包类型转换为
				char* hash_pkt = rte_pktmbuf_mtod(pkts_burst[j],char*);

				//写入文件先按每个数据包级别写入，之后在研究怎么设置缓存写入
				unsigned char tt[20];
    			memcpy(tt,&key,sizeof(key));
				crc32_result = CRC32(0xFFFFFFFF,0x04C11DB7, tt, sizeof(tt), 0, 0);//crc32在1800万数据集下的冲突数是3万多


				//printf("%u\n",crc32_result);
				//file_name 34个字节--文件名字（需要设置缓存吗？还是每次直接进行映射写入）
				//sprintf(file_name,"%s_%s_%d.pcap",ip_transform(rte_be_to_cpu_32(key.src_ip)),ip_transform(rte_be_to_cpu_32(key.dst_ip)),crc32_result);
				//printf("%d\n",strlen(file_name));
				

				atomic_lock(&lock);
				if(hashmap_size(crc32_result)>=100)
				{
					//写入pcap文件中
					char* file_path_getcwd;
					file_path_getcwd=(char *)malloc(FILEPATH_MAX);
					getcwd(file_path_getcwd,FILEPATH_MAX);
					sprintf(file_name,"%s/%u.pcap",file_path_getcwd,crc32_result);

					pcap_dumper_t * dumper = pcap_dump_open(pcap_open_dead(1, 1600), file_name);//DLT_EN10MB--打开pcap文件
					for(int pcap_i =0;pcap_i<hashmap_size(crc32_result);pcap_i++)
					{
						// struct timeval tv;
						// gettimeofday(&tv,NULL);
						// printf("start write pcap\n");
						struct five_tuple_tmp temp_val = hashmap_val(crc32_result,pcap_i);
						
						dumpFile(dumper,(const u_char*)temp_val.val,temp_val.data_len,temp_val.tv_sec,temp_val.tv_usec);
					
						//printf("end write pcap\n");
					}
					pcap_dump_close(dumper);//关闭pcap文件
					//然后清空该key值下的数据
					hashmap_clear(crc32_result);
				}
				atomic_unlock(&lock);
				// atomic_lock(&lock);
				// //写入pcap文件中
				// char* file_path_getcwd;
				// file_path_getcwd=(char *)malloc(FILEPATH_MAX);
				// getcwd(file_path_getcwd,FILEPATH_MAX);
				// sprintf(file_name,"%s/%u.pcap",file_path_getcwd,crc32_result);
				// //printf("%s\n",file_name);
				// pcap_dumper_t * dumper = pcap_dump_open(pcap_open_dead(DLT_EN10MB, 1600), file_name);
		 		
				// struct timeval tv;
		 		// gettimeofday(&tv,NULL);
				
				// dumpFile(dumper,(const u_char*)hash_pkt,m->data_len,tv.tv_sec,tv.tv_usec);

				// //if(strlen(hash_pkt)==m->data_len) {printf("*******相等*****\n");}
				// printf("len:%d---%d\n",strlen(hash_pkt),m->data_len);
				// pcap_dump_close(dumper);
				
				
				
				struct five_tuple_tmp node;
				node.data_len = m->data_len;
				memset(node.val,'\0',sizeof(node.val));
				memcpy(node.val,hash_pkt,m->data_len);
				struct timeval tv;
		 		gettimeofday(&tv,NULL);
				node.tv_sec = tv.tv_sec;
				node.tv_usec = tv.tv_usec;
				atomic_lock(&lock);
				hashmap_insert(crc32_result,node);//插入到hash_map中
				atomic_unlock(&lock);


				
				//count++;
                rte_pktmbuf_free(m);//释放包
            }
		}
	}
}

static int
mydpdk_launch_one_lcore(__attribute__((unused)) void *dummy)
{
	mydpdk_main_loop();
	return 0;
}


static void index_core(void)
{
	unsigned lcore_id;
	lcore_id = rte_lcore_id();//获取当前lcore
	printf("MY_DPDK: entering main loop on marst core %u\n", lcore_id);
	while (!force_quit)
	{
		/* code */
		//printf("index\n");
	}
}

/* display usage */
static void
mydpdk_usage(const char *prgname)//dpdk参数的演示
{
	printf("%s [EAL options] -- -p PORTMASK [-q NQ]\n"
	       "  -p PORTMASK: hexadecimal bitmask of ports to configure\n"
	       "  -q NQ: number of queue (=ports) per lcore (default is 1)\n"
		   "  -T PERIOD: statistics will be refreshed each PERIOD seconds (0 to disable, 10 default, 86400 maximum)\n"
		   "  --[no-]mac-updating: Enable or disable MAC addresses updating (enabled by default)\n"
		   "      When enabled:\n"
		   "       - The source MAC address is replaced by the TX port MAC address\n"
		   "       - The destination MAC address is replaced by 02:00:00:00:00:TX_PORT_ID\n",
	       prgname);
}

static int
parse_portmask(const char *portmask)
{
	char *end = NULL;
	unsigned long pm;

	/* parse hexadecimal string */
	//解析输入参数中的端口16进制掩码
	pm = strtoul(portmask, &end, 16);
	if ((portmask[0] == '\0') || (end == NULL) || (*end != '\0'))
		return -1;

	if (pm == 0)
		return -1;

	return pm;
}

static unsigned int
parse_nqueue(const char *q_arg)
{
	char *end = NULL;
	unsigned long n;

	/* parse hexadecimal string */
	n = strtoul(q_arg, &end, 10);
	if ((q_arg[0] == '\0') || (end == NULL) || (*end != '\0'))
		return 0;
	if (n == 0)
		return 0;
	if (n >= MAX_RX_QUEUE_PER_LCORE)
		return 0;

	return n;
}

static int
parse_timer_period(const char *q_arg)
{
	char *end = NULL;
	int n;

	/* parse number string */
	n = strtol(q_arg, &end, 10);
	if ((q_arg[0] == '\0') || (end == NULL) || (*end != '\0'))
		return -1;
	if (n >= MAX_TIMER_PERIOD)
		return -1;

	return n;
}

static const char short_options[] =
	"p:"  /* portmask */
	"q:"  /* number of queues */
	"T:"  /* timer period */
	;

// #define CMD_LINE_OPT_MAC_UPDATING "mac-updating"
// #define CMD_LINE_OPT_NO_MAC_UPDATING "no-mac-updating"

enum {
	/* long options mapped to a short option */

	/* first long only option value must be >= 256, so that we won't
	 * conflict with short options */
	CMD_LINE_OPT_MIN_NUM = 256,
};

static const struct option lgopts[] = {
	// { CMD_LINE_OPT_MAC_UPDATING, no_argument, &mac_updating, 1},
	// { CMD_LINE_OPT_NO_MAC_UPDATING, no_argument, &mac_updating, 0},
	{NULL, 0, 0, 0}
};

/* Parse the argument given in the command line of the application */
static int
parse_args(int argc, char **argv)
{
	int opt, ret, timer_secs;
	char **argvopt;
	int option_index;
	char *prgname = argv[0];//程序名

	argvopt = argv;

	while ((opt = getopt_long(argc, argvopt, short_options,
				  lgopts, &option_index)) != EOF) {

		switch (opt) {
		/* portmask */
		case 'p':
			mydpdk_enabled_port_mask = parse_portmask(optarg);
			if (mydpdk_enabled_port_mask == 0) {
				printf("invalid portmask\n");
				mydpdk_usage(prgname);
				return -1;
			}
			break;

		/* nqueue */
		case 'q':
			mydpdk_rx_queue_per_lcore = parse_nqueue(optarg);
			if (mydpdk_rx_queue_per_lcore == 0) {
				printf("invalid queue number\n");
				mydpdk_usage(prgname);
				return -1;
			}
			break;

		/* timer period */
		case 'T':
			timer_secs = parse_timer_period(optarg);
			if (timer_secs < 0) {
				printf("invalid timer period\n");
				mydpdk_usage(prgname);
				return -1;
			}
			timer_period = timer_secs;
			break;

		/* long options */
		case 0:
			break;

		default:
			mydpdk_usage(prgname);
			return -1;
		}
	}

	if (optind >= 0)
		argv[optind-1] = prgname;

	ret = optind-1;
	optind = 1; /* reset getopt lib */
	return ret;
}

/* Check the link status of all ports in up to 9s, and print them finally */
// static void
// check_all_ports_link_status(uint32_t port_mask)
// {
// #define CHECK_INTERVAL 100 /* 100ms */
// #define MAX_CHECK_TIME 90 /* 9s (90 * 100ms) in total */
// 	uint16_t portid;
// 	uint8_t count, all_ports_up, print_flag = 0;
// 	struct rte_eth_link link;

// 	printf("\nChecking link status");
// 	fflush(stdout);
// 	for (count = 0; count <= MAX_CHECK_TIME; count++) {
// 		if (force_quit)
// 			return;
// 		all_ports_up = 1;
// 		RTE_ETH_FOREACH_DEV(portid) {
// 			if (force_quit)
// 				return;
// 			if ((port_mask & (1 << portid)) == 0)
// 				continue;
// 			memset(&link, 0, sizeof(link));
// 			rte_eth_link_get_nowait(portid, &link);
// 			/* print link status if flag set */
// 			if (print_flag == 1) {
// 				if (link.link_status)
// 					printf(
// 					"Port%d Link Up. Speed %u Mbps - %s\n",
// 						portid, link.link_speed,
// 				(link.link_duplex == ETH_LINK_FULL_DUPLEX) ?
// 					("full-duplex") : ("half-duplex\n"));
// 				else
// 					printf("Port %d Link Down\n", portid);
// 				continue;
// 			}
// 			/* clear all_ports_up flag if any link down */
// 			if (link.link_status == ETH_LINK_DOWN) {
// 				all_ports_up = 0;
// 				break;
// 			}
// 		}
// 		/* after finally printing all link status, get out */
// 		if (print_flag == 1)
// 			break;

// 		if (all_ports_up == 0) {
// 			printf(".");
// 			fflush(stdout);
// 			rte_delay_ms(CHECK_INTERVAL);
// 		}

// 		/* set the print_flag if all ports up or timeout */
// 		if (all_ports_up == 1 || count == (MAX_CHECK_TIME - 1)) {
// 			print_flag = 1;
// 			printf("done\n");
// 		}
// 	}
// }

static void
signal_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n",
				signum);
		force_quit = true;
	}
}

int
my_spdk_env_init(const struct spdk_env_opts *opts)
{
	
	int i, rc;
	bool legacy_mem;
 	legacy_mem = false;

	if (g_external_init == false) {
		if (opts != NULL) {
			fprintf(stderr, "Invalid arguments to reinitialize SPDK env\n");
			return -EINVAL;
		}

		//printf("Starting %s / %s reinitialization...\n", SPDK_VERSION_STRING, rte_version());
		printf("Staarting reinitialization SPDK\n");
		pci_env_reinit();

		return 0;
	}

	if (opts->env_context && strstr(opts->env_context, "--legacy-mem") != NULL) {
		legacy_mem = true;
	}

	rc = spdk_env_dpdk_post_init(legacy_mem);
	if (rc == 0) {
		g_external_init = false;
	}
	return rc;
}




int
main(int argc, char **argv)
{
	struct lcore_queue_conf *qconf;
	int ret;
	uint16_t nb_ports;
	uint16_t nb_ports_available = 0;
	uint16_t portid, last_port;
	unsigned lcore_id, rx_lcore_id;
	unsigned nb_ports_in_mask = 0;
	unsigned int nb_lcores = 0;
	unsigned int nb_mbufs;
//printf("%d,%d\n",sizeof(struct metadata_info),sizeof(struct five_tuple));

	c=cuckoo_init(100);
	/* init EAL */
	ret = rte_eal_init(argc, argv);
	
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");
	argc -= ret;
	argv += ret;
	/***************************SPDK初始化开始******************************/
	int rc;
	struct spdk_env_opts opts;
	spdk_env_opts_init(&opts);//初始化操作
	opts.name = "my_dpdk";
	opts.shm_id = 0;
	//if (spdk_env_init(&opts) < 0) {//初始化环境-------需要修改
	if(my_spdk_env_init(&opts)<0){
		fprintf(stderr, "Unable to initialize SPDK env\n");
		return 1;
	}
	printf("Initializing NVMe Controllers\n");

	if (g_vmd && spdk_vmd_init()) {//vmd初始化卷管理设备
		fprintf(stderr, "Failed to initialize VMD."
			" Some NVMe devices can be unavailable.\n");
	}
	/*
	单线程,根据需要将用户空间NVME驱动程序附加到找到的每个设备上，既是将找到的nvme控制器选择附加到用户空间驱动程序
	*/
	rc = spdk_nvme_probe(NULL, NULL, probe_cb, attach_cb, NULL);
	//probe_cb 为找到的每个NVME控制器
	if (rc != 0) {
		fprintf(stderr, "spdk_nvme_probe() failed\n");
		cleanup();
		return 1;
	}

	if (TAILQ_EMPTY(&g_controllers)) {//判断控制器尾队列是否为空
		fprintf(stderr, "no NVMe controllers found\n");
		cleanup();
		return 1;
	}
	printf("SPDK Initialization complete.\n");

	//hello_world();
	/***************************SPDK初始化结束******************************/


	force_quit = false;
	signal(SIGINT, signal_handler);//信号处理,强制退出
	signal(SIGTERM, signal_handler);

	/* parse application arguments (after the EAL ones) */
	ret = parse_args(argc, argv);//解析参数进行设置
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid dpdk arguments\n");

	//printf("MAC updating %s\n", mac_updating ? "enabled" : "disabled");//?????

	/* convert to number of cycles */
	timer_period *= rte_get_timer_hz();//获取一秒内的循环数

	nb_ports = rte_eth_dev_count_avail();//获取可用的端口数
	printf("port num:%d\n",nb_ports);
	if (nb_ports == 0)
		rte_exit(EXIT_FAILURE, "No Ethernet ports - bye\n");

	/* check port mask to possible port mask */
	if (mydpdk_enabled_port_mask & ~((1 << nb_ports) - 1))//~取反,推荐出最大的端口数
		rte_exit(EXIT_FAILURE, "Invalid portmask; possible (0x%x)\n",
			(1 << nb_ports) - 1);

	/* reset mydpdk_dst_ports 重置所有端口 */
	for (portid = 0; portid < RTE_MAX_ETHPORTS; portid++)
		mydpdk_dst_ports[portid] = 0;
	last_port = 0;

	/*
	 * Each logical core is assigned a dedicated TX queue on each port.
	 */
	RTE_ETH_FOREACH_DEV(portid) {//为每个核上的端口分配转发队列
		/* skip ports that are not enabled */
		if ((mydpdk_enabled_port_mask & (1 << portid)) == 0)
			continue;

		if (nb_ports_in_mask % 2) {
			mydpdk_dst_ports[portid] = last_port;
			mydpdk_dst_ports[last_port] = portid;
		}
		else
			last_port = portid;

		nb_ports_in_mask++;
	}
	if (nb_ports_in_mask % 2) {//最后一个奇数端口的配置方法
		printf("Notice: odd number of ports in portmask.\n");
		mydpdk_dst_ports[last_port] = last_port;
	}

	rx_lcore_id = 0;
	qconf = NULL;

	/* Initialize the port/queue configuration of each logical core */
	RTE_ETH_FOREACH_DEV(portid) {
		/* skip ports that are not enabled */
		if ((mydpdk_enabled_port_mask & (1 << portid)) == 0)
			continue;

		/* get the lcore_id for this port 找到对于端口可用的core */
		while (rte_lcore_is_enabled(rx_lcore_id) == 0 ||
		       lcore_queue_conf[rx_lcore_id].n_rx_port ==
		       mydpdk_rx_queue_per_lcore) {
			rx_lcore_id++;
			if (rx_lcore_id >= RTE_MAX_LCORE)
				rte_exit(EXIT_FAILURE, "Not enough cores\n");
		}

		if (qconf != &lcore_queue_conf[rx_lcore_id]) {//对此端口可用的core进行配置
			/* Assigned a new logical core in the loop above. */
			qconf = &lcore_queue_conf[rx_lcore_id];
			nb_lcores++;
		}

		qconf->rx_port_list[qconf->n_rx_port] = portid;
		qconf->n_rx_port++;
		printf("Lcore %u: RX port %u\n", rx_lcore_id, portid);
	}

	nb_mbufs = RTE_MAX(nb_ports * (nb_rxd + nb_txd + MAX_PKT_BURST +
		nb_lcores * MEMPOOL_CACHE_SIZE), 8192U);//创建的内存大小，最小为8192U，MEMPOOL_CACHE_SIZE是每core缓冲区

	/* create the mbuf pool */
	mydpdk_pktmbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", nb_mbufs,
		MEMPOOL_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
		rte_socket_id());//设置缓存是为了向内存池申请空间是，首先从每个cpu缓存查找是否存在空闲的对象元素，无则从ring队列中取，
		//维护的是应用层为cpu准备的缓存,从而减少多个cpu同时访问内存池上的元素，减少竞争
	if (mydpdk_pktmbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");

	spdk_ring = rte_ring_create("message_ring",
            ring_size, rte_socket_id(), 0x0008 | 0x0010);

	/* Initialise each port 初始化每个端口 */
	RTE_ETH_FOREACH_DEV(portid) {
		struct rte_eth_rxconf rxq_conf;
		//struct rte_eth_txconf txq_conf;
		struct rte_eth_conf local_port_conf = port_conf;
		struct rte_eth_dev_info dev_info;

		/* skip ports that are not enabled */
		if ((mydpdk_enabled_port_mask & (1 << portid)) == 0) {
			printf("Skipping disabled port %u\n", portid);
			continue;
		}
		nb_ports_available++;

		/* init port */
		printf("Initializing port %u... ", portid);
		fflush(stdout);
		rte_eth_dev_info_get(portid, &dev_info);//获取端口设备的信息
		if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
			local_port_conf.txmode.offloads |=
				DEV_TX_OFFLOAD_MBUF_FAST_FREE;
		ret = rte_eth_dev_configure(portid, 1, 1, &local_port_conf);//对端口进行配置，rx和tx
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%u\n",
				  ret, portid);

		ret = rte_eth_dev_adjust_nb_rx_tx_desc(portid, &nb_rxd,
						       &nb_txd);//检查端口设备中的rx和tx的desc是否满足要求，否则调整为边界值
		if (ret < 0)
			rte_exit(EXIT_FAILURE,
				 "Cannot adjust number of descriptors: err=%d, port=%u\n",
				 ret, portid);

		//根据端口号获取到端口的mac地址，放在mydpdk_ports_eth_addr[portid]
		rte_eth_macaddr_get(portid,&mydpdk_ports_eth_addr[portid]);

		/* init one RX queue 初始化每个端口上的一个接收队列的处理 */
		fflush(stdout);
		rxq_conf = dev_info.default_rxconf;
		rxq_conf.offloads = local_port_conf.rxmode.offloads;
		ret = rte_eth_rx_queue_setup(portid, 0, nb_rxd,//为端口设置接收队列并分配空间
					     rte_eth_dev_socket_id(portid),//为与socket_id关联的port的rx描述符分配空间
					     &rxq_conf,
					     mydpdk_pktmbuf_pool);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup:err=%d, port=%u\n",
				  ret, portid);

		/* init one TX queue on each port 初始化每个端口上的一个转发队列 */
		//fflush(stdout);
		// txq_conf = dev_info.default_txconf;
		// txq_conf.offloads = local_port_conf.txmode.offloads;
		// ret = rte_eth_tx_queue_setup(portid, 0, nb_txd,
		// 		rte_eth_dev_socket_id(portid),
		// 		&txq_conf);
		// if (ret < 0)
		// 	rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup:err=%d, port=%u\n",
		// 		ret, portid);

		// /* Initialize TX buffers 为一个tx队列分配内存空间 */
		// tx_buffer[portid] = rte_zmalloc_socket("tx_buffer",
		// 		RTE_ETH_TX_BUFFER_SIZE(MAX_PKT_BURST), 0,
		// 		rte_eth_dev_socket_id(portid));//类似于rte_malloc()
		// if (tx_buffer[portid] == NULL)
		// 	rte_exit(EXIT_FAILURE, "Cannot allocate buffer for tx on port %u\n",
		// 			portid);

		// rte_eth_tx_buffer_init(tx_buffer[portid], MAX_PKT_BURST);//初始化一个转发队列的buffer
		// //功能：配置无法发送的缓冲数据包的回调
		// ret = rte_eth_tx_buffer_set_err_callback(tx_buffer[portid],
		// 		rte_eth_tx_buffer_count_callback,//对于未转发出去的数据包处理的函数，此处为释放和统计函数
		// 		&port_statistics[portid].dropped);//port_statistics[portid].dropped 端口为发送出的数量
		// if (ret < 0)
		// 	rte_exit(EXIT_FAILURE,
		// 	"Cannot set error callback for tx buffer on port %u\n",
		// 		 portid);

		/* Start device 启动此端口 */
		ret = rte_eth_dev_start(portid);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "rte_eth_dev_start:err=%d, port=%u\n",
				  ret, portid);

		printf("done: \n");

		//设置端口为混杂模式
		rte_eth_promiscuous_enable(portid);

		printf("Port %u, MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n\n",
				portid,
				mydpdk_ports_eth_addr[portid].addr_bytes[0],
				mydpdk_ports_eth_addr[portid].addr_bytes[1],
				mydpdk_ports_eth_addr[portid].addr_bytes[2],
				mydpdk_ports_eth_addr[portid].addr_bytes[3],
				mydpdk_ports_eth_addr[portid].addr_bytes[4],
				mydpdk_ports_eth_addr[portid].addr_bytes[5]);

		/* initialize port stats */
		//初始化端口接收和发送统计的数为0
		//memset(&port_statistics, 0, sizeof(port_statistics));
	}

	if (!nb_ports_available) {
		rte_exit(EXIT_FAILURE,
			"All available ports are disabled. Please set portmask.\n");
	}
	//最多检查9s中所有端口的链接状态，并最终打印它们
	//check_all_ports_link_status(mydpdk_enabled_port_mask);

	ret = 0;
	/* launch per-lcore init on every lcore */
	//在所有核上启动一个函数，（CALL_MASTER，包括主核，SKIP_MASTER 不包括主核）
	rte_eal_mp_remote_launch(mydpdk_launch_one_lcore, NULL, SKIP_MASTER);//CALL_MASTER SKIP_MASTER
	//index_core();//主函数用来处理包级别索引  使用完全k叉树，写进SPDK
	printf("start -----spdk\n");
	//hello_world();
	spdk_write();
	RTE_LCORE_FOREACH_SLAVE(lcore_id) {
		if (rte_eal_wait_lcore(lcore_id) < 0) {
			ret = -1;
			break;
		}
	}
	
	
	// lcore_id =1;

	// RTE_LCORE_FOREACH_SLAVE(lcore_id)
	// {
	// 	printf("%d\n",lcore_id);
	// 	rte_eal_mp_remote_launch(mydpdk_launch_one_lcore, NULL, CALL_MASTER);
	// }
	
	// rte_eal_mp_wait_lcore();
	
	RTE_ETH_FOREACH_DEV(portid) {
		if ((mydpdk_enabled_port_mask & (1 << portid)) == 0)
			continue;
		printf("Closing port %d...", portid);
		rte_eth_dev_stop(portid);
		rte_eth_dev_close(portid);
		printf(" Done\n");
	}
	cleanup();
	if (g_vmd) {
		spdk_vmd_fini();
	}
	printf("Bye...\n");

	return ret;
}
