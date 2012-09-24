/* (c) Copyright 2012 Felipe Magno de Almeida
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#include <middleware-api/graphics.h>
#include <gst/gst.h>
#include <gst/video/gstvideosink.h>
#include <gst/interfaces/navigation.h>
#include <gst/interfaces/colorbalance.h>

#include <directfb.h>

#include <assert.h>
#include <cairo.h>

GST_DEBUG_CATEGORY_STATIC (ghtv_video_sink_debug);

GType gst_ghtv_video_sink_get_type ();

enum
{
 ghtv_arg_surface = 1
 , ghtv_arg_pre_render_callback
 , ghtv_arg_post_render_callback
 , ghtv_arg_callback_data
};

static GstStaticPadTemplate gst_ghtv_video_sink_sink_template_factory =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw-rgb, "
        "framerate = (fraction) [ 0, MAX ], "
        "width = (int) [ 1, MAX ], "
        "height = (int) [ 1, MAX ]; "
        "video/x-raw-yuv, "
        "framerate = (fraction) [ 0, MAX ], "
        "width = (int) [ 1, MAX ], " "height = (int) [ 1, MAX ]")
    );

typedef void(*callback_type)(void*);

struct ghtv_video_sink
{
  GstVideoSink base;

  /* ghtv_video_sink() : video_width(0), video_height(0), surface(0), callback_data(0) */
  /*                   , pre_render_callback(0), post_render_callback(0) {} */

  gint video_width, video_height;
  IDirectFBSurface* surface;
  void* callback_data;
  callback_type pre_render_callback, post_render_callback;
};

typedef struct ghtv_video_sink ghtv_video_sink;

GstFlowReturn video_sink_render(ghtv_video_sink* self, GstBuffer* buf)
{
  if(self->surface)
  {
    GstStructure* structure = gst_caps_get_structure (GST_BUFFER_CAPS (buf), 0);
    gint w = 0, h = 0;
    if (structure)
    {
      gst_structure_get_int (structure, "width", &w);
      gst_structure_get_int (structure, "height", &h);
    }

    if(self->pre_render_callback)
      self->pre_render_callback(self->callback_data);

    assert(self->surface != 0);
    void* data = 0;
    int dest_pitch = 0, src_pitch = GST_BUFFER_SIZE(buf)/h;
    self->surface->Lock(self->surface, DSLF_WRITE, &data, &dest_pitch);

    int pitch = MIN(dest_pitch, src_pitch);
    guint8* current = GST_BUFFER_DATA (buf);

    int i = 0;
    for (; i != h; i++)
    {
      memcpy (data, current, pitch);
      current += src_pitch;
      data = (char*)((data) + dest_pitch);
    }

    self->surface->Unlock(self->surface);

    if(self->post_render_callback)
      self->post_render_callback(self->callback_data);
  }

  return GST_FLOW_OK;
}

void video_sink_set_property(ghtv_video_sink* self
                             , guint prop_id, GValue const* value, GParamSpec* pspec)
{
  switch(prop_id)
  {
  case ghtv_arg_surface:
    self->surface = (IDirectFBSurface*)(g_value_get_pointer(value));
    break;
  case ghtv_arg_pre_render_callback:
    self->pre_render_callback = (callback_type)(g_value_get_pointer(value));
    break;
  case ghtv_arg_post_render_callback:
    self->post_render_callback = (callback_type)(g_value_get_pointer(value));
    break;
  case ghtv_arg_callback_data:
    self->callback_data = (void*)(g_value_get_pointer(value));
    break;
  }
}

gboolean set_caps(ghtv_video_sink* self, GstCaps* caps)
{
  GstStructure* structure = gst_caps_get_structure(caps, 0);
  gst_structure_get_int(structure, "width", &self->video_width);
  gst_structure_get_int(structure, "height", &self->video_height);

  return true;
}

/* GstFlowReturn video_sink_render(ghtv_video_sink* self, GstBuffer* buf); */
/* void video_sink_set_property(ghtv_video_sink* self, guint prop_id */
/*                              , GValue const* value, GParamSpec* pspec); */
/* gboolean video_sink_set_caps(ghtv_video_sink* self, GstCaps* caps); */

static ghtv_video_sink* ghtv_video_sink_cast_from_object(GObject* obj)
{
  return G_TYPE_CHECK_INSTANCE_CAST ((obj), gst_ghtv_video_sink_get_type(), ghtv_video_sink);
}

static ghtv_video_sink* ghtv_video_sink_cast_from_base_sink(GstBaseSink* obj)
{
  return G_TYPE_CHECK_INSTANCE_CAST ((obj), gst_ghtv_video_sink_get_type(), ghtv_video_sink);
}

GstFlowReturn gst_ghtv_video_sink_render(GstBaseSink* base_sink, GstBuffer* buf)
{
  return video_sink_render(ghtv_video_sink_cast_from_base_sink(base_sink), buf);
}

void gst_ghtv_video_sink_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  video_sink_set_property(ghtv_video_sink_cast_from_object(object)
                          , prop_id, value, pspec);
}

gboolean gst_ghtv_video_sink_set_caps (GstBaseSink * base_sink, GstCaps * caps)
{
  return set_caps(ghtv_video_sink_cast_from_base_sink(base_sink), caps);
}

struct GstGhtvVideoSinkClass
{
  GstVideoSinkClass base;
};

