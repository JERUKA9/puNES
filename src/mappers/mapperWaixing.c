/*
 * mapperWaixing.c
 *
 *  Created on: 11/set/2011
 *      Author: fhorse
 */

#include <stdlib.h>
#include <string.h>
#include "mappers.h"
#include "mem_map.h"
#include "irqA12.h"
#include "save_slot.h"

WORD prg_rom_16k_max, prg_rom_8k_before_last, prg_rom_8k_max, chr_rom_4k_max, chr_rom_1k_max;
BYTE min, max;

#define waixing_swap_chr_bank_1k(src, dst)\
{\
	BYTE *chr_bank_1k = chr.bank_1k[src];\
	chr.bank_1k[src] = chr.bank_1k[dst];\
	chr.bank_1k[dst] = chr_bank_1k;\
	WORD map = waixing.chr_map[src];\
	waixing.chr_map[src] = waixing.chr_map[dst];\
	waixing.chr_map[dst] = map;\
}
#define waixing_8000(function)\
{\
	const BYTE chr_rom_cfg_old = mmc3.chr_rom_cfg;\
	const BYTE prg_rom_cfg_old = mmc3.prg_rom_cfg;\
	function;\
	mmc3.prg_rom_cfg = (value & 0x40) >> 5;\
	mmc3.chr_rom_cfg = (value & 0x80) >> 5;\
	/*\
	 * se il tipo di configurazione della chr cambia,\
	 * devo swappare i primi 4 banchi con i secondi\
	 * quattro.\
	 */\
	if (mmc3.chr_rom_cfg != chr_rom_cfg_old) {\
		waixing_swap_chr_bank_1k(0, 4)\
		waixing_swap_chr_bank_1k(1, 5)\
		waixing_swap_chr_bank_1k(2, 6)\
		waixing_swap_chr_bank_1k(3, 7)\
	}\
	if (mmc3.prg_rom_cfg != prg_rom_cfg_old) {\
		WORD p0 = mapper.rom_map_to[0];\
		WORD p2 = mapper.rom_map_to[2];\
		mapper.rom_map_to[0] = p2;\
		mapper.rom_map_to[2] = p0;\
		/*\
		 * prg_rom_cfg 0x00 : $C000 - $DFFF fisso al penultimo banco\
		 * prg_rom_cfg 0x02 : $8000 - $9FFF fisso al penultimo banco\
		 */\
		map_prg_rom_8k(1, mmc3.prg_rom_cfg ^ 0x02, prg_rom_8k_before_last);\
		map_prg_rom_8k_update();\
	}\
}

#define waixing_type_ACDE_chr_1k(a)\
	if ((value >= min) && (value <= max)) {\
		chr.bank_1k[a] = &waixing.chr_ram[(value - min) << 10];\
	} else {\
		chr.bank_1k[a] = &chr.data[value << 10];\
	}
#define waixing_type_ACDE_8001()\
{\
	switch (mmc3.bank_to_update) {\
		case 0:\
			waixing.chr_map[mmc3.chr_rom_cfg] = value;\
			waixing.chr_map[mmc3.chr_rom_cfg | 0x01] = value + 1;\
			control_bank_with_AND(0xFE, chr_rom_1k_max)\
			waixing_type_ACDE_chr_1k(mmc3.chr_rom_cfg)\
			value++;\
			waixing_type_ACDE_chr_1k(mmc3.chr_rom_cfg | 0x01)\
			return;\
		case 1:\
			waixing.chr_map[mmc3.chr_rom_cfg | 0x02] = value;\
			waixing.chr_map[mmc3.chr_rom_cfg | 0x03] = value + 1;\
			control_bank_with_AND(0xFE, chr_rom_1k_max)\
			waixing_type_ACDE_chr_1k(mmc3.chr_rom_cfg | 0x02)\
			value++;\
			waixing_type_ACDE_chr_1k(mmc3.chr_rom_cfg | 0x03)\
			return;\
		case 2:\
			waixing.chr_map[mmc3.chr_rom_cfg ^ 0x04] = value;\
			control_bank(chr_rom_1k_max)\
			waixing_type_ACDE_chr_1k(mmc3.chr_rom_cfg ^ 0x04)\
			return;\
		case 3:\
			waixing.chr_map[(mmc3.chr_rom_cfg ^ 0x04) | 0x01] = value;\
			control_bank(chr_rom_1k_max)\
			waixing_type_ACDE_chr_1k((mmc3.chr_rom_cfg ^ 0x04) | 0x01)\
			return;\
		case 4:\
			waixing.chr_map[(mmc3.chr_rom_cfg ^ 0x04) | 0x02] = value;\
			control_bank(chr_rom_1k_max)\
			waixing_type_ACDE_chr_1k((mmc3.chr_rom_cfg ^ 0x04) | 0x02)\
			return;\
		case 5:\
			waixing.chr_map[(mmc3.chr_rom_cfg ^ 0x04) | 0x03] = value;\
			control_bank(chr_rom_1k_max)\
			waixing_type_ACDE_chr_1k((mmc3.chr_rom_cfg ^ 0x04) | 0x03)\
			return;\
	}\
}

