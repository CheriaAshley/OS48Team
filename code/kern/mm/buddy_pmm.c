#include <buddy_pmm.h>
#include <list.h>
#include <pmm.h>

// lab2 task3 2413533: Buddy System implementation and tests
#define MAX_ORDER 15
#define PG_buddy_allocated 2
#define PageBuddyAllocated(page) \
    (((page)->flags >> PG_buddy_allocated) & 1)
#define SetPageBuddyAllocated(page) \
    ((page)->flags |= (1UL << PG_buddy_allocated))
#define ClearPageBuddyAllocated(page) \
    ((page)->flags &= ~(1UL << PG_buddy_allocated))

static free_area_t free_area[MAX_ORDER + 1];
static size_t nr_free;
static ppn_t managed_start_pfn;
static ppn_t managed_end_pfn;

static void buddy_check(void);

static size_t
buddy_block_size(unsigned int order) {
    assert(order <= MAX_ORDER);
    return 1UL << order;
}

static void
buddy_add_free_block(struct Page *base, unsigned int order) {
    base->property = order;
    ClearPageBuddyAllocated(base);
    SetPageProperty(base);
    list_add(&free_area[order].free_list, &base->page_link);
}

static void
buddy_init(void) {
    for (unsigned int order = 0; order <= MAX_ORDER; order++) {
        list_init(&free_area[order].free_list);
        free_area[order].nr_free = 0;
    }
    nr_free = 0;
    managed_start_pfn = 0;
    managed_end_pfn = 0;
}

static void
buddy_init_memmap(struct Page *base, size_t n) {
    assert(n > 0);
    assert(managed_start_pfn == 0 && managed_end_pfn == 0);

    for (struct Page *p = base; p != base + n; p++) {
        assert(PageReserved(p));
        p->flags = 0;
        p->property = 0;
        set_page_ref(p, 0);
    }

    managed_start_pfn = page2ppn(base);
    managed_end_pfn = managed_start_pfn + n;

    ppn_t pfn = managed_start_pfn;
    size_t remaining = n;
    while (remaining > 0) {
        unsigned int order = MAX_ORDER;
        while (order > 0) {
            size_t block_size = buddy_block_size(order);
            if (block_size <= remaining &&
                (pfn & (block_size - 1)) == 0) {
                break;
            }
            order--;
        }

        struct Page *block = &pages[pfn - nbase];
        buddy_add_free_block(block, order);
        size_t block_size = buddy_block_size(order);
        nr_free += block_size;
        free_area[order].nr_free++;
        pfn += block_size;
        remaining -= block_size;
    }
}

static void
buddy_split_block(struct Page *base, unsigned int from_order,
                  unsigned int to_order) {
    assert(from_order >= to_order);
    while (from_order > to_order) {
        from_order--;
        struct Page *buddy = base + buddy_block_size(from_order);
        buddy_add_free_block(buddy, from_order);
        free_area[from_order].nr_free++;
    }
    base->property = to_order;
    ClearPageProperty(base);
    SetPageBuddyAllocated(base);
}

static struct Page *
buddy_alloc_pages(size_t n) {
    assert(n > 0);
    if (n > buddy_block_size(MAX_ORDER)) {
        return NULL;
    }

    unsigned int order = 0;
    while (buddy_block_size(order) < n) {
        order++;
    }

    unsigned int found_order = order;
    while (found_order <= MAX_ORDER &&
           list_empty(&free_area[found_order].free_list)) {
        found_order++;
    }
    if (found_order > MAX_ORDER) {
        return NULL;
    }

    list_entry_t *entry = list_next(&free_area[found_order].free_list);
    struct Page *base = le2page(entry, page_link);
    list_del(&base->page_link);
    free_area[found_order].nr_free--;
    ClearPageProperty(base);
    base->property = 0;

    buddy_split_block(base, found_order, order);
    nr_free -= buddy_block_size(order);
    return base;
}

static struct Page *
buddy_find_page(ppn_t pfn) {
    assert(pfn >= managed_start_pfn && pfn < managed_end_pfn);
    assert(pfn >= nbase);
    return &pages[pfn - nbase];
}

