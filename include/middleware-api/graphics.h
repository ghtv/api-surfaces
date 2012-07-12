// (c) Copyright 2012 Felipe Magno de Almeida
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MIDDLEWARE_API_PROPOSAL_GRAPHICS_H
#define MIDDLEWARE_API_PROPOSAL_GRAPHICS_H

#define MIDDLEWARE_API_PROPOSAL_GRAPHICS_SURFACES

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct middleware_api_graphics_surface;

enum middleware_api_graphics_image_format
{
  middleware_api_graphics_image_format_jpeg
  , middleware_api_graphics_image_format_png
  , middleware_api_graphics_image_format_gif
};

void middleware_api_graphics_draw_on_primary_surface
 (struct middleware_api_graphics_surface* primary);

struct middleware_api_graphics_surface* middleware_api_graphics_create_surface
 (unsigned int width, unsigned int height);

void middleware_api_graphics_release_surface(struct middleware_api_graphics_surface* p);

void middleware_api_graphics_stretch_bitblit
(struct middleware_api_graphics_surface* destination, struct middleware_api_graphics_surface* source
 , unsigned int dx, unsigned int dy, unsigned int dw, unsigned int dh
 , unsigned int sx, unsigned int sy, unsigned int sw, unsigned int sh);

void middleware_api_graphics_clear_surface
 (struct middleware_api_graphics_surface* p, uint32_t color);

struct middleware_api_graphics_surface* middleware_api_graphics_load_image
 (const char* buffer, size_t size
  , enum middleware_api_graphics_image_format format);

size_t middleware_api_graphics_width(struct middleware_api_graphics_surface* p);

size_t middleware_api_graphics_height(struct middleware_api_graphics_surface* p);



#ifdef __cplusplus
}
#endif

#endif
