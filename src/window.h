/**
 * Copyright © 2024 Fraawlen <fraawlen@posteo.net>
 *
 * This file is part of the Cassette Graphics (CGUI) library.
 *
 * This library is free software; you can redistribute it and/or modify it either under the terms of the GNU
 * Lesser General Public License as published by the Free Software Foundation; either version 2.1 of the
 * License or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.
 * See the LGPL for the specific language governing rights and limitations.
 *
 * You should have received a copy of the GNU Lesser General Public License along with this program. If not,
 * see <http://www.gnu.org/licenses/>.
 */

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

#pragma once

#include <cassette/cgui.h>
#include <stdbool.h>

/************************************************************************************************************/
/* TYPES ****************************************************************************************************/
/************************************************************************************************************/

struct cgui_window
{
	/* geometry */

	int16_t x;
	int16_t y;
	uint16_t width;
	uint16_t height;

	/* xcb stuff */

	xcb_window_t x_id;

	/* callbacks */

	void (*fn_close) (cgui_window *);
	void (*fn_state) (cgui_window *, enum cgui_window_state_mask);

	/* states */

	struct cgui_window_state_flags state;
	bool valid;
};

/************************************************************************************************************/
/* CONSTRUCTORS / DESTRUCTORS *******************************************************************************/
/************************************************************************************************************/

void
window_destroy(cgui_window *window)
CGUI_NONNULL(1);

/************************************************************************************************************/
/* IMPURE METHODS *******************************************************************************************/
/************************************************************************************************************/

void
window_present(cgui_window *window)
CGUI_NONNULL(1);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
window_update_state(cgui_window *window, enum cgui_window_state_mask mask, bool value)
CGUI_NONNULL(1);
