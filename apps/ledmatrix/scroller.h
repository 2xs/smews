/* This file is part of rflpc.                        
 *									 
 * rflpc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by	 
 * the Free Software Foundation, either version 3 of the License, or	 
 * (at your option) any later version.					 
 * 									 
 * rflpc is distributed in the hope that it will be useful,		 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of	 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	 
 * GNU General Public License for more details.				 
 * 									 
 * You should have received a copy of the GNU General Public License	 
 * along with rflpc.  If not, see <http://www.gnu.org/licenses/>.	 
 */
/*
  Author: Michael Hauspie <Michael.Hauspie@univ-lille1.fr>
  Created: 
  Time-stamp: <2011-10-09 04:37:34 (mickey)>
*/
#ifndef __SCROLLER_H__
#define __SCROLLER_H__

void display_char(uint8_t *buffer, unsigned char c, int xpos, uint8_t color);
void display_text(uint8_t *buffer, const char *text, int position, uint8_t color);

#endif

