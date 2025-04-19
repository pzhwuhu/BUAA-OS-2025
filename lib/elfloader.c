#include <elf.h>
#include <pmap.h>

/* 验证给定的二进制文件是否为可执行文件并返回ELF头*/
const Elf32_Ehdr *elf_from(const void *binary, size_t size)
{
	const Elf32_Ehdr *ehdr = (const Elf32_Ehdr *)binary;
	if (size >= sizeof(Elf32_Ehdr) && ehdr->e_ident[EI_MAG0] == ELFMAG0 &&
		ehdr->e_ident[EI_MAG1] == ELFMAG1 && ehdr->e_ident[EI_MAG2] == ELFMAG2 &&
		ehdr->e_ident[EI_MAG3] == ELFMAG3 && ehdr->e_type == 2)
	{
		return ehdr;
	}
	return NULL;
}

/* Overview:
 *   load an elf format binary file. Map all section
 * at correct virtual address.
 *
 * Pre-Condition:
 *   `binary` can't be NULL and `size` is the size of binary.
 *
 * Post-Condition:
 *   Return 0 if success. Otherwise return < 0.
 *   If success, the entry point of `binary` will be stored in `start`
 */
int elf_load_seg(Elf32_Phdr *ph, const void *bin, elf_mapper_t map_page, void *data)
{
	u_long va = ph->p_vaddr;
	size_t bin_size = ph->p_filesz;
	size_t sgsize = ph->p_memsz;
	u_int perm = PTE_V; /* 由段的权限位设置页面的权限位 */
	if (ph->p_flags & PF_W)
	{
		perm |= PTE_D;
	}

	int r;
	size_t i;
	u_long offset = va - ROUNDDOWN(va, PAGE_SIZE);
	/* 处理未对齐的前缀部分*/
	if (offset != 0)
	{
		if ((r = map_page(data, va, offset, perm, bin,
						  MIN(bin_size, PAGE_SIZE - offset))) != 0)
		{
			return r;
		}
	}

	/* Step 1: load all content of bin into memory. */
	/* 将data和text段加载进内存*/
	for (i = offset ? MIN(bin_size, PAGE_SIZE - offset) : 0; i < bin_size; i += PAGE_SIZE)
	{
		if ((r = map_page(data, va + i, 0, perm, bin + i, MIN(bin_size - i, PAGE_SIZE))) !=
			0)
		{
			return r;
		}
	}

	/* Step 2: alloc pages to reach `sgsize` when `bin_size` < `sgsize`. */
	/* bss段用0填充 */
	while (i < sgsize)
	{
		if ((r = map_page(data, va + i, 0, perm, NULL, MIN(sgsize - i, PAGE_SIZE))) != 0)
		{
			return r;
		}
		i += PAGE_SIZE;
	}
	return 0;
}
