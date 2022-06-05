#include "fs/vfs.h"
#include "fs/tmpfs.h"
#include "malloc.h"
#include "string.h"

int register_filesystem(struct filesystem *fs)
{
    for (int i = 0; i < MAX_FS_REG;i++)
    {
        if(!reg_fs[i].name)
        {
            reg_fs[i].name = fs->name;
            reg_fs->setup_mount = fs->setup_mount;
            return i;
        }
    }
    return -1;
}

struct filesystem* find_filesystem(const char* fs_name)
{
    for (int i = 0; i < MAX_FS_REG; i++)
    {
        if (strcmp(reg_fs[i].name,fs_name)==0)
        {
            return &reg_fs[i];
        }
    }
    return 0;
}

int vfs_open(const char *pathname, int flags, struct file **target)
{
    // 1. Lookup pathname
    // 3. Create a new file if O_CREAT is specified in flags and vnode not found
    struct vnode *node;
    if (vfs_lookup(pathname, &node) != 0 && (flags&O_CREAT))
    {
        int last_slash_idx = 0;
        for (int i = 0; i < strlen(pathname); i++)
        {
            if(pathname[i]=='/')
            {
                last_slash_idx = i;
            }
        }

        char dirname[MAX_PATH_NAME+1];
        strcpy(dirname, pathname);
        dirname[last_slash_idx] = '/';
        if (vfs_lookup(dirname,&node)!=0)
        {
            uart_printf("cannot ocreate no dir name\r\n");
            return -1;
        }
        node->v_ops->create(node, &node, pathname+last_slash_idx+1);
        *target = kmalloc(sizeof(struct file));
        node->f_ops->open(node, target);
        (*target)->flags = flags;
        return 0;
    }
    else // 2. Create a new file handle for this vnode if found.
    {
        *target = kmalloc(sizeof(struct file));
        node->f_ops->open(node, target);
        (*target)->flags = flags;
        return 0;
    }

    // lookup error code shows if file exist or not or other error occurs
    // 4. Return error code if fails
    return -1;
}

int vfs_close(struct file *file)
{
    // 1. release the file handle
    // 2. Return error code if fails
    file->f_ops->close(file);
    return 0;
}

int vfs_write(struct file *file, const void *buf, size_t len)
{
    // 1. write len byte from buf to the opened file.
    // 2. return written size or error code if an error occurs.
    return file->f_ops->write(file,buf,len);
}

int vfs_read(struct file *file, void *buf, size_t len)
{
    // 1. read min(len, readable size) byte to buf from the opened file.
    // 2. block if nothing to read for FIFO type
    // 2. return read size or error code if an error occurs.
    return file->f_ops->read(file, buf, len);
}

int vfs_mkdir(const char *pathname)
{
    char dirname[MAX_PATH_NAME] = {};
    char newdirname[MAX_PATH_NAME] = {};

    int last_slash_idx = 0;
    for (int i = 0; i < strlen(pathname); i++)
    {
        if (pathname[i] == '/')
        {
            last_slash_idx = i;
        }
    }

    memcpy(dirname, pathname, last_slash_idx);
    strcpy(newdirname, pathname + last_slash_idx + 1);

    struct vnode *node;
    if(vfs_lookup(dirname,&node)==0)
    {
        node->v_ops->mkdir(node,&node,newdirname);
        return 0;
    }

    uart_printf("vfs_mkdir cannot find pathname");
    return -1;
}

int vfs_mount(const char *target, const char *filesystem)
{
    struct vnode *dirnode;
    struct filesystem *fs = find_filesystem(filesystem);
    if(!fs)
    {
        uart_printf("vfs_mount cannot find filesystem\r\n");
        return -1;
    }

    if(vfs_lookup(target, &dirnode)==-1)
    {
        uart_printf("vfs_mount cannot find dir\r\n");
        return -1;
    }else
    {
        dirnode->mount->fs->setup_mount(fs,dirnode->mount);
    }
    return 0;
}

int vfs_lookup(const char *pathname, struct vnode **target)
{
    struct vnode *dirnode = rootfs->root;
    char component_name[FILE_NAME_MAX+1] = {};
    int c_idx = 0;
    for (int i = 1; i < strlen(pathname); i++)
    {
        if (pathname[i] == '/')
        {
            component_name[c_idx++] = 0;
            struct vnode *prev_root = dirnode->mount->root;
            if (dirnode->v_ops->lookup(dirnode, &dirnode, component_name) != 0)return -1;
            // redirect to new mounted filesystem
            if (dirnode->mount->root != prev_root)
            {
                dirnode = dirnode->mount->root;
            }
            c_idx = 0;
        }
        else
        {
            component_name[c_idx++] = pathname[i];
        }
    }

    component_name[c_idx++] = 0;
    struct vnode *prev_root = dirnode->mount->root;
    if (dirnode->v_ops->lookup(dirnode, &dirnode, component_name) != 0)return -1;
    // redirect to new mounted filesystem
    if (dirnode->mount->root != prev_root)
    {
        dirnode = dirnode->mount->root;
    }

    *target = dirnode;

    return 0;
}

void init_rootfs()
{
    int idx = register_tmpfs();
    rootfs = kmalloc(sizeof(struct mount));
    reg_fs[idx].setup_mount(&reg_fs[idx], rootfs);
}

void vfs_test()
{
    vfs_mkdir("/lll");
    struct file* testfile;
    vfs_open("/lll/ddd", O_CREAT, &testfile);
}