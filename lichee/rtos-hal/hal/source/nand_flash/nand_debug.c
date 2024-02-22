/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY'S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#ifdef CONFIG_KERNEL_FREERTOS
#include <awlog.h>
#elif defined(CONFIG_OS_MELIS)
#include <log.h>
#endif
#include <aw_common.h>

#include <nand_inc.h>
#include <nand_phy.h>

#ifdef CONFIG_COMPONENTS_AW_DEVFS
#include <devfs.h>
#endif

typedef int (*show_op)(void *buf, unsigned int len);
typedef int (*store_op)(const void *buf, unsigned int len);

struct nand_dbg_node {
#define NAND_DBG_NODE_NAME_LEN 32
	char name[NAND_DBG_NODE_NAME_LEN];
	char *buf;
	unsigned int bytes;
	show_op show;
	store_op store;
	struct nand_dbg_node *next;
	void *private;
};

struct nand_dbg {
	struct nand_dbg_node *head;
	struct _nftl_blk *blk;
	int removed;
} ndbg;

#define min_t(t, x, y) ((t)(x) > (t)(y) ? (y) : (x))

#ifdef CONFIG_OS_ALIOSTHINGS
#define file_to_node(file) ((file)->node->i_arg)
static int nand_dbg_open(inode_t *node, file_t *f)
{
	return 0;
}

static ssize_t nand_dbg_read(file_t *f, void *buf, size_t nbytes)
{
	struct nand_dbg_node *node = file_to_node(f);

	if (!node)
		return -EIO;
	if (ndbg.removed)
		return -EBUSY;
	if (!node->show)
		return -EBUSY;

	if (!node->buf) {
		node->buf = malloc(4096);
		if (!node->buf)
			return -ENOMEM;
		node->bytes = node->show(node->buf, 4096);
		if (node->bytes < 0)
			goto free;
	}
	if (f->offset > node->bytes)
		goto free;

	nbytes = min_t(unsigned int, nbytes, node->bytes - f->offset);
	memcpy(buf, node->buf + f->offset, nbytes);
	f->offset += nbytes;
	return nbytes;
free:
	free(node->buf);
	return -EIO;
}

static ssize_t nand_dbg_write(file_t *f, const void *buf, size_t nbytes)
{
	struct nand_dbg_node *node = file_to_node(f);

	if (!node)
		return -EIO;
	if (ndbg.removed)
		return -EBUSY;
	if (!node->store)
		return -EBUSY;

	return node->store(buf, nbytes);
}

static int nand_dbg_close(file_t *f)
{
	struct nand_dbg_node *node = file_to_node(f);

	if (ndbg.removed)
		return -EBUSY;
	if (node->buf) {
		free(node->buf);
		node->buf = NULL;
		node->bytes = 0;
	}
	return 0;
}

static uint32_t nand_dbg_lseek(file_t *f, int64_t off, int32_t whence)
{
	struct nand_dbg_node *node = file_to_node(f);

	if (ndbg.removed)
		return -EBUSY;

	switch (whence) {
	case SEEK_CUR:
	    off = f->offset + off;
	    break;
	case SEEK_END:
	    off = node->bytes - 1 + off;
	    break;
	case SEEK_SET:
	    break;
	default: return -EINVAL;
	}

	if (off >= node->bytes || off < 0)
		return -EINVAL;

	f->offset = off;
	return 0;
}

static int nand_dbg_stat(file_t *f, const char *path, struct aos_stat *st)
{
	if (ndbg.removed)
		return -EBUSY;

	st->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
	st->st_size = 0;
	st->st_actime = st->st_modtime = 0;
	return 0;
}

static file_ops_t nand_dbg_fops = {
	.open = nand_dbg_open,
	.read = nand_dbg_read,
	.write = nand_dbg_write,
	.close = nand_dbg_close,
	.lseek = nand_dbg_lseek,
	.stat = nand_dbg_stat,
};

static int nand_dbg_register_os_node(struct nand_dbg_node *node)
{
	pr_info("register %s to aos\n", node->name);
	return aos_register_driver(node->name, &nand_dbg_fops, (void *)node);
}

static int nand_dbg_unregister_os_node(struct nand_dbg_node *node)
{
	return aos_unregister_driver(node->name);
}
#elif defined(CONFIG_KERNEL_FREERTOS)

#ifdef CONFIG_COMPONENTS_AW_DEVFS
static ssize_t nand_dbg_read(struct devfs_node *node, uint32_t offset,
        uint32_t size, void *data)
{
	if (size == 0)
		return 0;

	struct nand_dbg_node *dbg_node = (struct nand_dbg_node *)node->private;
	if (!dbg_node)
		return -EIO;
	if (ndbg.removed)
		return -EBUSY;
	if (!dbg_node->show)
		return -EBUSY;

	if (!dbg_node->buf) {
		dbg_node->buf = malloc(4096);
		if (!dbg_node->buf)
			return -ENOMEM;
		dbg_node->bytes = dbg_node->show(dbg_node->buf, 4096);
		if (dbg_node->bytes < 0)
			goto free;
	}
	if (offset > dbg_node->bytes)
		goto free;

	size = min_t(unsigned int, size, dbg_node->bytes - offset);
	memcpy(data, dbg_node->buf + offset, size);
	return size;
free:
	free(dbg_node->buf);
	return -EIO;
}

