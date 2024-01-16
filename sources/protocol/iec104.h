/*
 * Copyright (C) 2005, Grigoriy Sitkarev
 * sitkarev@komitex.ru
 * Copyright (C) 2007, Vladimir Lettiev
 * lettiev-vv@komi.tgc-9.ru
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#pragma once
#include "iec104_types.h"

#define IEC_OBJECT_MAX 127
#define IEC_TYPEID_LEN 3

/* Information object */
struct iec_object
{
	uint16_t ioa; /* information object address */
	uint8_t ioa2; /* information object address */
	union
	{
		struct iec_type1 type1;
		struct iec_type7 type7;
		struct iec_type9 type9;
		struct iec_type11 type11;
		struct iec_type13 type13;
		struct iec_type30 type30;
		struct iec_type33 type33;
		struct iec_type34 type34;
		struct iec_type35 type35;
		struct iec_type36 type36;
		struct iec_type37 type37;
		struct iec_type100 type100;
		struct iec_type101 type101;
		struct iec_type103 type103;
	} o;
};