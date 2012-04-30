/*
 * mapper52.h
 *
 *  Created on: 21/apr/2012
 *      Author: fhorse
 */

#ifndef MAPPER52_H_
#define MAPPER52_H_

#include "common.h"

struct _m52 {
	BYTE disabled;
	BYTE reg;
	WORD prgmap[4];
	WORD chrmap[8];
} m52;

void mapInit_52(void);
void extclCpuWrMem_52(WORD address, BYTE value);
BYTE extclSaveMapper_52(BYTE mode, BYTE slot, FILE *fp);

#endif /* MAPPER52_H_ */
