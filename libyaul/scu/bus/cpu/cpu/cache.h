/*
 * Copyright (c) 2012-2019 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef _YAUL_CPU_CACHE_H_
#define _YAUL_CPU_CACHE_H_

#include <sys/cdefs.h>

#include <cpu/map.h>

__BEGIN_DECLS

/// @defgroup CPU_CACHE CPU Cache

/// @addtogroup CPU_CACHE
/// @{

/// Partition designated for using the cache.
#define CPU_CACHE               0x00000000UL
/// Partition designated for bypassing the cache.
#define CPU_CACHE_THROUGH       0x20000000UL
/// Partition designated for purging a specific line.
#define CPU_CACHE_PURGE         0x40000000UL
/// Partition designated for accessing the address cache array directly.
#define CPU_CACHE_ADDRESS_RW    0x60000000UL
/// Partition designated for accessing the cache data directly.
#define CPU_CACHE_DATA_RW       0xC0000000UL
/// Partition designated for bypassing the cache (I/O area).
#define CPU_CACHE_IO            0xF0000000UL

/// Address for when accessing cache data directly.
#define CPU_CACHE_WAY_0_ADDR    0xC0000000UL
/// Address for when accessing cache data directly.
#define CPU_CACHE_WAY_1_ADDR    0xC0000400UL
/// Address for when accessing cache data directly.
#define CPU_CACHE_WAY_2_ADDR    0xC0000800UL
/// Address for when accessing cache data directly.
#define CPU_CACHE_WAY_3_ADDR    0xC0000C00UL

/// @brief The size in bytes of a cache way.
#define CPU_CACHE_WAY_SIZE      (CPU_CACHE_WAY_1_ADDR - CPU_CACHE_WAY_0_ADDR)

/// @brief The size in bytes of the 2KiB RAM.
/// @see cpu_cache_way_mode_set
#define CPU_CACHE_2_WAY_SIZE    (CPU_CACHE_WAY_2_ADDR - CPU_CACHE_WAY_0_ADDR)

/// @brief The size of a cache line in bytes.
#define CPU_CACHE_LINE_SIZE     16UL

/// @brief Given the tag bits from @ref cpu_cache_data_line_t, convert to a
/// physical address.
/// @see cpu_cache_data_line_t
#define CPU_CACHE_TAG_ADDRESS(x) ((uint32_t)(x) >> 10)

/// @brief Specify variable to be uncached.
#define __uncached          __section(".uncached")

/// @brief Specify function to be uncached.
#define __uncached_function __section(".uncached.function")

/// @brief Cache mode.
typedef enum cpu_cache_mode {
        /// Four-way set associative.
        CPU_CACHE_MODE_4_WAY = 0x00,
        /// Two-way set associate and 2KiB RAM.
        CPU_CACHE_MODE_2_WAY = 0x08
} cpu_cache_mode_t;

/// @brief Cache type.
typedef enum cpu_cache_type {
        /// Instruction cache type.
        CPU_CACHE_TYPE_I = 0x02,
        /// Data cache type.
        CPU_CACHE_TYPE_D = 0x04
} cpu_cache_type_t;

/// @brief Cache data line representing the data read from the cache.
/// @see cpu_cache_data_way_t
/// @see cpu_cache_data_way_read
typedef union cpu_cache_data_line {
        unsigned int      :3;
        /// @brief Tag bits.
        unsigned int   tag:19;
        /// @brief LRU bits.
        unsigned int   lru:6;
        unsigned int      :1;
        /// @brief Validity bit.
        unsigned int valid:1;
        unsigned int      :2;
} __packed cpu_cache_data_line_t;

/// @brief Cache data way.
/// @see cpu_cache_data_way_read
typedef struct cpu_cache_data_way {
        cpu_cache_data_line_t data[CPU_CACHE_WAY_SIZE / CPU_CACHE_LINE_SIZE];
} __aligned(4) cpu_cache_data_way_t;

/// @brief Enable cache.
static inline void __always_inline
cpu_cache_enable(void)
{
        MEMORY_WRITE_OR(8, CPU(CCR), 0x01);
}

/// @brief Disable cache.
static inline void __always_inline
cpu_cache_disable(void)
{
        MEMORY_WRITE_AND(8, CPU(CCR), ~0x01);
}

/// @brief Enable type replacement.
///
/// @details When either instruction and/or data is fetched from memory, the
/// cache data is written.
///
/// @param type The cache type(s) to enable.
static inline void __always_inline
cpu_cache_repl_enable(cpu_cache_type_t type)
{
        volatile uint8_t * const reg_ccr =
            (volatile uint8_t *)CPU(CCR);

        const uint8_t t0 = *reg_ccr & ~((uint8_t)type | 0x01);

        *reg_ccr = t0;
        *reg_ccr = t0 | 0x01;
}

/// @brief Disable type replacement in the cache.
///
/// @details When either an instruction or data is fetched from memory, cache
/// data is not written to the cache even if there is a cache miss.
///
/// @param type The cache type(s) to disable.
static inline void __always_inline
cpu_cache_repl_disable(cpu_cache_type_t type)
{
        volatile uint8_t * const reg_ccr =
            (volatile uint8_t *)CPU(CCR);

        const uint8_t t0 = *reg_ccr & ~0x01;

        /* Set bit with cache disabled */
        *reg_ccr = t0 | (uint8_t)type;
        /* Enable cache and set bit(s) */
        *reg_ccr = t0 | (uint8_t)type | 0x01;
}

/// @brief Change the mode the cache operates.
///
/// @details The cache can be set to operate as a four-way set associative
/// cache, or as a two-way associative cache and 2KiB RAM.
///
/// In the two-way mode, ways `0` and `1` are RAM.
///
/// @param mode Mode.
static inline void __always_inline
cpu_cache_way_mode_set(cpu_cache_mode_t mode)
{
        volatile uint8_t * const reg_ccr =
            (volatile uint8_t *)CPU(CCR);

        const uint8_t t0 = *reg_ccr & ~0x0F;

        *reg_ccr = t0;
        *reg_ccr = t0 | (uint8_t)mode;
        *reg_ccr = t0 | (uint8_t)mode | 0x01;
}

/// @brief Cache line of the specified address is purged.
///
/// @details The starting address to purge does not to be cache-line aligned.
/// Calling this function will not pollute the cache.
///
/// @param address The address associated with cache line.
extern void cpu_cache_line_purge(void *address) __uncached_function;

/// @brief Cache lines of the specified address are purged.
///
/// @details The starting address to purge does not to be cache-line aligned.
/// The area to cache purge does not have to be in multiples of the cache line
/// size. Calling this function will not pollute the cache.
///
/// @param address The starting address.
/// @param len     The area in bytes.
extern void cpu_cache_area_purge(void *address, uint32_t len) __uncached_function;

/// @brief Purge the entire cache.
///
/// @details All cache entries and all valid bits and LRU bits of all ways are
/// initialized to `0`. Calling this function will not pollute the cache.
extern void cpu_cache_purge(void) __no_reorder __uncached_function;

/// @brief Walk one of the 4 ways the CPU cache and dump cache state bits.
///
/// @details It's imperative that the function that calls @ref
/// cpu_cache_data_way_read must be uncached so that it doesn't taint the cache
/// itself.
///
/// @param     way       The cache way to read from.
/// @param[in] cache_way The cache way buffer to write to.
extern void cpu_cache_data_way_read(uint8_t way, cpu_cache_data_way_t *data_way) __uncached_function;

/// @}

__END_DECLS

#endif /* !_YAUL_CPU_CACHE_H_ */
