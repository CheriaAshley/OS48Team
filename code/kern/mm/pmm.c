#include <default_pmm.h>
#include <best_fit_pmm.h>
#include <defs.h>
#include <error.h>
#include <memlayout.h>
#include <mmu.h>
#include <pmm.h>
#include <sbi.h>
#include <stdio.h>
#include <string.h>
#include <riscv.h>
#include <dtb.h>

// virtual address of physical page array
struct Page *pages;
// amount of physical memory (in pages)
size_t npage = 0;
// the kernel image is mapped at VA=KERNBASE and PA=info.base
uint64_t va_pa_offset;
// memory starts at 0x80000000 in RISC-V
// DRAM_BASE defined in riscv.h as 0x80000000
const size_t nbase = DRAM_BASE / PGSIZE;

// virtual address of boot-time page directory
uintptr_t *satp_virtual = NULL;
// physical address of boot-time page directory
uintptr_t satp_physical;

// physical memory management
const struct pmm_manager *pmm_manager;


static void check_alloc_page(void);

// init_pmm_manager - initialize a pmm_manager instance
static void init_pmm_manager(void) {
    pmm_manager = &default_pmm_manager;
    cprintf("memory management: %s\n", pmm_manager->name);
    pmm_manager->init();
}

// init_memmap - call pmm->init_memmap to build Page struct for free memory
static void init_memmap(struct Page *base, size_t n) {
    pmm_manager->init_memmap(base, n);
}

// alloc_pages - call pmm->alloc_pages to allocate a continuous n*PAGESIZE
// memory
struct Page *alloc_pages(size_t n) {
    return pmm_manager->alloc_pages(n);
}

// free_pages - call pmm->free_pages to free a continuous n*PAGESIZE memory
void free_pages(struct Page *base, size_t n) {
    pmm_manager->free_pages(base, n);
}

// nr_free_pages - call pmm->nr_free_pages to get the size (nr*PAGESIZE)
// of current free memory
size_t nr_free_pages(void) {
    return pmm_manager->nr_free_pages();
}

//lab2 function1 2413195
static void page_init(void) {
    // 1. 获取物理内存信息
    uint64_t mem_base = get_memory_base();
    uint64_t mem_size = get_memory_size();

    // 2. 初始化地址转换偏移量
    va_pa_offset = PHYSICAL_MEMORY_OFFSET;

    // 3. 检查设备树返回的内存信息
    if (mem_size == 0 || mem_base + mem_size < mem_base) {
        panic("Invalid physical memory range");
    }

    // 4. 计算物理内存结束地址及页号上界
    uint64_t mem_end = mem_base + mem_size;
    npage = mem_end / PGSIZE;

    // 5. 检查物理内存范围是否符合当前内核布局
    if (mem_base != DRAM_BASE || mem_end > PHYSICAL_MEMORY_END) {
        panic("Unsupported physical memory range");
    }

    // 6. 获取内核镜像结束位置
    extern char end[];

    uintptr_t kernel_end = ROUNDUP((uintptr_t)end, PGSIZE);

    // 7. 检查内核结束位置
    uintptr_t kernel_end_pa = PADDR(kernel_end);

    if (kernel_end_pa < KERNEL_BEGIN_PADDR ||
        kernel_end_pa > mem_end) {
        panic("Invalid kernel end address");
    }

    // 8. 为物理页描述符数组分配空间
    // pages[0] 对应 DRAM_BASE 处的物理页
    pages = (struct Page *)kernel_end;

    // 计算描述符数组所占用的空间
    size_t page_count = npage - nbase;
    size_t pages_size = page_count * sizeof(struct Page);

    // 9. 计算描述符数组结束位置
    uintptr_t pages_end = ROUNDUP(
        (uintptr_t)pages + pages_size, PGSIZE
    );

    // 确保描述符数组没有超出物理内存范围
    if (pages_end < (uintptr_t)pages ||
        pages_end > (uintptr_t)(mem_end + va_pa_offset)) {
        panic("Not enough memory for Page descriptors");
    }

    // 10. 初始化所有物理页描述符
    for (size_t i = 0; i < page_count; i++) {
        SetPageReserved(pages + i);
        set_page_ref(pages + i, 0);
        (pages + i)->property = 0;
    }

    // 11. 计算真正可用的物理内存起始地址
    uintptr_t free_begin = PADDR(pages_end);
    free_begin = ROUNDUP(free_begin, PGSIZE);

    // 计算空闲物理页数量
    if (free_begin >= mem_end) {
        panic("No free physical memory");
    }

    size_t free_pages = (mem_end - free_begin) / PGSIZE;

    // 12. 将空闲物理页交给内存管理器
    init_memmap(pa2page(free_begin), free_pages);

    // 13. 输出初始化结果，方便实验验证
    cprintf("Physical memory base: 0x%016lx\n", mem_base);
    cprintf("Physical memory size: %lu MB\n",
            mem_size / (1024 * 1024));
    cprintf("Total managed pages: %lu\n", page_count);
    cprintf("Free pages: %lu\n", nr_free_pages());
}
/* pmm_init - initialize the physical memory management */
void pmm_init(void) {
    // We need to alloc/free the physical memory (granularity is 4KB or other size).
    // So a framework of physical memory manager (struct pmm_manager)is defined in pmm.h
    // First we should init a physical memory manager(pmm) based on the framework.
    // Then pmm can alloc/free the physical memory.
    // Now the first_fit/best_fit/worst_fit/buddy_system pmm are available.
    init_pmm_manager();

    // detect physical memory space, reserve already used memory,
    // then use pmm->init_memmap to create free page list
    page_init();

    // use pmm->check to verify the correctness of the alloc/free function in a pmm
    check_alloc_page();

    extern char boot_page_table_sv39[];
    satp_virtual = (pte_t*)boot_page_table_sv39;
    satp_physical = PADDR(satp_virtual);
    cprintf("satp virtual address: 0x%016lx\nsatp physical address: 0x%016lx\n", satp_virtual, satp_physical);
}

static void check_alloc_page(void) {
    pmm_manager->check();
    cprintf("check_alloc_page() succeeded!\n");
}
