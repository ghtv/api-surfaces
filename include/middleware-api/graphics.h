/* (c) Copyright 2012 Felipe Magno de Almeida
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef MIDDLEWARE_API_PROPOSAL_GRAPHICS_H
#define MIDDLEWARE_API_PROPOSAL_GRAPHICS_H

#define MIDDLEWARE_API_PROPOSAL_GRAPHICS_SURFACES

#include <stddef.h>
#include <stdint.h>

#include <middleware-api/declaration.h>

#ifdef __cplusplus
extern "C" {
#endif

struct middleware_api_graphics_surface;
typedef struct middleware_api_graphics_surface* middleware_api_graphics_surface_t;

enum middleware_api_graphics_image_format
{
  middleware_api_graphics_image_format_jpeg
  , middleware_api_graphics_image_format_png
  , middleware_api_graphics_image_format_gif
};

MIDDLEWARE_API_MANUFACTURER_DECL void middleware_api_graphics_draw_on_primary_surface
 (middleware_api_graphics_surface_t primary);

MIDDLEWARE_API_MANUFACTURER_DECL middleware_api_graphics_surface_t
 middleware_api_graphics_create_surface
 (unsigned int width, unsigned int height);

MIDDLEWARE_API_MANUFACTURER_DECL void middleware_api_graphics_release_surface
 (middleware_api_graphics_surface_t p);

MIDDLEWARE_API_MANUFACTURER_DECL void middleware_api_graphics_stretch_bitblit
(middleware_api_graphics_surface_t destination, middleware_api_graphics_surface_t source
 , unsigned int dx, unsigned int dy, unsigned int dw, unsigned int dh
 , unsigned int sx, unsigned int sy, unsigned int sw, unsigned int sh);

MIDDLEWARE_API_MANUFACTURER_DECL void middleware_api_graphics_clear_surface
 (middleware_api_graphics_surface_t p, uint32_t color);

MIDDLEWARE_API_MANUFACTURER_DECL middleware_api_graphics_surface_t
 middleware_api_graphics_load_image
 (const char* buffer, size_t size
  , enum middleware_api_graphics_image_format format);

MIDDLEWARE_API_MANUFACTURER_DECL size_t middleware_api_graphics_width
 (middleware_api_graphics_surface_t p);

MIDDLEWARE_API_MANUFACTURER_DECL size_t middleware_api_graphics_height
 (middleware_api_graphics_surface_t p);

#ifdef __cplusplus
}
#endif

#endif
