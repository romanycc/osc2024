#include "mem.h"
#include "heap.h"
#include "uart.h"

#define NULL 0

/*==================================================================
 * Some private members.
 *=================================================================*/
// The memory array
// Use -1 to represent Free but belong to others body
// Use -2 to represent allocated
static char *mem_arr = 0;
static mem_node **free_mem = 0;
static slot **free_smem = 0;

static int list_add(int, int index);
static int list_delete(int, int index);
static int split_buddy(int index, int);
static int merge_buddy(int index, int v);
static int gen_slots(int);

/*=================================================================
 * Private function implementation
 *===============================================================*/
static int split_buddy(int index, int cnt) {
  if (cnt >= MEM_MAX || cnt < 0)
    return 1;
  // Compute Size of memory
  int k = 1;
  for (int i = 0; i < cnt; i++) {
    k *= 2;
  }
  // Prepare to spilt into two buddy
  cnt -= 1;
  if (index % k == 0) {
    // Mark now cnt
    mem_arr[index + k / 2] = cnt;
    mem_arr[index] = cnt;
    // Mark as belonged
    for (int i = index + k / 2 + 1; i < index + k; ++i) {
      mem_arr[i] = -1;
    }
    // Mark as belonged
    for (int i = index + 1; i < index + k / 2; ++i) {
      mem_arr[i] = -1;
    }
    // Add to free list
    list_add(cnt, index);
    list_add(cnt, index + k / 2);

    // Log
    uart_puts("Frame split: ");
    uart_puth(index << 12);
    uart_puts(", size: ");
    uart_puti(k);
    uart_puts(" * 4 KB\n");
    uart_puts("New frames: ");
    uart_puth(index << 12);
    uart_puts(", ");
    uart_puth((index + k / 2) << 12);
    uart_puts("\n");

    return 0;
  }
  return 1;
}

static int merge_buddy(int index, int v) {
  int t = 1;
  // The largest frame
  if (v >= MEM_MAX - 1 || v < 0)
    return 2;
  // Now frame size
  for (int i = 0; i < v; i++) {
    t *= 2;
  }
  // [NF][F] -> [  LF  ]
  // 1. align
  // 2. the right neighbor frame has same size as now frame
  if (index % (t * 2) == 0 && mem_arr[index + t] == v) {
    // cnt, idx
    list_delete(v, index + t);
    list_delete(v, index);
    list_add(v + 1, index);
    // Reset value
    for (int i = 1; i < t * 2; i++) {
      mem_arr[index + i] = -1;
    }
    mem_arr[index] = v + 1;
    // Log
    uart_puts("Frame merge: ");
    uart_puth(index << 12);
    uart_puts(" and ");
    uart_puth((index + t) << 12);
    uart_puts("New frame: ");
    uart_puth(index << 12);
    uart_puts(", size ");
    uart_puti(t * 2);
    uart_puts(" * 4 KB\n");
    // Recursive call
    merge_buddy(index, v + 1);
    return 0;
  }
  // [F][NF] -> [  LF  ]
  // 1. align
  // 2. the left neighbor frame has same size as now frame
  else if (index % (t * 2) == t && mem_arr[index - t] == v) {
    list_delete(v, index - t);
    list_delete(v, index);
    list_add(v + 1, index - t);
    for (int i = -t + 1; i < t; i++) {
      mem_arr[index + i] = -1;
    }
    mem_arr[index - t] = v + 1;

    // Log
    uart_puts("Frame merge: ");
    uart_puth(index << 12);
    uart_puts(" and ");
    uart_puth((index - t) << 12);
    uart_puts("New frame: ");
    uart_puth((index - t) << 12);
    uart_puts(", size ");
    uart_puti(t * 2);
    uart_puts(" * 4 KB\n");
    merge_buddy(index - t, v + 1);
    return 0;
  } else {
    uart_puts("Frame merge: CANNOT MERGE!\n");
  }
  return 1;
}

// Add item into Free Page List
static int list_add(int cnt, int index) {
  mem_node *cur = (mem_node *)malloc(sizeof(mem_node));
  /*
  uart_puti(cnt);
  uart_puth(cur);
  uart_puth(cur->index);
  uart_puth(free_mem[cnt]);
  uart_puts("free_mem[cnt]");
  uart_puth(cur->index);
  */
  cur->index = index;
  // If no this size then create one
  if (free_mem[cnt] == NULL) {
    free_mem[cnt] = cur;
    cur->next = NULL;
    cur->prev = NULL;
  } else {
    // assert cur to linked list free_mem[cnt]
    cur->next = free_mem[cnt];
    free_mem[cnt]->prev = cur;
    free_mem[cnt] = cur;
  }
  // uart_puts("list_add return\n");
  return 0;
}

