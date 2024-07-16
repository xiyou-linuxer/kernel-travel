#include <xkernel/debug.h>
#include <fs/ext4_bitmap.h>
#include <xkernel/errno.h>

void ext4_bmap_bits_free(uint8_t *bmap, uint32_t sbit, uint32_t bcnt)
{
	uint32_t i = sbit;

	while (i & 7) {

		if (!bcnt)
			return;

		ext4_bmap_bit_clr(bmap, i);

		bcnt--;
		i++;
	}
	sbit = i;
	bmap += (sbit >> 3);

	while (bcnt >= 8) {
		*bmap = 0;
		bmap += 1;
		bcnt -= 8;
		sbit += 8;
	}

	for (i = 0; i < bcnt; ++i) {
		ext4_bmap_bit_clr(bmap, i);
	}
}

int ext4_bmap_bit_find_clr(uint8_t *bmap, uint32_t sbit, uint32_t ebit,
			   uint32_t *bit_id)
{
	uint32_t i;
	uint32_t bcnt = ebit - sbit;

	i = sbit;

	while (i & 7) {

		if (!bcnt)
			return ENOSPC;

		if (ext4_bmap_is_bit_clr(bmap, i)) {
			*bit_id = sbit;
			return 0;
		}

		i++;
		bcnt--;
	}

	sbit = i;
	bmap += (sbit >> 3);

	while (bcnt >= 8) {
		if (*bmap != 0xFF) {
			for (i = 0; i < 8; ++i) {
				if (ext4_bmap_is_bit_clr(bmap, i)) {
					*bit_id = sbit + i;
					return 0;
				}
			}
		}

		bmap += 1;
		bcnt -= 8;
		sbit += 8;
	}

	for (i = 0; i < bcnt; ++i) {
		if (ext4_bmap_is_bit_clr(bmap, i)) {
			*bit_id = sbit + i;
			return 0;
		}
	}

	return ENOSPC;
}

