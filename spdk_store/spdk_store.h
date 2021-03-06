#include "spdk/stdinc.h"
#include "spdk/nvme.h"
#include "spdk/vmd.h"
#include "spdk/nvme_zns.h"
#include "spdk/env.h"
#include <sys/time.h>

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


//��������Ŀ�ṹ
struct ctrlr_entry {
	struct spdk_nvme_ctrlr		*ctrlr;
	TAILQ_ENTRY(ctrlr_entry)	link;//β����
	char				name[1024];
};
//�����ռ���Ŀ�ṹ
struct ns_entry {
	struct spdk_nvme_ctrlr	*ctrlr;
	struct spdk_nvme_ns	*ns;
	TAILQ_ENTRY(ns_entry)	link;//β����
	struct spdk_nvme_qpair	*qpair;
};

// static TAILQ_HEAD(, ctrlr_entry) g_controllers = TAILQ_HEAD_INITIALIZER(g_controllers);//��ʼ������������ͷ��
// static TAILQ_HEAD(, ns_entry) g_namespaces = TAILQ_HEAD_INITIALIZER(g_namespaces);//��ʼ��namespace����ͷ��

// static bool g_vmd = false;//�Ƿ�ʹ��vmd(������豸)
// static bool g_external_init = true;

//ע�������ռ亯��
static void
register_ns(struct spdk_nvme_ctrlr *ctrlr, struct spdk_nvme_ns *ns);

struct hello_world_sequence {
	struct ns_entry	*ns_entry;
	char		*buf;
	unsigned        using_cmb_io;
	int		is_completed;
};

static void
read_complete(void *arg, const struct spdk_nvme_cpl *completion);

static void
write_complete(void *arg, const struct spdk_nvme_cpl *completion);

static void
reset_zone_complete(void *arg, const struct spdk_nvme_cpl *completion);

static void
reset_zone_and_wait_for_completion(struct hello_world_sequence *sequence);

static void
hello_world(void);


static bool
probe_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
	 struct spdk_nvme_ctrlr_opts *opts);

static void
attach_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
	  struct spdk_nvme_ctrlr *ctrlr, const struct spdk_nvme_ctrlr_opts *opts);

static void
cleanup(void);

static int
my_spdk_env_init(const struct spdk_env_opts *opts);

static int
my_spdk_all_init(const struct spdk_env_opts *opts);