// Delete the item in the Free Page List.
static int list_delete(int cnt, int index) {
  mem_node *head = free_mem[cnt];
  mem_node *cur;

  for (cur = head; cur != NULL; cur = cur->next) {
    if (index == cur->index) {
      if (cur == head) {
        free_mem[cnt] = cur->next;
        return 0;
      }
      if (cur->next != NULL) {
        cur->next->prev = cur->prev;
      }
      if (cur->prev != NULL) {
        cur->prev->next = cur->next;
      }
      return 0;
    }
  }
  uart_puth(index << 12);
  uart_puti(cnt);
  uart_puts("Cannot find the target node to delete\n");
  return 1;
}

// Pop a item in the Free Page list
static int list_pop(int cnt) {
  int ret = free_mem[cnt]->index;
  list_delete(cnt, ret);
  return ret;
}

// Add item in Free Slot List
static int list_adds(int cnt, void *addr) {
  slot *cur = (slot *)malloc(sizeof(slot));
  cur->addr = addr;
  if (free_smem[cnt] == NULL) {
    free_smem[cnt] = cur;
    cur->next = NULL;
    cur->prev = NULL;
  } else {
    cur->next = free_smem[cnt];
    free_smem[cnt]->prev = cur;
    free_smem[cnt] = cur;
  }
  return 0;
}

// Delete the specific item in Free Slot list
static int list_deletes(int cnt, void *addr) {
  slot *head = free_smem[cnt];
  slot *cur;

  for (cur = head; cur != NULL; cur = cur->next) {
    if (addr == cur->addr) {
      if (cur == head) {
        free_smem[cnt] = cur->next;
        return 0;
      }
      if (cur->next != NULL) {
        cur->next->prev = cur->prev;
      }
      if (cur->prev != NULL) {
        cur->prev->next = cur->next;
      }
      return 0;
    }
  }
  uart_puth(addr);
  uart_puti(cnt);
  uart_puts("Cannot find the target node to delete\n");
  return 1;
}

// Pop the first item for Slots in free list
static void *list_pops(int cnt) {
  int ret = free_smem[cnt]->addr;
  list_deletes(cnt, ret);
  return ret;
}
/*******************************************************************
 * Generate small memorys by a 4KB frame
 * support 2^0 -> 2^4 * 32 bits
 * @size: The size should be multiple of 4 bytes.
 ******************************************************************/
static int gen_slots(int size) {
  int t = 1;
  for (int i = 0; i < size; ++i) {
    t *= 2;
  }
  uint32_t *mem = (uint32_t *)pmalloc(0);
  for (int i = 0; i < FRAME_SIZE; i += t) {
    list_adds(size, (void *)(mem + i));
  }
  // Log
  uart_puts("***Slot generate size: ");
  uart_puti(t * 4);
  uart_puts(" bytes\n");
  return 0;
}

/******************************************************************
 * Page malloc init (buddy system)
 *****************************************************************/
int pmalloc_init(void) {
  mem_arr = (char *)malloc(sizeof(char) * MEM_SIZE);            // use a char to indicate status of entry
  free_mem = (mem_node **)malloc(sizeof(mem_node *) * MEM_MAX); // a linked list array head with prev and next and index
  free_smem = (slot **)malloc(sizeof(slot *) * SMEM_MAX);       // also a linked list array head with prev and next and address(small allocate)
  int t = 1;
  if (!mem_arr || !free_mem) {
    uart_puts("Pmalloc Init error!\n");
    return 1;
  }
  // Initialize these memory value
  memset(mem_arr, -1, MEM_SIZE);
  // Initialize these memory's management value
  memset(free_mem, 0, MEM_MAX * sizeof(mem_node *));
  memset(free_smem, 0, SMEM_MAX * sizeof(slot *));
  // Compute the maximum size
  // In this case, t=2^6 * (4KB)
  for (int i = 1; i < MEM_MAX; i++) {
    t *= 2;
  }

  // Split the global array into max page size
  int i;
  for (i = 0; i < MEM_SIZE; i += t) {
    // uart_puti(i);
    // uart_puts(" ");
    mem_arr[i] = MEM_MAX - 1;
    // Add item into Free Page List
    list_add(MEM_MAX - 1, i);
  }

  return 0;
}

/********************************************************************
 * Generate slots for small memorys.
 * This function shoud be called AFTER pmalloc_init() and preserve()
 * Iterative generate different small size slot and store address in free_smem
 *******************************************************************/