#define waixing_type_B_chr_1k(a)\
	if (save & 0x80) {\
		chr.bank_1k[a] = &waixing.chr_ram[value << 10];\
	} else {\
		chr.bank_1k[a] = &chr.data[value << 10];\
	}
#define waixing_type_B_8001()\
{\
	switch (mmc3.bank_to_update) {\
		case 0:\
			waixing.chr_map[mmc3.chr_rom_cfg] = save;\
			waixing.chr_map[mmc3.chr_rom_cfg | 0x01] = save + 1;\
			control_bank_with_AND(0xFE, chr_rom_1k_max)\
			waixing_type_B_chr_1k(mmc3.chr_rom_cfg)\
			value++;\
			waixing_type_B_chr_1k(mmc3.chr_rom_cfg | 0x01)\
			return;\
		case 1:\
			waixing.chr_map[mmc3.chr_rom_cfg | 0x02] = save;\
			waixing.chr_map[mmc3.chr_rom_cfg | 0x03] = save + 1;\
			control_bank_with_AND(0xFE, chr_rom_1k_max)\
			waixing_type_B_chr_1k(mmc3.chr_rom_cfg | 0x02)\
			value++;\
			waixing_type_B_chr_1k(mmc3.chr_rom_cfg | 0x03)\
			return;\
		case 2:\
			waixing.chr_map[mmc3.chr_rom_cfg ^ 0x04] = save;\
			control_bank(chr_rom_1k_max)\
			waixing_type_B_chr_1k(mmc3.chr_rom_cfg ^ 0x04)\
			return;\
		case 3:\
			waixing.chr_map[(mmc3.chr_rom_cfg ^ 0x04) | 0x01] = save;\
			control_bank(chr_rom_1k_max)\
			waixing_type_B_chr_1k((mmc3.chr_rom_cfg ^ 0x04) | 0x01)\
			return;\
		case 4:\
			waixing.chr_map[(mmc3.chr_rom_cfg ^ 0x04) | 0x02] = save;\
			control_bank(chr_rom_1k_max)\
			waixing_type_B_chr_1k((mmc3.chr_rom_cfg ^ 0x04) | 0x02)\
			return;\
		case 5:\
			waixing.chr_map[(mmc3.chr_rom_cfg ^ 0x04) | 0x03] = save;\
			control_bank(chr_rom_1k_max)\
			waixing_type_B_chr_1k((mmc3.chr_rom_cfg ^ 0x04) | 0x03)\
			return;\
	}\
}

#define waixing_type_G_chr_1k(a)\
	if (save < 8) {\
		chr.bank_1k[a] = &waixing.chr_ram[save << 10];\
	} else {\
		chr.bank_1k[a] = &chr.data[value << 10];\
	}
#define Waixing_type_G_8001()\
{\
	switch (mmc3.bank_to_update) {\
		case 0:\
			waixing.chr_map[0] = value;\
			control_bank(chr_rom_1k_max)\
			waixing_type_G_chr_1k(0)\
			return;\
		case 1:\
			waixing.chr_map[2] = value;\
			control_bank(chr_rom_1k_max)\
			waixing_type_G_chr_1k(2)\
			return;\
		case 2:\
			waixing.chr_map[4] = value;\
			control_bank(chr_rom_1k_max)\
			waixing_type_G_chr_1k(4)\
			return;\
		case 3:\
			waixing.chr_map[5] = value;\
			control_bank(chr_rom_1k_max)\
			waixing_type_G_chr_1k(5)\
			return;\
		case 4:\
			waixing.chr_map[6] = value;\
			control_bank(chr_rom_1k_max)\
			waixing_type_G_chr_1k(6)\
			return;\
		case 5:\
			waixing.chr_map[7] = value;\
			control_bank(chr_rom_1k_max)\
			waixing_type_G_chr_1k(7)\
			return;\
		case 6:\
			control_bank(prg_rom_8k_max)\
			map_prg_rom_8k(1, 0, value);\
			map_prg_rom_8k_update();\
			return;\
		case 7:\
			control_bank(prg_rom_8k_max)\
			map_prg_rom_8k(1, 1, value);\
			map_prg_rom_8k_update();\
			return;\
		case 8:\
			control_bank(prg_rom_8k_max)\
			map_prg_rom_8k(1, 2, value);\
			map_prg_rom_8k_update();\
			return;\
		case 9:\
			control_bank(prg_rom_8k_max)\
			map_prg_rom_8k(1, 3, value);\
			map_prg_rom_8k_update();\
			return;\
		case 10:\
			waixing.chr_map[1] = value;\
			control_bank(chr_rom_1k_max)\
			waixing_type_G_chr_1k(1)\
			return;\
		case 11:\
			waixing.chr_map[3] = value;\
			control_bank(chr_rom_1k_max)\
			waixing_type_G_chr_1k(3)\
			return;\
	}\
}

