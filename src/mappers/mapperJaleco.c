/*
 * mapperJaleco.c
 *
 *  Created on: 16/lug/2011
 *      Author: fhorse
 */

#include <stdlib.h>
#include <string.h>
#include "mem_map.h"
#include "mappers.h"
#include "cpu.h"
#include "ppu.h"
#include "save_slot.h"

WORD prgRom32kMax, prgRom16kMax, prgRom8kMax, chrRom8kMax, chrRom1kMax;

#define prgRom8kUpdate(slot, mask, shift)\
	value = (mapper.rom_map_to[slot] & mask) | ((value & 0x0F) << shift);\
	control_bank(prgRom8kMax)\
	map_prg_rom_8k(1, slot, value);\
	map_prg_rom_8k_update()
#define chrRom1kUpdate(slot, mask, shift)\
	value = (ss8806.chrRomBank[slot] & mask) | ((value & 0x0F) << shift);\
	control_bank(chrRom1kMax)\
	chr.bank_1k[slot] = &chr.data[value << 10];\
	ss8806.chrRomBank[slot] = value

void map_init_Jaleco(BYTE model) {
	prgRom32kMax = (info.prg_rom_16k_count >> 1) - 1;
	prgRom16kMax = info.prg_rom_16k_count - 1;
	prgRom8kMax = info.prg_rom_8k_count - 1;
	chrRom8kMax = (info.chr_rom_4k_count >> 1) - 1;
	chrRom1kMax = info.chr_rom_1k_count - 1;

	switch (model) {
		case JF05:
			EXTCL_CPU_WR_MEM(Jaleco_JF05);
			info.mapper_extend_wr = TRUE;
			break;
		case JF11:
			EXTCL_CPU_WR_MEM(Jaleco_JF11);
			info.mapper_extend_wr = TRUE;
			if (info.reset >= HARD) {
				map_prg_rom_8k(4, 0, 0);
			}
			break;
		case JF13:
			EXTCL_CPU_WR_MEM(Jaleco_JF13);
			info.mapper_extend_wr = TRUE;
			if (info.reset >= HARD) {
				map_prg_rom_8k(4, 0, 0);
			}
			break;
		case JF16:
			EXTCL_CPU_WR_MEM(Jaleco_JF16);
			break;
		case JF17:
			EXTCL_CPU_WR_MEM(Jaleco_JF17);
			break;
		case JF19:
			EXTCL_CPU_WR_MEM(Jaleco_JF19);
			break;
		case SS8806:
			EXTCL_CPU_WR_MEM(Jaleco_SS8806);
			EXTCL_SAVE_MAPPER(Jaleco_SS8806);
			EXTCL_CPU_EVERY_CYCLE(Jaleco_SS8806);
			mapper.internal_struct[0] = (BYTE *) &ss8806;
			mapper.internal_struct_size[0] = sizeof(ss8806);

			if (info.reset >= HARD) {
				BYTE i;

				memset(&ss8806, 0x00, sizeof(ss8806));
				ss8806.mask = 0xFFFF;
				ss8806.delay = 255;

				for (i = 0; i < 8; i++) {
					ss8806.chrRomBank[i] = i;
				}
			} else {
				ss8806.mask = 0xFFFF;
				ss8806.reload = 0;
				ss8806.count = 0;
				ss8806.delay = 255;
			}

			switch (info.id) {
				case JAJAMARU:
				case MEZASETOPPRO:
					info.prg_ram_plus_8k_count = 1;
					info.prg_ram_bat_banks = 1;
					break;
			}
			break;
		default:
			break;
	}

}

void extcl_cpu_wr_mem_Jaleco_JF05(WORD address, BYTE value) {
	DBWORD bank;

	if ((address < 0x6000) || (address >= 0x8000)) {
		return;
	}

	value = (((value >> 1) & 0x1) | ((value << 1) & 0x2));
	control_bank_with_AND(0x03, chrRom8kMax)
	bank = value << 13;
	chr.bank_1k[0] = &chr.data[bank];
	chr.bank_1k[1] = &chr.data[bank | 0x0400];
	chr.bank_1k[2] = &chr.data[bank | 0x0800];
	chr.bank_1k[3] = &chr.data[bank | 0x0C00];
	chr.bank_1k[4] = &chr.data[bank | 0x1000];
	chr.bank_1k[5] = &chr.data[bank | 0x1400];
	chr.bank_1k[6] = &chr.data[bank | 0x1800];
	chr.bank_1k[7] = &chr.data[bank | 0x1C00];
}

void extcl_cpu_wr_mem_Jaleco_JF11(WORD address, BYTE value) {
	BYTE save = value;
	DBWORD bank;

	if ((address < 0x6000) || (address >= 0x8000)) {
		return;
	}

	value >>= 4;
	control_bank_with_AND(0x03, prgRom32kMax)
	map_prg_rom_8k(4, 0, value);
	map_prg_rom_8k_update();

	value = save;
	control_bank_with_AND(0x0F, chrRom8kMax)
	bank = value << 13;
	chr.bank_1k[0] = &chr.data[bank];
	chr.bank_1k[1] = &chr.data[bank | 0x0400];
	chr.bank_1k[2] = &chr.data[bank | 0x0800];
	chr.bank_1k[3] = &chr.data[bank | 0x0C00];
	chr.bank_1k[4] = &chr.data[bank | 0x1000];
	chr.bank_1k[5] = &chr.data[bank | 0x1400];
	chr.bank_1k[6] = &chr.data[bank | 0x1800];
	chr.bank_1k[7] = &chr.data[bank | 0x1C00];
}

