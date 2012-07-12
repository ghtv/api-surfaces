// (c) Copyright 2012 Felipe Magno de Almeida
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <middleware-api/graphics.h>
#include <directfb.h>

#include <assert.h>

struct middleware_api_graphics_surface;
typedef struct middleware_api_graphics_surface middleware_api_graphics_surface;
typedef enum middleware_api_graphics_image_format middleware_api_graphics_image_format;

static IDirectFB* dfb;
static IDirectFBSurface* primary_surface;

void middleware_api_graphics_initialize(int argc, char** argv)
{
  assert(dfb == 0);
  assert(primary_surface == 0);

  DirectFBInit(&argc, &argv);
  DirectFBCreate(&dfb);
  
  IDirectFBDisplayLayer* display_layer = 0;
  dfb->GetDisplayLayer(dfb, DLID_PRIMARY, &display_layer);
  
  display_layer->GetSurface(display_layer, &primary_surface);
}

middleware_api_graphics_surface* middleware_api_graphics_create_surface
 (unsigned int width, unsigned int height)
{
  DFBSurfaceDescription description;
  memset(&description, 0, sizeof(description));
  description.flags  = DSDESC_WIDTH | DSDESC_HEIGHT;
  description.width = width;
  description.height = height;
  IDirectFBSurface* surface = 0;
  dfb->CreateSurface(dfb, &description, &surface);
  return (middleware_api_graphics_surface*)surface;
}

void middleware_api_graphics_release_surface(middleware_api_graphics_surface* p)
{
  IDirectFBSurface* surface = (IDirectFBSurface*)p;
  surface->Release(surface);
}

void middleware_api_graphics_stretch_bitblit
(middleware_api_graphics_surface* destination, middleware_api_graphics_surface* source
 , unsigned int dx, unsigned int dy, unsigned int dw, unsigned int dh
 , unsigned int sx, unsigned int sy, unsigned int sw, unsigned int sh)
{
  
}

void middleware_api_graphics_clear_surface
 (middleware_api_graphics_surface* p, uint32_t color)
{
}

middleware_api_graphics_surface* middleware_api_graphics_load_image
 (const char* buffer, size_t size
  , middleware_api_graphics_image_format format)
{
  const char* filename = 0;
  switch(format)
  {
  case middleware_api_graphics_image_format_jpeg:
    filename = "a.jpg";
    break;
  case middleware_api_graphics_image_format_png:
    filename = "a.png";
    break;
  case middleware_api_graphics_image_format_gif:
    filename = "a.gif";
    break;
  default:
    abort();
  }
  IDirectFBImageProvider provider = 0;
  dfb->CreateImageProvider(dfb, filename, &provider);
}

size_t middleware_api_graphics_width(middleware_api_graphics_surface* p)
{
  IDirectFBSurface* surface = (IDirectFBSurface*)p;
  int w = 0, h = 0;
  surface->GetSize(surface, &w, &h);
  return w;
}

size_t middleware_api_graphics_height(middleware_api_graphics_surface* p)
{
  IDirectFBSurface* surface = (IDirectFBSurface*)p;
  int w = 0, h = 0;
  surface->GetSize(surface, &w, &h);
  return h;
}