#define waixing_type_H_chr_1k(a)\
	if (mapper.write_vram) {\
		chr.bank_1k[a] = &waixing.chr_ram[(value & 0x07) << 10];\
	} else {\
		control_bank(chr_rom_1k_max)\
		chr.bank_1k[a] = &chr.data[value << 10];\
	}
#define waixing_type_H_prg_8k(vl)\
	value = (vl & 0x3F) | ((waixing.ctrl[0] & 0x02) << 5)
#define waixing_type_H_prg_8k_update()\
{\
	BYTE i;\
	for (i = 0; i < 4; i++) {\
		waixing_type_H_prg_8k(waixing.prg_map[i]);\
		control_bank(prg_rom_8k_max)\
		map_prg_rom_8k(1, i, value);\
	}\
	map_prg_rom_8k_update();\
}
#define waixing_type_H_8000()\
{\
	const BYTE chr_rom_cfg_old = mmc3.chr_rom_cfg;\
	const BYTE prg_rom_cfg_old = mmc3.prg_rom_cfg;\
	mmc3.bank_to_update = value & 0x07;\
	mmc3.prg_rom_cfg = (value & 0x40) >> 5;\
	mmc3.chr_rom_cfg = (value & 0x80) >> 5;\
	/*\
	 * se il tipo di configurazione della chr cambia,\
	 * devo swappare i primi 4 banchi con i secondi\
	 * quattro.\
	 */\
	if (mmc3.chr_rom_cfg != chr_rom_cfg_old) {\
		waixing_swap_chr_bank_1k(0, 4)\
		waixing_swap_chr_bank_1k(1, 5)\
		waixing_swap_chr_bank_1k(2, 6)\
		waixing_swap_chr_bank_1k(3, 7)\
	}\
	if (mmc3.prg_rom_cfg != prg_rom_cfg_old) {\
		WORD p0 = mapper.rom_map_to[0];\
		WORD p2 = mapper.rom_map_to[2];\
		mapper.rom_map_to[0] = p2;\
		mapper.rom_map_to[2] = p0;\
		p0 = waixing.prg_map[0];\
		p2 = waixing.prg_map[2];\
		waixing.prg_map[0] = p2;\
		waixing.prg_map[2] = p0;\
		waixing.prg_map[mmc3.prg_rom_cfg ^ 0x02] = prg_rom_8k_before_last;\
		/*\
		 * prg_rom_cfg 0x00 : $C000 - $DFFF fisso al penultimo banco\
		 * prg_rom_cfg 0x02 : $8000 - $9FFF fisso al penultimo banco\
		 */\
		waixing_type_H_prg_8k(prg_rom_8k_before_last);\
		control_bank(prg_rom_8k_max)\
		map_prg_rom_8k(1, mmc3.prg_rom_cfg ^ 0x02, value);\
		map_prg_rom_8k_update();\
	}\
}
#define waixing_type_H_8001()\
{\
	switch (mmc3.bank_to_update) {\
		case 0:\
			value &= 0xFE;\
			waixing.chr_map[mmc3.chr_rom_cfg] = value;\
			waixing.chr_map[mmc3.chr_rom_cfg | 0x01] = value + 1;\
			waixing_type_H_chr_1k(mmc3.chr_rom_cfg)\
			value++;\
			waixing_type_H_chr_1k(mmc3.chr_rom_cfg | 0x01)\
			if (mapper.write_vram) {\
				waixing.ctrl[0] = save;\
				waixing_type_H_prg_8k_update()\
			}\
			return;\
		case 1:\
			value &= 0xFE;\
			waixing.chr_map[mmc3.chr_rom_cfg | 0x02] = value;\
			waixing.chr_map[mmc3.chr_rom_cfg | 0x03] = value + 1;\
			waixing_type_H_chr_1k(mmc3.chr_rom_cfg | 0x02)\
			value++;\
			waixing_type_H_chr_1k(mmc3.chr_rom_cfg | 0x03)\
			waixing_type_H_prg_8k_update()\
			return;\
		case 2:\
			waixing.chr_map[mmc3.chr_rom_cfg ^ 0x04] = value;\
			waixing_type_H_chr_1k(mmc3.chr_rom_cfg ^ 0x04)\
			waixing_type_H_prg_8k_update()\
			return;\
		case 3:\
			waixing.chr_map[(mmc3.chr_rom_cfg ^ 0x04) | 0x01] = value;\
			waixing_type_H_chr_1k((mmc3.chr_rom_cfg ^ 0x04) | 0x01)\
			waixing_type_H_prg_8k_update()\
			return;\
		case 4:\
			waixing.chr_map[(mmc3.chr_rom_cfg ^ 0x04) | 0x02] = value;\
			waixing_type_H_chr_1k((mmc3.chr_rom_cfg ^ 0x04) | 0x02)\
			waixing_type_H_prg_8k_update()\
			return;\
		case 5:\
			waixing.chr_map[(mmc3.chr_rom_cfg ^ 0x04) | 0x03] = value;\
			waixing_type_H_chr_1k((mmc3.chr_rom_cfg ^ 0x04) | 0x03)\
			waixing_type_H_prg_8k_update()\
			return;\
		case 6:\
			waixing.prg_map[mmc3.prg_rom_cfg] = value;\
			waixing_type_H_prg_8k(value);\
			control_bank(prg_rom_8k_max)\
			map_prg_rom_8k(1, mmc3.prg_rom_cfg, value);\
			map_prg_rom_8k_update();\
			return;\
		case 7:\
			waixing.prg_map[1] = value;\
			waixing_type_H_prg_8k(value);\
			control_bank(prg_rom_8k_max)\
			map_prg_rom_8k(1, 1, value);\
			map_prg_rom_8k_update();\
			return;\
	}\
}

