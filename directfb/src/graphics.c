/* (c) Copyright 2012 Felipe Magno de Almeida
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#include <middleware-api/graphics.h>
#include <directfb.h>

#include <assert.h>

struct middleware_api_graphics_surface;
typedef struct middleware_api_graphics_surface middleware_api_graphics_surface;
typedef enum middleware_api_graphics_image_format middleware_api_graphics_image_format;

IDirectFB* directfb;
static IDirectFBSurface* primary_surface;

void middleware_api_graphics_initialize(int argc, char** argv)
{
  assert(directfb == 0);
  assert(primary_surface == 0);

  DirectFBInit(&argc, &argv);
  DirectFBCreate(&directfb);

  IDirectFBDisplayLayer* display_layer = 0;
  directfb->GetDisplayLayer(directfb, DLID_PRIMARY, &display_layer);

  DFBDisplayLayerConfig config;
  config.flags = 
    (DLCONF_WIDTH | DLCONF_HEIGHT/* | DLCONF_PIXELFORMAT*/ | DLCONF_SURFACE_CAPS/**/ | DLCONF_BUFFERMODE);
  config.width = 1280;
  config.height = 720;
  //config.pixelformat = DSPF_ARGB;
  config.surface_caps = (DSCAPS_DOUBLE | DSCAPS_FLIPPING | DSCAPS_PRIMARY);
  config.buffermode = DLBM_BACKVIDEO;
  display_layer->SetConfiguration(display_layer, &config);
  
  display_layer->GetSurface(display_layer, &primary_surface);
}

middleware_api_graphics_surface_t middleware_api_graphics_create_surface
 (unsigned int width, unsigned int height)
{
  DFBSurfaceDescription description;
  memset(&description, 0, sizeof(description));
  description.flags  = DSDESC_WIDTH | DSDESC_HEIGHT;
  description.width = width;
  description.height = height;
  IDirectFBSurface* surface = 0;
  directfb->CreateSurface(directfb, &description, &surface);
  return (middleware_api_graphics_surface_t)surface;
}

void middleware_api_graphics_draw_on_primary_surface
 (middleware_api_graphics_surface_t primary)
{
  IDirectFBSurface* destination = (IDirectFBSurface*)primary_surface;
  IDirectFBSurface* source = (IDirectFBSurface*)primary;
  /* destination->SetBlittingFlags(destination, DSBLIT_BLEND_ALPHACHANNEL); */
  /* DFBRectangle rectangle = {0, 0, 0, 0}; */
  /* source->GetSize(source, &rectangle.w, &rectangle.h); */
  destination->StretchBlit(destination, source, 0, 0);
  destination->Flip(destination, 0, DSFLIP_BLIT);
}

void middleware_api_graphics_release_surface(middleware_api_graphics_surface_t p)
{
  IDirectFBSurface* surface = (IDirectFBSurface*)p;
  surface->Release(surface);
}

void middleware_api_graphics_stretch_bitblit
(middleware_api_graphics_surface_t sdestination, middleware_api_graphics_surface_t ssource
 , unsigned int dx, unsigned int dy, unsigned int dw, unsigned int dh
 , unsigned int sx, unsigned int sy, unsigned int sw, unsigned int sh)
{
  IDirectFBSurface* destination = (IDirectFBSurface*)sdestination;
  IDirectFBSurface* source = (IDirectFBSurface*)ssource;
  destination->SetBlittingFlags(destination, DSBLIT_BLEND_ALPHACHANNEL);
  DFBRectangle rectangle = {sx, sy, sw, sh};
  DFBRectangle drectangle = {dx, dy, dw, dh};
  destination->StretchBlit(destination, source, &rectangle, &drectangle);
}

void middleware_api_graphics_clear_surface
 (middleware_api_graphics_surface_t p, uint32_t color)
{
}

middleware_api_graphics_surface_t middleware_api_graphics_load_image
 (const char* buffer, size_t size
  , middleware_api_graphics_image_format format)
{
  DFBDataBufferDescription ddsc;
  ddsc.flags = DBDESC_MEMORY;
  ddsc.file = 0;
  ddsc.memory.data = (const void*)buffer;
  ddsc.memory.length = size;
  IDirectFBDataBuffer* dbuffer = 0;
  directfb->CreateDataBuffer(directfb, &ddsc, &dbuffer);
  IDirectFBImageProvider* provider = 0;
  dbuffer->CreateImageProvider(dbuffer, &provider);
  DFBSurfaceDescription desc;
  provider->GetSurfaceDescription(provider, &desc);
  IDirectFBSurface* surface = 0;
  directfb->CreateSurface(directfb, &desc, &surface);
  provider->RenderTo(provider, surface, 0);
  return (middleware_api_graphics_surface_t)surface;
}

size_t middleware_api_graphics_width(middleware_api_graphics_surface_t p)
{
  IDirectFBSurface* surface = (IDirectFBSurface*)p;
  int w = 0, h = 0;
  surface->GetSize(surface, &w, &h);
  return w;
}

size_t middleware_api_graphics_height(middleware_api_graphics_surface_t p)
{
  IDirectFBSurface* surface = (IDirectFBSurface*)p;
  int w = 0, h = 0;
  surface->GetSize(surface, &w, &h);
  return h;
}


