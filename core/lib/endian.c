/*
   This file is part of QuasselC.

   QuasselC is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   QuasselC is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with QuasselC.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include "quasselc.h"

unsigned long long int lltob(unsigned long long int a) {
	unsigned long long int ret = 0;
	int i;
	for(i=0;i<8;++i) {
		ret <<= 8;
		ret |= a&0xff;
		a>>=8;
	}
	return ret;
}