#define waixing_SH2_chr_4k(a, v)\
{\
	if (!v) {\
		waixing.ctrl[a >> 2] = 0;\
		chr.bank_1k[a] = &waixing.chr_ram[0];\
		chr.bank_1k[a | 0x01] = &waixing.chr_ram[0x0400];\
		chr.bank_1k[a | 0x02] = &waixing.chr_ram[0x0800];\
		chr.bank_1k[a | 0x03] = &waixing.chr_ram[0x0C00];\
	} else {\
		DBWORD bank = v << 12;\
		waixing.ctrl[a >> 2] = 1;\
		chr.bank_1k[a] = &chr.data[bank];\
		chr.bank_1k[a | 0x01] = &chr.data[bank | 0x0400];\
		chr.bank_1k[a | 0x02] = &chr.data[bank | 0x0800];\
		chr.bank_1k[a | 0x03] = &chr.data[bank | 0x0C00];\
	}\
}
#define waixing_SH2_PPUFD()\
	if (waixing.reg == 0xFD) {\
		waixing_SH2_chr_4k(0, waixing.chr_map[0])\
		waixing_SH2_chr_4k(4, waixing.chr_map[2])\
	}
#define waixing_SH2_PPUFE()\
	if (waixing.reg == 0xFE) {\
		waixing_SH2_chr_4k(0, waixing.chr_map[1])\
		waixing_SH2_chr_4k(4, waixing.chr_map[4])\
	}
#define waixing_SH2_PPU(a)\
	if ((a & 0x1FF0) == 0x1FD0) {\
		waixing.reg = 0xFD;\
		waixing_SH2_PPUFD()\
	} else if ((a & 0x1FF0) == 0x1FE0) {\
		waixing.reg = 0xFE;\
		waixing_SH2_PPUFE()\
	}