typedef struct GstGhtvVideoSinkClass GstGhtvVideoSinkClass;

void gst_ghtv_video_sink_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_set_details_simple (element_class, "GHTV video sink",
      "Sink/Video",
      "GHTV video sink", "GHTV");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_ghtv_video_sink_sink_template_factory));
}

void gst_ghtv_video_sink_class_init (GstGhtvVideoSinkClass * klass)
{
  GObjectClass *gobject_class;
  GstBaseSinkClass *gstbasesink_class;

  gobject_class = (GObjectClass *) klass;
  gstbasesink_class = (GstBaseSinkClass *) klass;

  // gobject_class->finalize = gst_dfbvideosink_finalize;
  gobject_class->set_property = &gst_ghtv_video_sink_set_property;
  // gobject_class->get_property = gst_dfbvideosink_get_property;

  GParamFlags flags = G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS;
  g_object_class_install_property (gobject_class, ghtv_arg_surface,
      g_param_spec_pointer ("surface", "Surface",
                            "The target surface for video",
                            flags));

  g_object_class_install_property (gobject_class, ghtv_arg_pre_render_callback,
      g_param_spec_pointer ("pre-render-callback", "Pre Render Callback",
                            "Callback with void(void*) signature",
                            flags));

  g_object_class_install_property (gobject_class, ghtv_arg_post_render_callback,
      g_param_spec_pointer ("post-render-callback", "Post Render Callback",
                            "Callback with void(void*) signature",
                            flags));

  g_object_class_install_property (gobject_class, ghtv_arg_callback_data,
      g_param_spec_pointer ("callback-data", "callback_data",
                            "Data to be passed to callback",
                            flags));

  // gstelement_class->change_state = gst_dfbvideosink_change_state;

  // gstbasesink_class->get_caps = gst_dfbvideosink_getcaps;
  gstbasesink_class->set_caps = &gst_ghtv_video_sink_set_caps;
  // gstbasesink_class->buffer_alloc = gst_dfbvideosink_buffer_alloc;
  // gstbasesink_class->get_times = gst_dfbvideosink_get_times;
  gstbasesink_class->preroll = &gst_ghtv_video_sink_render;
  gstbasesink_class->render = &gst_ghtv_video_sink_render;
}

void gst_ghtv_video_sink_init (ghtv_video_sink * v)
{
  v = (ghtv_video_sink*)malloc(sizeof(ghtv_video_sink));
  v->video_width = 0;
  v->video_height = 0;
  v->surface = 0;
  v->callback_data = 0;
  v->pre_render_callback = 0;
  v->post_render_callback = 0;
}

/* // void gst_ghtv_video_sink_interface_init (GstImplementsInterfaceClass * klass); */
/* // void gst_ghtv_video_sink_navigation_init (GstNavigationInterface * iface); */
/* // void gst_ghtv_video_sink_colorbalance_init (GstColorBalanceClass * iface); */

GType gst_ghtv_video_sink_get_type ()
{
  static GType ghtv_video_sink_type = 0;

  if (!ghtv_video_sink_type) {
    static const GTypeInfo ghtv_video_sink_info = {
      sizeof (GstGhtvVideoSinkClass),
      gst_ghtv_video_sink_base_init,
      NULL,
      (GClassInitFunc) gst_ghtv_video_sink_class_init,
      NULL,
      NULL,
      sizeof (ghtv_video_sink),
      0,
      (GInstanceInitFunc) gst_ghtv_video_sink_init,
    };
    // static const GInterfaceInfo iface_info = {
    //   (GInterfaceInitFunc) gst_ghtv_video_sink_interface_init,
    //   NULL,
    //   NULL,
    // };
    // static const GInterfaceInfo navigation_info = {
    //   (GInterfaceInitFunc) gst_ghtv_video_sink_navigation_init,
    //   NULL,
    //   NULL,
    // };
    // static const GInterfaceInfo colorbalance_info = {
    //   (GInterfaceInitFunc) gst_ghtv_video_sink_colorbalance_init,
    //   NULL,
    //   NULL,
    // };

    GTypeFlags zero = 0;
    ghtv_video_sink_type = g_type_register_static
      (GST_TYPE_VIDEO_SINK,
       "ghtv_video_sink", &ghtv_video_sink_info, zero);

    assert(ghtv_video_sink_type != 0);

    // g_type_add_interface_static (ghtv_video_sink_type,
    //     GST_TYPE_IMPLEMENTS_INTERFACE, &iface_info);
    // g_type_add_interface_static (ghtv_video_sink_type, GST_TYPE_NAVIGATION,
    //     &navigation_info);
    // g_type_add_interface_static (ghtv_video_sink_type, GST_TYPE_COLOR_BALANCE,
    //     &colorbalance_info);
  }

  return ghtv_video_sink_type;
}

gboolean video_sink_init(GstPlugin* plugin)
{
  /* std::cout << "video_sink_init" << std::endl; */
  if (!gst_element_register (plugin, "ghtv_video_sink", GST_RANK_NONE
                             , gst_ghtv_video_sink_get_type()))
    return FALSE;

  GST_DEBUG_CATEGORY_INIT (ghtv_video_sink_debug, "ghtv_video_sink", 0,
      "GHTV DirectFB video sink element");

  return TRUE;
}