void extcl_cpu_wr_mem_Jaleco_JF13(WORD address, BYTE value) {
	BYTE save = value;
	DBWORD bank;

	/* FIXME: da 0x7000 a 0x7FFF c'e' la gestione dell'audio */
	if ((address < 0x6000) || (address >= 0x7000)) {
		return;
	}

	value >>= 4;
	control_bank_with_AND(0x03, prgRom32kMax)
	map_prg_rom_8k(4, 0, value);
	map_prg_rom_8k_update();

	value = ((save & 0x40) >> 4) | (save & 0x03);
	control_bank(chrRom8kMax)
	bank = value << 13;
	chr.bank_1k[0] = &chr.data[bank];
	chr.bank_1k[1] = &chr.data[bank | 0x0400];
	chr.bank_1k[2] = &chr.data[bank | 0x0800];
	chr.bank_1k[3] = &chr.data[bank | 0x0C00];
	chr.bank_1k[4] = &chr.data[bank | 0x1000];
	chr.bank_1k[5] = &chr.data[bank | 0x1400];
	chr.bank_1k[6] = &chr.data[bank | 0x1800];
	chr.bank_1k[7] = &chr.data[bank | 0x1C00];
}

void extcl_cpu_wr_mem_Jaleco_JF16(WORD address, BYTE value) {
	BYTE save = value &= prg_rom_rd(address);
	DBWORD bank;

	control_bank_with_AND(0x07, prgRom16kMax)
	map_prg_rom_8k(2, 0, value);
	map_prg_rom_8k_update();

	value = save >> 4;
	control_bank_with_AND(0x0F, chrRom8kMax)
	bank = value << 13;
	chr.bank_1k[0] = &chr.data[bank];
	chr.bank_1k[1] = &chr.data[bank | 0x0400];
	chr.bank_1k[2] = &chr.data[bank | 0x0800];
	chr.bank_1k[3] = &chr.data[bank | 0x0C00];
	chr.bank_1k[4] = &chr.data[bank | 0x1000];
	chr.bank_1k[5] = &chr.data[bank | 0x1400];
	chr.bank_1k[6] = &chr.data[bank | 0x1800];
	chr.bank_1k[7] = &chr.data[bank | 0x1C00];

	if (save & 0x08) {
		mirroring_SCR1();
		if (info.mapper_type == HOLYDIVER) {
			mirroring_V();
		}
	} else {
		mirroring_SCR0();
		if (info.mapper_type == HOLYDIVER) {
			mirroring_H();
		}
	}
}

void extcl_cpu_wr_mem_Jaleco_JF17(WORD address, BYTE value) {
	/* bus conflict */
	BYTE save = value &= prg_rom_rd(address);
	DBWORD bank;

	if (save & 0x80) {
		control_bank_with_AND(0x0F, prgRom16kMax)
		map_prg_rom_8k(2, 0, value);
		map_prg_rom_8k_update();
	}

	if (save & 0x40) {
		value = save;
		control_bank_with_AND(0x0F, chrRom8kMax)
		bank = value << 13;
		chr.bank_1k[0] = &chr.data[bank];
		chr.bank_1k[1] = &chr.data[bank | 0x0400];
		chr.bank_1k[2] = &chr.data[bank | 0x0800];
		chr.bank_1k[3] = &chr.data[bank | 0x0C00];
		chr.bank_1k[4] = &chr.data[bank | 0x1000];
		chr.bank_1k[5] = &chr.data[bank | 0x1400];
		chr.bank_1k[6] = &chr.data[bank | 0x1800];
		chr.bank_1k[7] = &chr.data[bank | 0x1C00];
	}

	/* FIXME : aggiungere l'emulazione del D7756C */
}

void extcl_cpu_wr_mem_Jaleco_JF19(WORD address, BYTE value) {
	/* bus conflict */
	BYTE save = value &= prg_rom_rd(address);
	DBWORD bank;

	if (save & 0x80) {
		control_bank_with_AND(0x0F, prgRom16kMax)
		map_prg_rom_8k(2, 2, value);
		map_prg_rom_8k_update();
	}

	if (save & 0x40) {
		value = save;
		control_bank_with_AND(0x0F, chrRom8kMax)
		bank = value << 13;
		chr.bank_1k[0] = &chr.data[bank];
		chr.bank_1k[1] = &chr.data[bank | 0x0400];
		chr.bank_1k[2] = &chr.data[bank | 0x0800];
		chr.bank_1k[3] = &chr.data[bank | 0x0C00];
		chr.bank_1k[4] = &chr.data[bank | 0x1000];
		chr.bank_1k[5] = &chr.data[bank | 0x1400];
		chr.bank_1k[6] = &chr.data[bank | 0x1800];
		chr.bank_1k[7] = &chr.data[bank | 0x1C00];
	}

	/* FIXME : aggiungere l'emulazione del D7756C */
}

