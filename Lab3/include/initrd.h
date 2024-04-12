#ifndef INITRD_H
#define INITRD_H
#define __cpio_start (volatile unsigned char *)0x40000
typedef struct {
  char magic[6];      /* Magic header "070701". */
  char ino[8];        /* "i-node" number. */
  char mode[8];       /* Permisions. */
  char uid[8];        /* User ID. */
  char gid[8];        /* Group ID. */
  char nlink[8];      /* Number of hard links. */
  char mtime[8];      /* Modification time. */
  char filesize[8];   /* File size. */
  char devmajor[8];   /* device major/minor number. */
  char devminor[8];   /* device major/minor number. */
  char r_devmajor[8]; /* device major/minor number. */
  char r_devminor[8]; /* device major/minor number. */
  char namesize[8];   /* Length of filename in bytes. */
  char check[8];      /* Check field, should be all zero. */
} cpio_t;
void initrd_list(void);
void initrd_cat(const char *name);

// Callback function of dts
int initrd_fdt_callback(void *, int);
int initrd_getLo(void);

// Loading program
void *initrd_content_getLo(const char *);
#define CPIO_HEADER_MAGIC "070701"
#define CPIO_FOOTER_MAGIC "TRAILER!!!"
#define CPIO_ALIGNMENT 4
struct cpio_header {
    char    c_magic[6];         // 魔数 (magic string) - 固定为 "070701"
    char    c_ino[8];           // 文件索引节点号 (inode number)
    char    c_mode[8];          // 文件权限 (file mode)
    char    c_uid[8];           // 用户 ID (user ID)
    char    c_gid[8];           // 组 ID (group ID)
    char    c_nlink[8];         // 硬链接数 (number of hard links)
    char    c_mtime[8];         // 最后修改时间 (last modification time)
    char    c_filesize[8];      // 文件大小 (file size)
    char    c_devmajor[8];      // 设备主要编号 (device major number)
    char    c_devminor[8];      // 设备次要编号 (device minor number)
    char    c_rdevmajor[8];     // 特殊设备主要编号 (special device major number)
    char    c_rdevminor[8];     // 特殊设备次要编号 (special device minor number)
    char    c_namesize[8];      // 路径名长度 (pathname size)
    char    c_check[8];         // 校验和 (checksum) - 总是被设置为 0
};
void cpio_ls(void *archive);
void *cpio_get_file(void *archive, const char *name, unsigned long *size);
#endif // INITRD_H