#define waixing_SH2_8000()\
{\
	const BYTE prg_rom_cfg_old = mmc3.prg_rom_cfg;\
	mmc3.bank_to_update = value & 0x07;\
	mmc3.prg_rom_cfg = (value & 0x40) >> 5;\
	mmc3.chr_rom_cfg = (value & 0x80) >> 5;\
	if (mmc3.prg_rom_cfg != prg_rom_cfg_old) {\
		WORD p0 = mapper.rom_map_to[0];\
		WORD p2 = mapper.rom_map_to[2];\
		mapper.rom_map_to[0] = p2;\
		mapper.rom_map_to[2] = p0;\
		/*\
		 * prg_rom_cfg 0x00 : $C000 - $DFFF fisso al penultimo banco\
		 * prg_rom_cfg 0x02 : $8000 - $9FFF fisso al penultimo banco\
		 */\
		map_prg_rom_8k(1, mmc3.prg_rom_cfg ^ 0x02, prg_rom_8k_before_last);\
		map_prg_rom_8k_update();\
	}\
}
#define waixing_SH2_8001()\
{\
	switch (mmc3.bank_to_update) {\
		case 0:\
			waixing.chr_map[0] = value >> 2;\
			_control_bank(waixing.chr_map[0], chr_rom_4k_max)\
			waixing_SH2_PPUFD()\
			return;\
		case 1:\
			waixing.chr_map[1] = value >> 2;\
			_control_bank(waixing.chr_map[1], chr_rom_4k_max)\
			waixing_SH2_PPUFE()\
			return;\
		case 2:\
			waixing.chr_map[2] = value >> 2;\
			_control_bank(waixing.chr_map[2], chr_rom_4k_max)\
			waixing_SH2_PPUFD()\
			return;\
		case 3:\
			return;\
		case 4:\
			waixing.chr_map[4] = value >> 2;\
			_control_bank(waixing.chr_map[4], chr_rom_4k_max)\
			waixing_SH2_PPUFE()\
			return;\
		case 5:\
			return;\
	}\
}