void extcl_cpu_wr_mem_Jaleco_SS8806(WORD address, BYTE value) {
	switch (address) {
		case 0x8000:
			prgRom8kUpdate(0, 0xF0, 0);
			break;
		case 0x8001:
			prgRom8kUpdate(0, 0x0F, 4);
			break;
		case 0x8002:
			prgRom8kUpdate(1, 0xF0, 0);
			break;
		case 0x8003:
			prgRom8kUpdate(1, 0x0F, 4);
			break;
		case 0x9000:
			prgRom8kUpdate(2, 0xF0, 0);
			break;
		case 0x9001:
			prgRom8kUpdate(2, 0x0F, 4);
			break;
		case 0x9002:
			if ((value != 0x03) || !value) {
				break;
			}

			if (value & 0x03) {
				cpu.prg_ram_rd_active = cpu.prg_ram_wr_active = TRUE;
			} else {
				cpu.prg_ram_rd_active = cpu.prg_ram_wr_active = FALSE;
			}
			break;
		case 0xA000:
			chrRom1kUpdate(0, 0xF0, 0);
			break;
		case 0xA001:
			chrRom1kUpdate(0, 0x0F, 4);
			break;
		case 0xA002:
			chrRom1kUpdate(1, 0xF0, 0);
			break;
		case 0xA003:
			chrRom1kUpdate(1, 0x0F, 4);
			break;
		case 0xB000:
			chrRom1kUpdate(2, 0xF0, 0);
			break;
		case 0xB001:
			chrRom1kUpdate(2, 0x0F, 4);
			break;
		case 0xB002:
			chrRom1kUpdate(3, 0xF0, 0);
			break;
		case 0xB003:
			chrRom1kUpdate(3, 0x0F, 4);
			break;
		case 0xC000:
			chrRom1kUpdate(4, 0xF0, 0);
			break;
		case 0xC001:
			chrRom1kUpdate(4, 0x0F, 4);
			break;
		case 0xC002:
			chrRom1kUpdate(5, 0xF0, 0);
			break;
		case 0xC003:
			chrRom1kUpdate(5, 0x0F, 4);
			break;
		case 0xD000:
			chrRom1kUpdate(6, 0xF0, 0);
			break;
		case 0xD001:
			chrRom1kUpdate(6, 0x0F, 4);
			break;
		case 0xD002:
			chrRom1kUpdate(7, 0xF0, 0);
			break;
		case 0xD003:
			chrRom1kUpdate(7, 0x0F, 4);
			break;
		case 0xE000:
			ss8806.reload = (ss8806.reload & 0xFFF0) | (value & 0x0F);
			break;
		case 0xE001:
			ss8806.reload = (ss8806.reload & 0xFF0F) | (value & 0x0F) << 4;
			break;
		case 0xE002:
			ss8806.reload = (ss8806.reload & 0xF0FF) | (value & 0x0F) << 8;
			break;
		case 0xE003:
			ss8806.reload = (ss8806.reload & 0x0FFF) | (value & 0x0F) << 12;
			break;
		case 0xF000:
			ss8806.count = ss8806.reload;
			irq.high &= ~EXT_IRQ;
			break;
		case 0xF001:
			ss8806.enabled = value & 0x01;
			if (value & 0x8) {
				ss8806.mask = 0x000F;
			} else if (value & 0x4) {
				ss8806.mask = 0x00FF;
			} else if (value & 0x2) {
				ss8806.mask = 0x0FFF;
			} else {
				ss8806.mask = 0xFFFF;
			}
			irq.high &= ~EXT_IRQ;
			break;
		case 0xF002:
			if (value & 0x02) {
				mirroring_SCR0();
			} else if (value & 0x01) {
				mirroring_V();
			} else {
				mirroring_H();
			}
			break;
	}
}
BYTE extcl_save_mapper_Jaleco_SS8806(BYTE mode, BYTE slot, FILE *fp) {
	save_slot_ele(mode, slot, ss8806.chrRomBank);
	save_slot_ele(mode, slot, ss8806.enabled);
	save_slot_ele(mode, slot, ss8806.mask);
	save_slot_ele(mode, slot, ss8806.reload);
	save_slot_ele(mode, slot, ss8806.count);
	save_slot_ele(mode, slot, ss8806.delay);

	return (EXIT_OK);
}
void extcl_cpu_every_cycle_Jaleco_SS8806(void) {
	if (!ss8806.enabled) {
		return;
	}

	/* gestisco questo delay sempre per la sincronizzazzione con la CPU */
	if (ss8806.delay != 255) {
		if (!(--ss8806.delay)) {
			irq.delay = TRUE;
			irq.high |= EXT_IRQ;
			ss8806.delay = 255;
		}
	}

	if ((ss8806.count & ss8806.mask) && !(--ss8806.count & ss8806.mask)) {
		ss8806.delay = 1;
	}
}
