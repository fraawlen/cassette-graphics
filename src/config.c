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

#include <assert.h>
#include <float.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include <cairo/cairo.h>

#include <cassette/ccfg.h>
#include <cassette/cgui.h>
#include <cassette/cobj.h>

#include "config.h"
#include "config-default.h"
#include "util.h"

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

#define _SCALE(X) X *= _config.scale;

#define _SCALE_WINDOW(W) \
	W.thickness_border  *= _config.scale; \
	W.padding_outer     *= _config.scale; \
	W.padding_inner     *= _config.scale; \
	W.padding_cell      *= _config.scale; \

#define _SCALE_CELL(C) \
	C.thickness_border  *= _config.scale; \
	C.thickness_outline *= _config.scale; \
	C.margin            *= _config.scale;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#define _STYLE_WINDOW(NAMESPACE, TARGET) \
	{ NAMESPACE, "border_thickness",          _LENGTH, &TARGET.thickness_border          }, \
	{ NAMESPACE, "outer_padding",             _LENGTH, &TARGET.padding_outer             }, \
	{ NAMESPACE, "inner_padding",             _LENGTH, &TARGET.padding_inner             }, \
	{ NAMESPACE, "cell_padding",              _LENGTH, &TARGET.padding_cell              }, \
	{ NAMESPACE, "color_background",          _COLOR,  &TARGET.color_background          }, \
	{ NAMESPACE, "color_background_disabled", _COLOR,  &TARGET.color_background_disabled }, \
	{ NAMESPACE, "color_background_focused",  _COLOR,  &TARGET.color_background_focused  }, \
	{ NAMESPACE, "color_background_locked",   _COLOR,  &TARGET.color_background_locked   }, \
	{ NAMESPACE, "color_border",              _COLOR,  &TARGET.color_border              }, \
	{ NAMESPACE, "color_border_disabled",     _COLOR,  &TARGET.color_border_disabled     }, \
	{ NAMESPACE, "color_border_focused",      _COLOR,  &TARGET.color_border_focused      }, \
	{ NAMESPACE, "color_border_locked",       _COLOR,  &TARGET.color_border_locked       }, \
	{ NAMESPACE, "enable_disabled_substyle",  _BOOL,   &TARGET.enable_disabled           }, \
	{ NAMESPACE, "enable_focused_substyle",   _BOOL,   &TARGET.enable_focused            }, \
	{ NAMESPACE, "enable_locked_substyle",    _BOOL,   &TARGET.enable_locked             },

#define _STYLE_CELL(NAMESPACE, TARGET) \
	{ NAMESPACE, "border_thickness",          _LENGTH, &TARGET.thickness_border          }, \
	{ NAMESPACE, "outline_thickness",         _LENGTH, &TARGET.thickness_outline         }, \
	{ NAMESPACE, "margin",                    _LENGTH, &TARGET.margin                    }, \
	{ NAMESPACE, "color_background",          _COLOR,  &TARGET.color_background          }, \
	{ NAMESPACE, "color_border",              _COLOR,  &TARGET.color_border              }, \
	{ NAMESPACE, "color_outline",             _COLOR,  &TARGET.color_outline             },

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

enum _value_t
{
	_STRING,
	_COLOR,
	_BOOL,
	_LENGTH,
	_POSITION,
	_DOUBLE,
	_UDOUBLE,
	_RATIO,
	_ANTIALIAS,
	_SUBPIXEL,
};

typedef enum _value_t _value_t;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

struct _resource_t
{
	char *namespace;
	char *name;
	_value_t type;
	void *target;
};

typedef struct _resource_t _resource_t;

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

static void _fetch (const _resource_t *resource);
static bool _fill  (void);

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

