#ifndef _ASM_GENERIC_IO_H
#define _ASM_GENERIC_IO_H

#include <linux/types.h>
#include <linux/compiler.h>
#include <asm/barrier.h>
#include <uapi/linux/little_endian.h>

#ifndef __io_br
#define __io_br()      barrier()
#endif

/* prevent prefetching of coherent DMA data ahead of a dma-complete */
#ifndef __io_ar
#ifdef rmb
#define __io_ar(v)      rmb()
#else
#define __io_ar(v)      barrier()
#endif
#endif

/* serialize device access against a spin_unlock, usually handled there. */
#ifndef __io_aw
#define __io_aw()      do { } while (0)
#endif

#ifndef __io_pbw
#define __io_pbw()     __io_bw()
#endif

#ifndef __io_paw
#define __io_paw()     __io_aw()
#endif

#ifndef __io_pbr
#define __io_pbr()     __io_br()
#endif

#ifndef __io_par
#define __io_par(v)     __io_ar(v)
#endif

/* flush writes to coherent DMA data before possibly triggering a DMA read */
#ifndef __io_bw
#ifdef wmb
#define __io_bw()      wmb()
#else
#define __io_bw()      barrier()
#endif
#endif

static inline void log_write_mmio(u64 val, u8 width, volatile void *addr,
				  unsigned long caller_addr, unsigned long caller_addr0) {}
static inline void log_post_write_mmio(u64 val, u8 width, volatile void *addr,
				       unsigned long caller_addr, unsigned long caller_addr0) {}
static inline void log_read_mmio(u8 width, const volatile void *addr,
				 unsigned long caller_addr, unsigned long caller_addr0) {}
static inline void log_post_read_mmio(u64 val, u8 width, const volatile void *addr,
				      unsigned long caller_addr, unsigned long caller_addr0) {}

static inline u8 __raw_readb(const volatile void *addr)
{
	return *(const volatile u8 *)addr;
}

static inline u16 __raw_readw(const volatile void *addr)
{
	return *(const volatile u16 *)addr;
}

static inline u32 __raw_readl(const volatile void *addr)
{
	return *(const volatile u32 *)addr;
}

static inline u64 __raw_readq(const volatile void *addr)
{
	return *(const volatile u64 *)addr;
}

static inline void __raw_writeb(u8 value, volatile void *addr)
{
	*(volatile u8 *)addr = value;
}

static inline void __raw_writew(u16 value, volatile void *addr)
{
	*(volatile u16 *)addr = value;
}

static inline void __raw_writel(u32 value, volatile void *addr)
{
	*(volatile u32 *)addr = value;
}

static inline void __raw_writeq(u64 value, volatile void *addr)
{
	*(volatile u64 *)addr = value;
}

/*
 * {read,write}{b,w,l,q}() access little endian memory and return result in
 * native endianness.
 */

#ifndef readb
#define readb readb
static inline u8 readb(const volatile void *addr)
{
	u8 val;

	__io_br();
	val = __raw_readb(addr);
	__io_ar(val);
	return val;
}
#endif

#ifndef readw
#define readw readw
static inline u16 readw(const volatile void *addr)
{
	u16 val;

	__io_br();
	val = __le16_to_cpu((__le16 __force)__raw_readw(addr));
	__io_ar(val);
	return val;
}
#endif

#ifndef readl
#define readl readl
static inline u32 readl(const volatile void *addr)
{
	u32 val;

	__io_br();
	val = __le32_to_cpu((__le32 __force)__raw_readl(addr));
	__io_ar(val);

	return val;
}
#endif

#ifndef readq
#define readq readq
static inline u64 readq(const volatile void *addr)
{
	u64 val;

	__io_br();
	val = __le64_to_cpu(__raw_readq(addr));
	__io_ar(val);

	return val;
}
#endif

#ifndef writeb
#define writeb writeb
static inline void writeb(u8 value, volatile void *addr)
{
	__io_bw();
	__raw_writeb(value, addr);
	__io_aw();
}
#endif

#ifndef writew
#define writew writew
static inline void writew(u16 value, volatile void *addr)
{
	__io_bw();
	__raw_writew((u16 __force)__cpu_to_le16(value), addr);
	__io_aw();
}
#endif

#ifndef writel
#define writel writel
static inline void writel(u32 value, volatile void *addr)
{
	__io_bw();
	__raw_writel((u32 __force)__cpu_to_le32(value), addr);
	__io_aw();
}
#endif

#ifndef writeq
#define writeq writeq
static inline void writeq(u64 value, volatile void *addr)
{
	__io_bw();
	__raw_writeq(__cpu_to_le64(value), addr);
	__io_aw();
}
#endif

#ifndef PCI_IOBASE
#define PCI_IOBASE ((void *)0)
#endif

#ifndef IO_SPACE_LIMIT
#define IO_SPACE_LIMIT 0xffff
#endif

/*
 * {in,out}{b,w,l}() access little endian I/O. {in,out}{b,w,l}_p() can be
 * implemented on hardware that needs an additional delay for I/O accesses to
 * take effect.
 */

#if !defined(inb) && !defined(_inb)
#define _inb _inb
static inline u8 _inb(unsigned long addr)
{
	u8 val;

	__io_pbr();
	val = __raw_readb(PCI_IOBASE + addr);
	__io_par(val);
	return val;
}
#endif

#if !defined(inw) && !defined(_inw)
#define _inw _inw
static inline u16 _inw(unsigned long addr)
{
	u16 val;

	__io_pbr();
	val = __le16_to_cpu((__le16 __force)__raw_readw(PCI_IOBASE + addr));
	__io_par(val);
	return val;
}
#endif

#if !defined(inl) && !defined(_inl)
#define _inl _inl
static inline u32 _inl(unsigned long addr)
{
	u32 val;

	__io_pbr();
	val = __le32_to_cpu((__le32 __force)__raw_readl(PCI_IOBASE + addr));
	__io_par(val);
	return val;
}
#endif

#if !defined(outb) && !defined(_outb)
#define _outb _outb
static inline void _outb(u8 value, unsigned long addr)
{
	__io_pbw();
	__raw_writeb(value, PCI_IOBASE + addr);
	__io_paw();
}
#endif

#if !defined(outw) && !defined(_outw)
#define _outw _outw
static inline void _outw(u16 value, unsigned long addr)
{
	__io_pbw();
	__raw_writew((u16 __force)__cpu_to_le16(value), PCI_IOBASE + addr);
	__io_paw();
}
#endif

#if !defined(outl) && !defined(_outl)
#define _outl _outl
static inline void _outl(u32 value, unsigned long addr)
{
	__io_pbw();
	__raw_writel((u32 __force)__cpu_to_le32(value), PCI_IOBASE + addr);
	__io_paw();
}
#endif

#ifndef inb
#define inb _inb
#endif

#ifndef inw
#define inw _inw
#endif

#ifndef inl
#define inl _inl
#endif

#ifndef outb
#define outb _outb
#endif

#ifndef outw
#define outw _outw
#endif

#ifndef outl
#define outl _outl
#endif

#endif /* _ASM_GENERIC_IO_H */
