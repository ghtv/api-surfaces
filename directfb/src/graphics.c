/* (c) Copyright 2012 Felipe Magno de Almeida
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#include <middleware-api/graphics.h>
#include <gst/gst.h>

#include <directfb.h>

#include <assert.h>
#include <cairo.h>

struct middleware_api_graphics_surface;
typedef struct middleware_api_graphics_surface middleware_api_graphics_surface;
typedef enum middleware_api_graphics_image_format middleware_api_graphics_image_format;

IDirectFB* directfb;
static IDirectFBSurface* primary_surface;
static middleware_api_graphics_primary_surface_draw_callback_t callback;
static void* callback_ud;

gboolean video_sink_init(GstPlugin* plugin);
gboolean video_source_init(GstPlugin* plugin);
void init_plugin(GstElement* appsrc);
void middleware_api_graphics_draw_frame();

static int fps = 0;
static struct timespec start_tm;
struct timespec m_start_tm;

void post_render(void* v)
{
  middleware_api_graphics_draw_frame();
  ++fps;

  struct timespec tm;
  clock_gettime(CLOCK_REALTIME, &tm);
  if(tm.tv_sec - start_tm.tv_sec)
  {
    printf("FPS: %d\n", fps);
    fps = 0;
    start_tm = tm;
  }
}

void middleware_api_graphics_initialize(int argc, char** argv)
{
  assert(directfb == 0);
  assert(primary_surface == 0);

  DirectFBInit(&argc, &argv);
  gst_init(&argc, &argv);

  gst_plugin_register_static(GST_VERSION_MAJOR
                             , GST_VERSION_MINOR
                             , "ghtv_video_sink"
                             , "Some description"
                             , &video_sink_init
                             , "1.0"
                             , "Proprietary"
                             , "XXX"
                             , "XXX", "XXX");

  DirectFBCreate(&directfb);

  IDirectFBDisplayLayer* display_layer = 0;
  directfb->GetDisplayLayer(directfb, DLID_PRIMARY, &display_layer);

  DFBDisplayLayerConfig config;
  config.flags = 
    (DLCONF_WIDTH | DLCONF_HEIGHT | DLCONF_PIXELFORMAT | DLCONF_SURFACE_CAPS/**/ | DLCONF_BUFFERMODE);
  /* config.width = 1280; */
  /* config.height = 720; */
  /* config.pixelformat = DSPF_ARGB; */
  config.surface_caps = (DSCAPS_DOUBLE | DSCAPS_FLIPPING | DSCAPS_PRIMARY);
  config.buffermode = DLBM_BACKVIDEO;
  display_layer->SetConfiguration(display_layer, &config);
  
  display_layer->GetSurface(display_layer, &primary_surface);

  int width, height;
  primary_surface->GetSize(primary_surface, &width, &height);

  GstElement* pipeline = gst_pipeline_new(0);
  GstElement* source = gst_element_factory_make("appsrc", 0);
  assert(source != 0);
  init_plugin(source);
  GstElement* h264parse = gst_element_factory_make("h264parse", 0);
  assert(h264parse != 0);
#if GST_VERSION_MAJOR == 0
  GstElement* h264dec = gst_element_factory_make("ffdec_h264", 0);
  assert(h264dec != 0);
  GstElement* conv = gst_element_factory_make ("ffmpegcolorspace", "conv");
  assert(conv != 0);
#else
  GstElement* h264dec = gst_element_factory_make("avdec_h264", 0);
  assert(h264dec != 0);
  GstElement* conv = gst_element_factory_make ("videoconvert", "conv");
  assert(conv != 0);
#endif
  /* GstElement* color_filter = gst_element_factory_make("capsfilter", "color_filter"); */
  /* assert(color_filter != 0); */
  GstElement* scale = 0, *scale_filter = 0;
  if(width != 1920 && height != 1080)
  {
    scale = gst_element_factory_make ("videoscale", "scale");
    assert(conv != 0);
    scale_filter = gst_element_factory_make("capsfilter", "scale_filter");
    assert(scale_filter != 0);
    g_object_set(G_OBJECT(scale_filter), "caps"
                 , gst_caps_new_simple("video/x-raw-rgb"
                                       /* , "bpp", G_TYPE_INT, 32 */
                                       /* , "depth", G_TYPE_INT, 32 */
                                       , "width", G_TYPE_INT, width
                                       , "height", G_TYPE_INT, height
                                       , NULL)
                 , NULL);

  }

  /* g_object_set(G_OBJECT(color_filter), "caps" */
  /*              , gst_caps_new_simple("*" */
  /*                                    , "bpp", G_TYPE_INT, 32 */
  /*                                    , "depth", G_TYPE_INT, 32 */
  /*                                    /\* , "width", G_TYPE_INT, width *\/ */
  /*                                    /\* , "height", G_TYPE_INT, height *\/ */
  /*                                    , NULL) */
  /*              , NULL); */

  GstElement* sink = gst_element_factory_make("ghtv_video_sink", 0);
  assert(sink != 0);

  printf("Primary surface: %p\n", primary_surface);
  g_object_set(sink, "surface", (void*)primary_surface, NULL);
  g_object_set(sink, "post-render-callback", (void*)&post_render, NULL);
  g_object_set(sink, "callback-data", (void*)0, NULL);

  gst_bin_add_many (GST_BIN(pipeline), source, h264parse, h264dec, conv, sink, (void*)0);
  if(scale)
  {
    gst_bin_add_many (GST_BIN(pipeline), scale, scale_filter, (void*)0);
    gst_element_link_many(source, h264parse, h264dec, conv, scale, scale_filter, sink, (void*)0);
  }
  else
    gst_element_link_many(source, h264parse, h264dec, conv, sink, (void*)0);
  clock_gettime(CLOCK_REALTIME, &start_tm);
  clock_gettime(CLOCK_REALTIME, &m_start_tm);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);
}

middleware_api_graphics_surface_t middleware_api_graphics_create_surface
 (unsigned int width, unsigned int height)
{
  DFBSurfaceDescription description;
  memset(&description, 0, sizeof(description));
  description.flags  = DSDESC_WIDTH | DSDESC_HEIGHT | DLCONF_PIXELFORMAT;
  description.width = width;
  description.height = height;
  description.pixelformat = DSPF_ARGB;
  IDirectFBSurface* surface = 0;
  directfb->CreateSurface(directfb, &description, &surface);
  return (middleware_api_graphics_surface_t)surface;
}

void middleware_api_graphics_on_primary_surface_draw
 (middleware_api_graphics_primary_surface_draw_callback_t c, void* ud)
{
  callback = c;
  callback_ud = ud;
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

size_t middleware_api_graphics_primary_surface_width()
{
  return middleware_api_graphics_width((middleware_api_graphics_surface_t)primary_surface);
}

size_t middleware_api_graphics_primary_surface_height()
{
  return middleware_api_graphics_height((middleware_api_graphics_surface_t)primary_surface);
}

void middleware_api_graphics_draw_frame()
{
  if(callback)
  {
    callback((middleware_api_graphics_surface_t)primary_surface, callback_ud);
  }
  primary_surface->Flip(primary_surface, 0, DSFLIP_ONSYNC);
}