static cgui_config_t _config     = config_default;
static ccfg_t *_parser           = NULL;
static cobj_dictionary_t *_words = NULL;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static const _resource_t _resources[] =
{
	{ "global", "scale",               _UDOUBLE,   &_config.scale                    },

	{ "font",   "face",                _STRING,     _config.font_face                },
	{ "font",   "size",                _LENGTH,    &_config.font_size                },
	{ "font",   "horizontal_spacing",  _LENGTH,    &_config.font_spacing_horizontal  },
	{ "font",   "vertical_spacing",    _LENGTH,    &_config.font_spacing_vertical    },
	{ "font",   "width_override",      _LENGTH,    &_config.font_override_width      },
	{ "font",   "ascent_override",     _LENGTH,    &_config.font_override_ascent     },
	{ "font",   "descent_override",    _LENGTH,    &_config.font_override_descent    },
	{ "font",   "x_offset",            _POSITION,  &_config.font_offset_x            },
	{ "font",   "y_offset",            _POSITION,  &_config.font_offset_y            },
	{ "font",   "enable_overrides",    _BOOL,      &_config.font_enable_overrides    },
	{ "font",   "enable_hint_metrics", _BOOL,      &_config.font_enable_hint_metrics },
	{ "font",   "antialias_mode",      _ANTIALIAS, &_config.font_antialias           },
	{ "font",   "subpixel_mode",       _SUBPIXEL,  &_config.font_subpixel            },

	_STYLE_WINDOW("window", _config.window_style)
	_STYLE_WINDOW("popup",  _config.popup_style)
};

/************************************************************************************************************/
/* PUBLIC ***************************************************************************************************/
/************************************************************************************************************/

