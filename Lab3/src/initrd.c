#include "initrd.h"
#include "dtb.h"
#include "str.h"
#include "uart.h"
#include <stddef.h>

static void *lo_ramfs = 0x0;

int memcmp(void *s1, void *s2, int n) {
  unsigned char *a = s1, *b = s2;
  while (n-- > 0) {
    if (*a != *b) {
      return *a - *b;
    }
    a++;
    b++;
  }
  return 0;
}
/*************************************************************************
 * The value stores in the cpio header is hex value, we need this function
 * to transform from hex char* to bin
 *
 * s : The string of hex numbers.
 * n : The size of string.
 ***********************************************************************/
static int hex2bin(char *s, int n) {
  int r = 0;
  while (n-- > 0) {
    r = r << 4;
    if (*s >= 'A') {
      r += *s++ - 'A' + 10;
    } else if (*s >= 0) {
      r += *s++ - '0'; // traslate hex to oct
    }
  }
  return r;
}

/*********************************************************
 * This is callback function for getting the start * address of the initrd.
 * Please use this function * with `fdt_find_do()`.
 * *******************************************************/
int initrd_fdt_callback(void *start, int size) {
  if (size != 4) {
    uart_puti(size);
    uart_puts("Size not 4!\n");
    return 1;
  }
  uint32_t t = *((uint32_t *)start);
  lo_ramfs = (void *)(b2l_32(t));
  return 0;
}

/********************************************************
 * Function return the location (address) of the initrd.
 *******************************************************/
int initrd_getLo() { return lo_ramfs; }

/********************************************************
 * Return the start address of the content.
 * Called by lodaer to run the program.
 * *****************************************************/
void *initrd_content_getLo(const char *name) {
  char *buf = (char *)lo_ramfs;
  int ns = 0;
  int fs = 0;
  int pad_n = 0;
  int pad_f = 0;
  // uart_puts(name);
  while (!(memcmp(buf, "070701", 6)) &&
         memcmp(buf + sizeof(cpio_t), "TRAILER!!",
                9)) { // test magic number of new ascii
    cpio_t *header = (cpio_t *)buf;
    ns = hex2bin(header->namesize, 8); // Get the size of name
    fs = hex2bin(header->filesize, 8); // Get teh size of file content
    pad_n = (4 - ((sizeof(cpio_t) + ns) % 4)) % 4; // Padding size
    pad_f = (4 - (fs % 4)) % 4;
    // Find the target file
    if (!(strncmp(buf + sizeof(cpio_t), name, ns - 1)))
      break;
    buf += (sizeof(cpio_t) + ns + fs + pad_n + pad_f); // Jump to next record
  }
  if (fs > 0) {
    return (void *)(buf + sizeof(cpio_t) + ns + pad_n);
  }
  return NULL;
}

/* Align 'n' up to the value 'align', which must be a power of two. */
static unsigned long align_up(unsigned long n, unsigned long align)
{
    return (n + align - 1) & (~(align - 1));
}

/* Parse an ASCII hex string into an integer. */
static unsigned long parse_hex_str(char *s, unsigned int max_len)
{
    unsigned long r = 0;
    unsigned long i;

    for (i = 0; i < max_len; i++) {
        r *= 16;
        if (s[i] >= '0' && s[i] <= '9') {
            r += s[i] - '0';
        }  else if (s[i] >= 'a' && s[i] <= 'f') {
            r += s[i] - 'a' + 10;
        }  else if (s[i] >= 'A' && s[i] <= 'F') {
            r += s[i] - 'A' + 10;
        } else {
            return r;
        }
        continue;
    }
    return r;
}

/*
 * Compare up to 'n' characters in a string.
 *
 * We re-implement the wheel to avoid dependencies on 'libc', required for
 * certain environments that are particularly impoverished.
 */
static int cpio_strncmp(const char *a, const char *b, unsigned long n)
{
    unsigned long i;
    for (i = 0; i < n; i++) {
        if (a[i] != b[i]) {
            return a[i] - b[i];
        }
        if (a[i] == 0) {
            return 0;
        }
    }
    return 0;
}

// /**
//  * This is an implementation of string copy because, cpi doesn't want to
//  * use string.h.
//  */
// static char* cpio_strcpy(char *to, const char *from) {
//     char *save = to;
//     while (*from != 0) {
//         *to = *from;
//         to++;
//         from++;
//     }
//     return save;
// }

/*
 * Parse the header of the given CPIO entry.
 *
 * Return -1 if the header is not valid, 1 if it is EOF.
 */
int cpio_parse_header(struct cpio_header *archive,
        const char **filename, unsigned long *_filesize, void **data,
        struct cpio_header **next)
{
    unsigned long filesize;
    /* Ensure magic header exists. */
    if (cpio_strncmp(archive->c_magic, CPIO_HEADER_MAGIC,
                sizeof(archive->c_magic)) != 0)
        return -1;

    /* Get filename and file size. */
    filesize = parse_hex_str(archive->c_filesize, sizeof(archive->c_filesize));
    *filename = ((char *)archive) + sizeof(struct cpio_header);

    /* Ensure filename is not the trailer indicating EOF. */
    /* CPIO_FOOTER_MAGIC = "TRAILER!!!"*/
    if (cpio_strncmp(*filename, CPIO_FOOTER_MAGIC, sizeof(CPIO_FOOTER_MAGIC)) == 0)
        return 1;

    /* Find offset to data. */
    unsigned long filename_length = parse_hex_str(archive->c_namesize,
            sizeof(archive->c_namesize));
    /* data is after filename. */
    *data = (void *)align_up(((unsigned long)archive)
            + sizeof(struct cpio_header) + filename_length, CPIO_ALIGNMENT);
    *next = (struct cpio_header *)align_up(((unsigned long)*data) + filesize, CPIO_ALIGNMENT);
    if(_filesize){
        *_filesize = filesize;
    }
    return 0;
}

/*
 * Find the location and size of the file named "name" in the given 'cpio'
 * archive.
 *
 * Return NULL if the entry doesn't exist.
 *
 * Runs in O(n) time.
 */
void *cpio_get_file(void *archive, const char *name, unsigned long *size)
{
    struct cpio_header *header = archive;

    /* Find n'th entry. */
    while (1) {
        struct cpio_header *next;
        void *result;
        const char *current_filename;

        int error = cpio_parse_header(header, &current_filename,
                size, &result, &next);
        // End of file
        // uart_printf("ccc\n");
        if (error)
            return NULL;
        if (cpio_strncmp(current_filename, name, -1) == 0) 
            return result;
        header = next;
    }
}

void cpio_ls(void *archive) {
    const char *current_filename;
    struct cpio_header *header, *next;
    void *result;
    int error;
    unsigned long size;
    
    header = archive;
    while (1) {
        error = cpio_parse_header(header, &current_filename, &size,
                &result, &next);
        // Break on an error or nothing left to read.
        if (error) break;
        uart_printf("%s\n", current_filename);
        header = next;
    }
}

