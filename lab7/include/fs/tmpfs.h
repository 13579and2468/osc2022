#ifndef TMPFS_H
#define TMPFS_H

#include "stddef.h"
#include "vfs.h"

#define FILE_NAME_MAX 16
#define MAX_DIR_ENTRY 17
#define MAX_FILE_SIZE 4096

enum tmpfs_type
{
    dir_t,
    file_t
};

struct tmpfs_inode
{
    enum tmpfs_type type;
    char name[FILE_NAME_MAX];
    struct vnode *entry[MAX_DIR_ENTRY];
    char *data;
    size_t datasize;
};

int register_tmpfs();
int tmpfs_setup_mount(struct filesystem *fs, struct mount *_mount);

int tmpfs_write(struct file *file, const void *buf, size_t len);
int tmpfs_read(struct file *file, void *buf, size_t len);
int tmpfs_open(struct vnode *file_node, struct file **target);
int tmpfs_close(struct file *file);
long tmpfs_lseek64(struct file *file, long offset, int whence);

int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);

struct vnode *tmpfs_create_vnode(struct mount *_mount, enum tmpfs_type type);

#endif