const cgui_config_t *
cgui_config_get(void)
{
	return &_config;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

ccfg_t *
cgui_config_get_object(void)
{
	return _parser ? _parser : ccfg_get_placeholder();
}

/************************************************************************************************************/
/* PRIVATE **************************************************************************************************/
/************************************************************************************************************/

bool
config_init(void)
{
	cobj_string_t *home;

	bool fail = false;

	/* parser */

	_parser = ccfg_create();
	home  = cobj_string_create();

	cobj_string_set_raw(home, util_env_exists("HOME") ? getenv("HOME") : getpwuid(getuid())->pw_dir);
	cobj_string_append_raw(home, "/.config/cgui.conf");

	ccfg_push_source(_parser, getenv("CGUI_CONFIG_SOURCE"));
	ccfg_push_source(_parser, cobj_string_get_chars(home));
	ccfg_push_source(_parser, "/usr/share/cgui/cgui.conf");
	ccfg_push_source(_parser, "/etc/cgui.conf");

	cobj_string_destroy(&home);

	/* dictionary */

	_words = cobj_dictionary_create(5, 0.6);

	cobj_dictionary_write(_words, "none",     _ANTIALIAS, CGUI_CONFIG_ANTIALIAS_NONE);
	cobj_dictionary_write(_words, "gray",     _ANTIALIAS, CGUI_CONFIG_ANTIALIAS_GRAY);
	cobj_dictionary_write(_words, "subpixel", _ANTIALIAS, CGUI_CONFIG_ANTIALIAS_SUBPIXEL);
	cobj_dictionary_write(_words, "rgb",      _SUBPIXEL,  CGUI_CONFIG_SUBPIXEL_RGB);
	cobj_dictionary_write(_words, "bgr",      _SUBPIXEL,  CGUI_CONFIG_SUBPIXEL_BGR);
	cobj_dictionary_write(_words, "vrgb",     _SUBPIXEL,  CGUI_CONFIG_SUBPIXEL_VRGB);
	cobj_dictionary_write(_words, "vbgr",     _SUBPIXEL,  CGUI_CONFIG_SUBPIXEL_VBGR);
	
	/* end */

	fail |= ccfg_has_failed(_parser);
	fail |= cobj_dictionary_has_failed(_words);
	fail |= !config_load();

	return !fail;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

bool
config_load(void)
{
	_config      = config_default;
	_config.init = true;

	if (util_env_exists("CGUI_CONFIG_HARDCODED_ONLY"))
	{
		return true;
	}

	ccfg_load(_parser);
	for (size_t i = 0; i < sizeof(_resources) / sizeof(_resource_t); i++)
	{
		_fetch(_resources + i);
	}

	return _fill() && !ccfg_has_failed(_parser);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
config_reset(void)
{
	ccfg_destroy(&_parser);
	cobj_dictionary_destroy(&_words);

	_config = config_default;
}

/************************************************************************************************************/
/* _ ********************************************************************************************************/
/************************************************************************************************************/

static void
_fetch(const _resource_t *resource)
{
	const char *str;
	size_t ref;

	ccfg_fetch_resource(_parser, resource->namespace, resource->name);
	if (ccfg_pick_next_resource_value(_parser))
	{
		str = ccfg_get_resource_value(_parser);
	}
	else
	{
		return;
	}

	switch (resource->type)
	{
		case _STRING:
			snprintf((char*)resource->target, CGUI_CONFIG_MAX_STRING, "%s", str);
			break;

		case _COLOR:
			*(cobj_color_t*)resource->target = cobj_color_convert_str(str, NULL);
			break;

		case _BOOL:
			*(bool*)resource->target = strtod(str, NULL) != 0.0;
			break;

		case _LENGTH:
			*(uint16_t*)resource->target = util_str_to_long(str, 0, UINT16_MAX);
			break;

		case _POSITION:
			*(int16_t*)resource->target = util_str_to_long(str, INT16_MIN, INT16_MAX);
			break;

		case _DOUBLE:
			*(double*)resource->target = util_str_to_double(str, DBL_MIN, DBL_MAX);
			break;

		case _UDOUBLE:
			*(double*)resource->target = util_str_to_double(str, 0.0, DBL_MAX);
			break;

		case _RATIO:
			*(double*)resource->target = util_str_to_double(str, 0.0, 1.0);
			break;

		case _ANTIALIAS:
		case _SUBPIXEL:
			cobj_dictionary_find(_words, str, resource->type, &ref);
			*(int*)resource->target = ref;
			break;

		default:
			return;
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static bool
_fill(void)
{
	cairo_font_extents_t f_e;
	cairo_text_extents_t t_e;
	cairo_surface_t *c_srf;
	cairo_t *c_ctx;

	bool failed = false;

	/* geometry and font scaling */

	_SCALE(_config.font_size);
	_SCALE(_config.font_spacing_horizontal);
	_SCALE(_config.font_spacing_vertical);
	_SCALE(_config.font_offset_x);
	_SCALE(_config.font_offset_y);
	_SCALE(_config.font_override_ascent);
	_SCALE(_config.font_override_descent);
	_SCALE(_config.font_override_width);

	_SCALE_WINDOW(_config.window_style);
	_SCALE_WINDOW(_config.popup_style);

	/* get font geometry with cairo */

	if (_config.font_enable_overrides)
	{
		_config.font_descent = _config.font_override_descent;
		_config.font_ascent  = _config.font_override_ascent;
		_config.font_width   = _config.font_override_width;
		goto skip_auto_font;
	}

	c_srf = cairo_image_surface_create(CAIRO_FORMAT_A1, 0, 0);
	c_ctx = cairo_create(c_srf);
	if (cairo_surface_status(c_srf) != CAIRO_STATUS_SUCCESS || cairo_status(c_ctx) != CAIRO_STATUS_SUCCESS)
	{
		failed = true;
		goto skip_font_setup;
	}
	
	cairo_set_font_size(c_ctx, _config.font_size);
	cairo_select_font_face(
		c_ctx,
		_config.font_face,
		CAIRO_FONT_SLANT_NORMAL,
		CAIRO_FONT_WEIGHT_NORMAL);

	cairo_font_extents(c_ctx, &f_e);
	cairo_text_extents(c_ctx, "A", &t_e);

	_config.font_descent = f_e.descent;
	_config.font_ascent  = f_e.ascent;
	_config.font_width   = t_e.width;

skip_font_setup:

	cairo_destroy(c_ctx);
	cairo_surface_destroy(c_srf);

skip_auto_font:

	_config.font_height = _config.font_ascent + _config.font_descent;

	/* end */

	printf(">> %u\n", _config.window_style.padding_cell);
	printf(">> %i\n", _config.font_antialias);
	printf(">> %s\n", _config.font_face);

	return !failed;
}