void map_init_Waixing(BYTE model) {
	prg_rom_16k_max = info.prg_rom_16k_count - 1;
	prg_rom_8k_max = info.prg_rom_8k_count - 1;
	prg_rom_8k_before_last = info.prg_rom_8k_count - 2;
	chr_rom_4k_max = info.chr_rom_4k_count - 1;
	chr_rom_1k_max = info.chr_rom_1k_count - 1;

	switch (model) {
		case WPSX:
			EXTCL_CPU_WR_MEM(Waixing_PSx);

			map_prg_rom_8k(4, 0, 0);
			break;
		case WTB:
			EXTCL_CPU_WR_MEM(Waixing_type_B);
			EXTCL_SAVE_MAPPER(Waixing_type_B);
			EXTCL_WR_CHR(Waixing_type_B);
			EXTCL_CPU_EVERY_CYCLE(MMC3);
			EXTCL_PPU_000_TO_34X(MMC3);
			EXTCL_PPU_000_TO_255(MMC3);
			EXTCL_PPU_256_TO_319(MMC3);
			EXTCL_PPU_320_TO_34X(MMC3);
			EXTCL_UPDATE_R2006(MMC3);
			mapper.internal_struct[0] = (BYTE *) &waixing;
			mapper.internal_struct_size[0] = sizeof(waixing);
			mapper.internal_struct[1] = (BYTE *) &mmc3;
			mapper.internal_struct_size[1] = sizeof(mmc3);

			if (info.reset >= HARD) {
				memset(&mmc3, 0x00, sizeof(mmc3));
				memset(&irqA12, 0x00, sizeof(irqA12));
				memset(&waixing, 0x00, sizeof(waixing));

				{
					BYTE i;

					chr_bank_1k_reset()

					for (i = 0; i < 8; i++) {
						waixing.chr_map[i] = i;
					}
				}
			}

			irqA12.present = TRUE;
			irqA12_delay = 1;
			break;
		case WTA:
		case WTC:
		case WTD:
		case WTE:
			if (model == WTA) {
				min = 0x08;
				max = 0x09;
			} else if (model == WTC) {
				min = 0x08;
				max = 0x0B;
			} else if (model == WTD) {
				min = 0x00;
				max = 0x01;
			} else if (model == WTE) {
				min = 0x00;
				max = 0x03;
			}
			EXTCL_CPU_WR_MEM(Waixing_type_ACDE);
			EXTCL_SAVE_MAPPER(Waixing_type_ACDE);
			EXTCL_WR_CHR(Waixing_type_ACDE);
			EXTCL_CPU_EVERY_CYCLE(MMC3);
			EXTCL_PPU_000_TO_34X(MMC3);
			EXTCL_PPU_000_TO_255(MMC3);
			EXTCL_PPU_256_TO_319(MMC3);
			EXTCL_PPU_320_TO_34X(MMC3);
			EXTCL_UPDATE_R2006(MMC3);
			mapper.internal_struct[0] = (BYTE *) &waixing;
			mapper.internal_struct_size[0] = sizeof(waixing);
			mapper.internal_struct[1] = (BYTE *) &mmc3;
			mapper.internal_struct_size[1] = sizeof(mmc3);

			if (info.reset >= HARD) {
				memset(&mmc3, 0x00, sizeof(mmc3));
				memset(&irqA12, 0x00, sizeof(irqA12));
				memset(&waixing, 0x00, sizeof(waixing));

				{
					BYTE i;

					chr_bank_1k_reset()

					for (i = 0; i < 8; i++) {
						waixing.chr_map[i] = i;

						if ((waixing.chr_map[i] >= min) && (waixing.chr_map[i] <= max)) {
							chr.bank_1k[i] = &waixing.chr_ram[(waixing.chr_map[i] - min) << 10];
						}
					}
				}
			}

			irqA12.present = TRUE;
			irqA12_delay = 1;
			break;
		case WTG:
			EXTCL_CPU_WR_MEM(Waixing_type_G);
			EXTCL_SAVE_MAPPER(Waixing_type_G);
			EXTCL_WR_CHR(Waixing_type_G);
			EXTCL_CPU_EVERY_CYCLE(MMC3);
			EXTCL_PPU_000_TO_34X(MMC3);
			EXTCL_PPU_000_TO_255(MMC3);
			EXTCL_PPU_256_TO_319(MMC3);
			EXTCL_PPU_320_TO_34X(MMC3);
			EXTCL_UPDATE_R2006(MMC3);
			mapper.internal_struct[0] = (BYTE *) &waixing;
			mapper.internal_struct_size[0] = sizeof(waixing);
			mapper.internal_struct[1] = (BYTE *) &mmc3;
			mapper.internal_struct_size[1] = sizeof(mmc3);

			if (info.reset >= HARD) {
				memset(&mmc3, 0x00, sizeof(mmc3));
				memset(&irqA12, 0x00, sizeof(irqA12));
				memset(&waixing, 0x00, sizeof(waixing));

				{
					BYTE i;

					chr_bank_1k_reset()

					for (i = 0; i < 8; i++) {
						waixing.chr_map[i] = i;

						if (waixing.chr_map[i] < 8) {
							chr.bank_1k[i] = &waixing.chr_ram[waixing.chr_map[i] << 10];
						}
					}
				}
			}

			irqA12.present = TRUE;
			irqA12_delay = 1;
			break;
		case WTH:
			EXTCL_CPU_WR_MEM(Waixing_type_H);
			EXTCL_SAVE_MAPPER(Waixing_type_H);
			EXTCL_CPU_EVERY_CYCLE(MMC3);
			EXTCL_PPU_000_TO_34X(MMC3);
			EXTCL_PPU_000_TO_255(MMC3);
			EXTCL_PPU_256_TO_319(MMC3);
			EXTCL_PPU_320_TO_34X(MMC3);
			EXTCL_UPDATE_R2006(MMC3);
			mapper.internal_struct[0] = (BYTE *) &waixing;
			mapper.internal_struct_size[0] = sizeof(waixing);
			mapper.internal_struct[1] = (BYTE *) &mmc3;
			mapper.internal_struct_size[1] = sizeof(mmc3);

			if (info.reset >= HARD) {
				memset(&mmc3, 0x00, sizeof(mmc3));
				memset(&irqA12, 0x00, sizeof(irqA12));
				memset(&waixing, 0x00, sizeof(waixing));

				{
					BYTE i, value;

					map_prg_rom_8k_reset();
					chr_bank_1k_reset()

					for (i = 0; i < 8; i++) {
						if (i < 4) {
							waixing.prg_map[i] = mapper.rom_map_to[i];
						}

						waixing.chr_map[i] = i;

						if (mapper.write_vram) {
							chr.bank_1k[i] = &waixing.chr_ram[waixing.chr_map[i] << 10];
						}
					}

					waixing_type_H_prg_8k_update()
				}
			}

			irqA12.present = TRUE;
			irqA12_delay = 1;
			break;
		case SH2:
			EXTCL_CPU_WR_MEM(Waixing_SH2);
			EXTCL_SAVE_MAPPER(Waixing_SH2);
			EXTCL_AFTER_RD_CHR(Waixing_SH2);
			EXTCL_UPDATE_R2006(Waixing_SH2);
			EXTCL_WR_CHR(Waixing_SH2);
			EXTCL_CPU_EVERY_CYCLE(MMC3);
			EXTCL_PPU_000_TO_34X(MMC3);
			EXTCL_PPU_000_TO_255(MMC3);
			EXTCL_PPU_256_TO_319(MMC3);
			EXTCL_PPU_320_TO_34X(MMC3);
			mapper.internal_struct[0] = (BYTE *) &waixing;
			mapper.internal_struct_size[0] = sizeof(waixing);
			mapper.internal_struct[1] = (BYTE *) &mmc3;
			mapper.internal_struct_size[1] = sizeof(mmc3);

			if (info.reset >= HARD) {
				memset(&mmc3, 0x00, sizeof(mmc3));
				memset(&irqA12, 0x00, sizeof(irqA12));
				memset(&waixing, 0x00, sizeof(waixing));

				waixing.reg = 0xFD;

				waixing.ctrl[0] = 1;
				waixing.ctrl[1] = 1;

				map_prg_rom_8k_reset();
				chr_bank_1k_reset()

				waixing.chr_map[0] = 0;
				waixing.chr_map[1] = 0;
				waixing.chr_map[2] = 0;
				waixing.chr_map[4] = 0;

				waixing_SH2_PPUFD();
			}

			irqA12.present = TRUE;
			irqA12_delay = 1;
			break;
	}
}

