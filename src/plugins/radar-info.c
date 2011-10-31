/*
 * Copyright (C) 2009-2011 Andy Spencer <andy753421@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <rsl.h>
#include "radar-info.h"

AWeatherColormap colormaps[] = {
	// type    file      ...
	{DZ_INDEX, "dz.clr"},
	{VR_INDEX, "vr.clr"},
	{SW_INDEX, "sw.clr"},
	{DR_INDEX, "dr.clr"},
	{PH_INDEX, "ph.clr"},
	{RH_INDEX, "rh.clr"},
	{0,        NULL    },
};
