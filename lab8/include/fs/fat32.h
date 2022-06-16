#ifndef FAT32_H
#define FAT32_H

#include "stddef.h"
#include "fs/vfs.h"

#define FILE_NAME_MAX 16
#define MAX_DIR_ENTRY 17
#define MAX_FILE_SIZE 4096

struct fat32_inode
{
    enum node_type type;
    char name[FILE_NAME_MAX];
    struct vnode *entry[MAX_DIR_ENTRY];
    char *data;
    size_t datasize;
};

int register_fat32();
int fat32_setup_mount(struct filesystem *fs, struct mount *_mount);

int fat32_write(struct file *file, const void *buf, size_t len);
int fat32_read(struct file *file, void *buf, size_t len);
int fat32_open(struct vnode *file_node, struct file **target);
int fat32_close(struct file *file);
long fat32_getsize(struct vnode *vd);

int fat32_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int fat32_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int fat32_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);

struct vnode *fat32_create_vnode(struct mount *_mount, enum node_type type);

#endif