void extcl_cpu_wr_mem_Waixing_PSx(WORD address, BYTE value) {
	BYTE swap = value >> 7;

	if (value & 0x40) {
		mirroring_H();
	} else {
		mirroring_V();
	}

	value &= 0x3F;

	switch (address & 0x0003) {
		case 0x0000:
			control_bank(prg_rom_16k_max)
			map_prg_rom_8k(2, 0, value);
			value++;
			control_bank(prg_rom_16k_max)
			map_prg_rom_8k(2, 2, value);
			break;
		case 0x0001:
			control_bank(prg_rom_16k_max)
			map_prg_rom_8k(2, 0, value);
			map_prg_rom_8k(2, 2, prg_rom_16k_max);
			break;
		case 0x0002:
			value = (value << 1) | swap;
			control_bank(prg_rom_8k_max)
			map_prg_rom_8k(1, 0, value);
			map_prg_rom_8k(1, 1, value);
			map_prg_rom_8k(1, 2, value);
			map_prg_rom_8k(1, 3, value);
			break;
		case 0x0003:
			control_bank(prg_rom_16k_max)
			map_prg_rom_8k(2, 0, value);
			map_prg_rom_8k(2, 2, value);
			break;
	}
	map_prg_rom_8k_update();
}

void extcl_cpu_wr_mem_Waixing_type_ACDE(WORD address, BYTE value) {
	BYTE save = value;

	switch (address & 0xE001) {
		case 0x8000:
			waixing_8000(mmc3.bank_to_update = value & 0x07)
			return;
		case 0x8001:
			waixing_type_ACDE_8001()
			break;
		case 0xA001:
			return;
	}
	extcl_cpu_wr_mem_MMC3(address, save);
}
BYTE extcl_save_mapper_Waixing_type_ACDE(BYTE mode, BYTE slot, FILE *fp) {
	save_slot_ele(mode, slot, waixing.chr_map);
	save_slot_ele(mode, slot, waixing.chr_ram);
	extcl_save_mapper_MMC3(mode, slot, fp);

	if (mode == SAVE_SLOT_READ) {
		BYTE i;

		for (i = 0; i < 8; i++) {
			if ((waixing.chr_map[i] >= min) && (waixing.chr_map[i] <= max)) {
				chr.bank_1k[i] = &waixing.chr_ram[(waixing.chr_map[i] - min) << 10];
			}
		}
	}

	return (EXIT_OK);
}
void extcl_wr_chr_Waixing_type_ACDE(WORD address, BYTE value) {
	const BYTE slot = address >> 10;

	if ((waixing.chr_map[slot] >= min) && (waixing.chr_map[slot] <= max)) {
		chr.bank_1k[slot][address & 0x3FF] = value;
	}
}

void extcl_cpu_wr_mem_Waixing_type_B(WORD address, BYTE value) {
	BYTE save = value;

	switch (address & 0xE001) {
		case 0x8000:
			waixing_8000(mmc3.bank_to_update = value & 0x07)
			return;
		case 0x8001:
			waixing_type_B_8001()
			break;
		case 0xA001:
			return;
	}
	extcl_cpu_wr_mem_MMC3(address, save);
}
BYTE extcl_save_mapper_Waixing_type_B(BYTE mode, BYTE slot, FILE *fp) {
	save_slot_ele(mode, slot, waixing.chr_map);
	save_slot_ele(mode, slot, waixing.chr_ram);
	extcl_save_mapper_MMC3(mode, slot, fp);

	if (mode == SAVE_SLOT_READ) {
		BYTE i;

		for (i = 0; i < 8; i++) {
			if (waixing.chr_map[i] & 0x80) {
				BYTE value = waixing.chr_map[i];

				control_bank(chr_rom_1k_max)
				chr.bank_1k[i] = &waixing.chr_ram[value << 10];
			}
		}
	}

	return (EXIT_OK);
}
void extcl_wr_chr_Waixing_type_B(WORD address, BYTE value) {
	const BYTE slot = address >> 10;

	if (waixing.chr_map[slot] & 0x80) {
		chr.bank_1k[slot][address & 0x3FF] = value;
	}
}