static void
buddy_free_pages(struct Page *base, size_t n) {
    assert(base != NULL);
    assert(n > 0 && (n & (n - 1)) == 0);

    uintptr_t base_addr = (uintptr_t)base;
    uintptr_t pages_addr = (uintptr_t)pages;
    assert(base_addr >= pages_addr);
    assert((base_addr - pages_addr) % sizeof(struct Page) == 0);

    size_t page_index = (base_addr - pages_addr) / sizeof(struct Page);
    assert(page_index < npage - nbase);
    assert(page_index >= managed_start_pfn - nbase);

    ppn_t pfn = page2ppn(base);
    assert(pfn >= managed_start_pfn && pfn < managed_end_pfn);
    assert(!PageReserved(base));
    assert(PageBuddyAllocated(base));
    assert(!PageProperty(base));
    assert(base->property <= MAX_ORDER);

    unsigned int order = base->property;
    size_t block_size = buddy_block_size(order);
    assert(n == block_size);
    assert((pfn & (block_size - 1)) == 0);
    assert(n <= managed_end_pfn - pfn);

    for (size_t i = 0; i < n; i++) {
        struct Page *page = base + i;
        assert(!PageReserved(page));
        if (i != 0) {
            assert(!PageProperty(page));
            assert(!PageBuddyAllocated(page));
        }
    }
    for (size_t i = 0; i < n; i++) {
        struct Page *page = base + i;
        set_page_ref(page, 0);
    }

    ClearPageBuddyAllocated(base);
    base->property = order;
    SetPageProperty(base);

    while (order < MAX_ORDER) {
        ppn_t buddy_pfn = page2ppn(base) ^ buddy_block_size(order);
        if (buddy_pfn < managed_start_pfn ||
            buddy_pfn >= managed_end_pfn ||
            buddy_block_size(order) > managed_end_pfn - buddy_pfn) {
            break;
        }

        struct Page *buddy = buddy_find_page(buddy_pfn);
        if (!PageProperty(buddy) || PageBuddyAllocated(buddy) ||
            buddy->property != order) {
            break;
        }

        list_del(&buddy->page_link);
        free_area[order].nr_free--;
        ClearPageProperty(buddy);
        buddy->property = 0;

        if (buddy_pfn < page2ppn(base)) {
            ClearPageProperty(base);
            base->property = 0;
            ClearPageBuddyAllocated(base);
            base = buddy;
        } else {
            ClearPageProperty(buddy);
            buddy->property = 0;
        }
        order++;
        base->property = order;
        SetPageProperty(base);
    }

    buddy_add_free_block(base, order);
    free_area[order].nr_free++;
    nr_free += n;
}

static size_t
buddy_nr_free_pages(void) {
    return nr_free;
}

static void
buddy_check_free_lists(void) {
    size_t total = 0;

    for (unsigned int order = 0; order <= MAX_ORDER; order++) {
        size_t count = 0;
        list_entry_t *head = &free_area[order].free_list;
        for (list_entry_t *entry = list_next(head); entry != head;
             entry = list_next(entry)) {
            struct Page *page = le2page(entry, page_link);
            size_t block_size = buddy_block_size(order);
            ppn_t pfn = page2ppn(page);

            assert(PageProperty(page));
            assert(!PageBuddyAllocated(page));
            assert(page->property == order);
            assert(pfn >= managed_start_pfn);
            assert(pfn < managed_end_pfn);
            assert((pfn & (block_size - 1)) == 0);
            assert(block_size <= managed_end_pfn - pfn);
            assert(entry->next->prev == entry);
            assert(entry->prev->next == entry);
            total += block_size;
            count++;
        }
        assert(count == free_area[order].nr_free);
    }
    assert(total == nr_free);
}

static void
buddy_check(void) {
    size_t initial_free = nr_free;
    buddy_check_free_lists();

    struct Page *page = alloc_page();
    assert(page != NULL);
    assert(page->property == 0);
    assert(page_ref(page) == 0);
    assert(PageBuddyAllocated(page));
    assert(!PageProperty(page));
    assert(nr_free == initial_free - 1);
    free_page(page);
    assert(nr_free == initial_free);

    page = alloc_pages(3);
    assert(page != NULL);
    assert(page->property == 2);
    assert(PageBuddyAllocated(page));
    assert((page2ppn(page) & 3) == 0);
    assert(nr_free == initial_free - 4);
    free_pages(page, 4);
    assert(nr_free == initial_free);

    size_t insufficient = nr_free + 1;
    assert(alloc_pages(insufficient) == NULL);
    assert(nr_free == initial_free);

    unsigned int source_order = MAX_ORDER;
    while (source_order >= 2 &&
           list_empty(&free_area[source_order].free_list)) {
        source_order--;
    }
    assert(source_order >= 2);

    list_entry_t *entry = list_next(&free_area[source_order].free_list);
    page = le2page(entry, page_link);
    list_del(&page->page_link);
    free_area[source_order].nr_free--;
    ClearPageProperty(page);
    page->property = 0;
    buddy_split_block(page, source_order, 0);
    nr_free--;
    assert(nr_free == initial_free - 1);
    assert(page->property == 0);
    assert(PageBuddyAllocated(page));
    buddy_free_pages(page, 1);
    assert(nr_free == initial_free);

    buddy_check_free_lists();
}

const struct pmm_manager buddy_pmm_manager = {
    .name = "buddy_pmm_manager",
    .init = buddy_init,
    .init_memmap = buddy_init_memmap,
    .alloc_pages = buddy_alloc_pages,
    .free_pages = buddy_free_pages,
    .nr_free_pages = buddy_nr_free_pages,
    .check = buddy_check,
};