static ssize_t nand_dbg_write(struct devfs_node *node, uint32_t offset,
        uint32_t size, void *data)
{
	if (size == 0)
		return 0;

	struct nand_dbg_node *dbg_node = (struct nand_dbg_node *)node->private;
	if (!dbg_node)
		return -EIO;
	if (ndbg.removed)
		return -EBUSY;
	if (!dbg_node->store)
		return -EBUSY;

	return dbg_node->store(data, size);
}
#endif

static int nand_dbg_register_os_node(struct nand_dbg_node *node)
{
	int ret = 0;
#ifdef CONFIG_COMPONENTS_AW_DEVFS
	pr_info("register %s to devfs\n", node->name);
    struct devfs_node *dev_node = malloc(sizeof(struct devfs_node));
	if (dev_node) {
		memset(dev_node, 0, sizeof(struct devfs_node));
		dev_node->name = node->name;
		dev_node->alias = node->name;
		dev_node->size = 4096;
		dev_node->write = (void *)nand_dbg_write;
		dev_node->read = (void *)nand_dbg_read;
		dev_node->private = node;
		node->private = dev_node;
		ret = devfs_add_node(dev_node);
	}
#endif
	return ret;
}

static int nand_dbg_unregister_os_node(struct nand_dbg_node *node)
{
#ifdef CONFIG_COMPONENTS_AW_DEVFS
	devfs_del_node(node->private);
#endif
	return 0;
}

#else
static int nand_dbg_register_os_node(struct nand_dbg_node *node)
{
	return 0;
}

static int nand_dbg_unregister_os_node(struct nand_dbg_node *node)
{
	return 0;
}
#endif

static void nand_dbg_add_list(struct nand_dbg_node *node)
{
	if (!ndbg.head) {
		ndbg.head = node;
	} else {
		node->next = ndbg.head;
		ndbg.head = node;
	}
}

static int nand_dbg_add_node(const char *name, show_op show, store_op store)
{
	int ret;
	struct nand_dbg_node *node;

	if (name[0] == '\0')
		return -EINVAL;

	node = malloc(sizeof(*node));
	if (!node)
		return -ENOMEM;

#ifdef CONFIG_OS_ALIOSTHINGS
	snprintf(node->name, NAND_DBG_NODE_NAME_LEN, "/dev/nand_debug/%s", name);
#else
	snprintf(node->name, NAND_DBG_NODE_NAME_LEN, "nand_debug_%s", name);
#endif
	node->show = show;
	node->store = store;
	node->buf = NULL;
	node->next = NULL;
	node->bytes = 0;

	ret = nand_dbg_register_os_node(node);
	if (ret)
		free(node);
	else
		nand_dbg_add_list(node);
	return ret;
}

static void nand_dbg_del_node(struct nand_dbg_node *node)
{
	nand_dbg_unregister_os_node(node);
	free(node);
}

static void nand_dbg_del_all_nodes(void)
{
	while (ndbg.head) {
		struct nand_dbg_node *next = ndbg.head->next;

		nand_dbg_del_node(ndbg.head);
		ndbg.head = next;
	}
}

static int nand_dbg_show_arch(void *buf, unsigned int len)
{
	return PHY_GetArchInfo_Str(buf);
}

static int nand_dbg_show_gcinfo(void *buf, unsigned int len)
{
	return ndbg.blk->gc_stat(ndbg.blk, buf, 4096);
}

static int nand_dbg_show_badblock(void *buf, unsigned int len)
{
	return sprintf(buf, "cnt: %d\n", ndbg.blk->badblk(ndbg.blk));
}

static int nand_dbg_show_version(void *buf, unsigned int len)
{
	return snprintf(buf, len, "nftl: %u.%u.%u\n",
			ndbg.blk->ver_main,
			ndbg.blk->ver_mid,
			ndbg.blk->ver_sub);
}

int nand_dbg_init(struct _nftl_blk *blk)
{
	ndbg.blk = blk;
	ndbg.removed = 0;
	ndbg.head = NULL;

#define add_node(name, show, store) {				\
	if (nand_dbg_add_node(name, show, store)) {			\
		pr_err("add %s for nand debug failed\n", name); \
		goto err;					\
	}							\
}
	add_node("arch", nand_dbg_show_arch, NULL);
	add_node("gcinfo", nand_dbg_show_gcinfo, NULL);
	add_node("badblock", nand_dbg_show_badblock, NULL);
	add_node("version", nand_dbg_show_version, NULL);
#undef add_node

	pr_info("nand debug init OK\n");
	return 0;
err:
	return -EIO;
}

void nand_dbg_exit(void)
{
	nand_dbg_del_all_nodes();
	ndbg.blk = NULL;
	ndbg.removed = 1;
}