void extcl_cpu_wr_mem_Waixing_type_G(WORD address, BYTE value) {
	BYTE save = value;

	switch (address & 0xE001) {
		case 0x8000:
			waixing_8000(mmc3.bank_to_update = value & 0x0F)
			return;
		case 0x8001:
			Waixing_type_G_8001()
			return;
		case 0xA000:
			switch (value & 0x03) {
				case 0:
					mirroring_V();
					break;
				case 1:
					mirroring_H();
					break;
				case 2:
					mirroring_SCR0();
					break;
				case 3:
					mirroring_SCR1();
					break;
			}
			return;
		case 0xA001:
			return;
	}
	extcl_cpu_wr_mem_MMC3(address, save);
}
BYTE extcl_save_mapper_Waixing_type_G(BYTE mode, BYTE slot, FILE *fp) {
	save_slot_ele(mode, slot, waixing.chr_map);
	save_slot_ele(mode, slot, waixing.chr_ram);
	extcl_save_mapper_MMC3(mode, slot, fp);

	if (mode == SAVE_SLOT_READ) {
		BYTE i;

		for (i = 0; i < 8; i++) {
			if (waixing.chr_map[i] < 8) {
				chr.bank_1k[i] = &waixing.chr_ram[waixing.chr_map[i] << 10];
			}
		}
	}

	return (EXIT_OK);
}
void extcl_wr_chr_Waixing_type_G(WORD address, BYTE value) {
	const BYTE slot = address >> 10;

	if (waixing.chr_map[slot] < 8) {
		chr.bank_1k[slot][address & 0x3FF] = value;
	}
}

void extcl_cpu_wr_mem_Waixing_type_H(WORD address, BYTE value) {
	BYTE save = value;

	switch (address & 0xE001) {
		case 0x8000:
			waixing_type_H_8000()
			return;
		case 0x8001:
			waixing_type_H_8001()
			return;
	}
	extcl_cpu_wr_mem_MMC3(address, save);
}
BYTE extcl_save_mapper_Waixing_type_H(BYTE mode, BYTE slot, FILE *fp) {
	save_slot_ele(mode, slot, waixing.prg_map);
	save_slot_ele(mode, slot, waixing.chr_map);
	save_slot_ele(mode, slot, waixing.chr_ram);
	save_slot_ele(mode, slot, waixing.ctrl);
	extcl_save_mapper_MMC3(mode, slot, fp);

	if ((mode == SAVE_SLOT_READ) && mapper.write_vram) {
		BYTE i;

		for (i = 0; i < 8; i++) {
			chr.bank_1k[i] = &waixing.chr_ram[waixing.chr_map[i] << 10];
		}
	}

	return (EXIT_OK);
}

void extcl_cpu_wr_mem_Waixing_SH2(WORD address, BYTE value) {
	BYTE save = value;

	switch (address & 0xE001) {
		case 0x8000:
			waixing_SH2_8000()
			return;
		case 0x8001:
			waixing_SH2_8001()
			break;
	}
	extcl_cpu_wr_mem_MMC3(address, save);
}
BYTE extcl_save_mapper_Waixing_SH2(BYTE mode, BYTE slot, FILE *fp) {
	save_slot_ele(mode, slot, waixing.chr_map);
	save_slot_ele(mode, slot, waixing.chr_ram);
	save_slot_ele(mode, slot, waixing.reg);
	save_slot_ele(mode, slot, waixing.ctrl);
	extcl_save_mapper_MMC3(mode, slot, fp);

	if (mode == SAVE_SLOT_READ) {
		waixing_SH2_PPUFD()
		waixing_SH2_PPUFE()
	}

	return (EXIT_OK);
}
void extcl_after_rd_chr_Waixing_SH2(WORD address) {
	waixing_SH2_PPU(address)
}
void extcl_update_r2006_Waixing_SH2(WORD old_r2006) {
	/* questo e' per l'MMC3 */
	irqA12_IO(old_r2006);

	if (r2006.value >= 0x2000) {
		return;
	}

	waixing_SH2_PPU(r2006.value)
}
void extcl_wr_chr_Waixing_SH2(WORD address, BYTE value) {
	if (!waixing.ctrl[address >> 12]) {
		chr.bank_1k[address >> 10][address & 0x3FF] = value;
	}
}
