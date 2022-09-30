#define SYS_BASE_CACHED					(0x80000000)
#define SYS_BASE_UNCACHED				(0xC0000000)

#define MEM_VIRTUAL_TO_PHYSICAL(x)		(((u32)(x)) & ~SYS_BASE_UNCACHED)									/*!< Cast virtual address to physical address, e.g. 0x8xxxxxxx -> 0x0xxxxxxx */
#define MEM_PHYSICAL_TO_K0(x)			(void*)((u32)(x) + SYS_BASE_CACHED)									/*!< Cast physical address to cached virtual address, e.g. 0x0xxxxxxx -> 0x8xxxxxxx */
#define MEM_PHYSICAL_TO_K1(x)			(void*)((u32)(x) + SYS_BASE_UNCACHED)								/*!< Cast physical address to uncached virtual address, e.g. 0x0xxxxxxx -> 0xCxxxxxxx */
#define MEM_K0_TO_PHYSICAL(x)			(void*)((u32)(x) - SYS_BASE_CACHED)									/*!< Cast physical address to cached virtual address, e.g. 0x0xxxxxxx -> 0x8xxxxxxx */
#define MEM_K1_TO_PHYSICAL(x)			(void*)((u32)(x) - SYS_BASE_UNCACHED)								/*!< Cast physical address to uncached virtual address, e.g. 0x0xxxxxxx -> 0xCxxxxxxx */
#define MEM_K0_TO_K1(x)					(void*)((u32)(x) + (SYS_BASE_UNCACHED - SYS_BASE_CACHED))			/*!< Cast cached virtual address to uncached virtual address, e.g. 0x8xxxxxxx -> 0xCxxxxxxx */
#define MEM_K1_TO_K0(x)					(void*)((u32)(x) - (SYS_BASE_UNCACHED - SYS_BASE_CACHED))			/*!< Cast uncached virtual address to cached virtual address, e.g. 0xCxxxxxxx -> 0x8xxxxxxx */

#define SYS_PROTECTNONE					0x00000000		/*!< Read and write operations on protected region is granted */
#define SYS_PROTECTREAD					0x00000001		/*!< Read from protected region is permitted */
#define SYS_PROTECTWRITE				0x00000002		/*!< Write to protected region is permitted */
#define SYS_PROTECTRDWR					(SYS_PROTECTREAD|SYS_PROTECTWRITE)	/*!< Read and write operations on protected region is permitted */