int smalloc_init(void) {

  // Generate small memory slots
  for (int i = 0; i < SMEM_MAX; i++) {
    gen_slots(i);
  }
  return 0;
}

/*****************************************************************
 * Page malloc for buddy system
 ****************************************************************/
void *pmalloc(int cnt) {
  int t = 1;
  for (int i = 0; i < cnt; i++) {
    t *= 2;
  }
  if (cnt >= MEM_MAX || cnt < 0) {
    uart_puts("PMALLOC invalid size!\n");
    return 0;
  }
  int ret = 0;
  // If free memory in current size
  if (free_mem[cnt] != NULL) {
    ret = list_pop(cnt);
    // indicate these memory is allocated
    for (int i = 1; i < t; i++) {
      mem_arr[ret + i] = -1;
    }
    // indicate the start of memory
    mem_arr[ret] = -2;
    uart_puts("***PMALLOC without split!: ");
    uart_puth(ret << 12);
    uart_puts("\n");
    return (void *)(ret << 12);
  }
  // Else
  int i;
  // Find the free memory closet to target
  for (i = cnt + 1; i < MEM_MAX; i++) {
    if (free_mem[i] != NULL)
      break;
  }
  // Pop one and split it to target size
  for (int j = i; j > cnt; --j) {
    ret = list_pop(j);
    split_buddy(ret, j);
  }
  // Get memory again
  ret = list_pop(cnt);
  // Mark as belonged
  for (int i = 1; i < t; i++) {
    mem_arr[ret + i] = -1;
  }
  // Mark as used
  mem_arr[ret] = -2;
  // Log
  uart_puts("***PMALLOC with split!: ");
  uart_puth(ret << 12);
  uart_puts("\n");
  return (void *)(ret << 12);
}

/************************************************************************
 * Free the memory allocated from buddy system.
 * @addr: the target memory address to free.
 ***********************************************************************/
int pfree(void *addr) {
  int c = 1;
  // Map back to index
  int index = ((int)addr) >> 12;
  int t = 1;
  // Get the Max size
  for (int i = 0; i < MEM_MAX; i++) {
    t *= 2;
  }
  // Get the real size and saved in c
  for (int i = 1; i < t; i++) {
    if (mem_arr[index + i] != (char)-1)
      break;
    c++;
  }
  for (int i = MEM_MAX - 1; i >= 0; --i) {
    // Since c must be 2^k then store this memory back to free list
    if (c >> i) {
      mem_arr[index] = i;
      list_add(i, index);
      // Log
      uart_puts("***Free index: ");
      uart_puth(addr);
      uart_puts(", size: ");
      uart_puti(c);
      uart_puts("*4KB\n");
      merge_buddy(index, i);
      break;
    }
  }
  return 0;
}

/********************************************************************
 * page reserve
 * Each reserve is at least the largest frame size.
 * @addr: The start address to reserve.
 * @size: The size from start address to reserve.
 ******************************************************************/
int preserve(void *addr, int size) {
  int index = ((int)addr) >> 12;
  int t = 1;
  for (int i = 0; i < MEM_MAX; i++) {
    t *= 2;
  }
  // how many maximum frame, if less than one largest block then plus 1 
  size = size / (t * FRAME_SIZE) + (size % (t * FRAME_SIZE) ? 1 : 0);
  // compute how many frame is need 
  size *= t;
  // Mark as used
  for (int i = 0; i < size; i++) {
    mem_arr[index + i] = -2;
    // Delete from the largest free frme list
    if((index + i) % 64 == 0){
	    //uart_puti(index + i);
	    //uart_puts(" ");
	    //uart_puth((index+i) << 12);
	    //uart_puts(" ");
	    list_delete(MEM_MAX - 1, (index + i));
    }
  }
  // Log
  uart_puts("***Reserve from: ");
  uart_puth(addr);
  uart_puts(" to: ");
  uart_puth((index + size) << 12);
  uart_puts("\n");

  uart_puts("\n");
  return 0;
}

/***********************************************************************
 * Small memory allocation.
 * @size: the size need to be allocated
 **********************************************************************/
void *smalloc(int size) {
  size = size / 4 + ((size % 4) ? 1 : 0); // Base 32bits
  if (size >= SMEM_MAX || size < 0)
    return 0;
  uart_puti(size);
  uart_puts("* 4 bytes\n");
  return list_pops(size);
}

/*
void* sfree(void* addr){
        return list_delete(size, addr);
}
*/
