/* (c) Copyright 2012 Felipe Magno de Almeida
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#include <middleware-api/graphics.h>
#include <middleware-api/sections.h>
#include <gst/gst.h>
#include <gst/gstversion.h>
#include <gst/base/gstpushsrc.h>
#include <gst/video/gstvideosink.h>
//#include <gst/interfaces/navigation.h>
//#include <gst/interfaces/colorbalance.h>

#include <directfb.h>

#include <assert.h>
#include <cairo.h>

#include <stdlib.h>

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
    GST_STATIC_CAPS (
#if GST_VERSION_MAJOR == 0
                     "video/x-raw-rgb, "
#else
                     "video/x-raw, format=(string) RGB, "
#endif
                     "red_mask=(int)65280, green_mask=(int)16711680, blue_mask=(int)-16777216, "
                     "framerate = (fraction) [ 0, MAX ], "
                     "bpp = 32, "
                     "depth = 24, "
                     "width = (int) [ 1, MAX ], "
                     "height = (int) [ 1, MAX ]; "
                     )
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
  /* printf("video_sink_render\n"); */
  if(self->surface)
  {
    /* GstStructure* structure = gst_caps_get_structure (GST_BUFFER_CAPS (buf), 0); */
    gint w = self->video_width, h = self->video_height;
    /* if (structure) */
    /* { */
    /*   gst_structure_get_int (structure, "width", &w); */
    /*   gst_structure_get_int (structure, "height", &h); */
    /* } */
    assert(w != 0 && h != 0);

    /* printf("width %d and height %d\n", (int)w, (int)h); */

    /* printf("surface: %p\n", self->surface); */
    if(self->pre_render_callback)
      self->pre_render_callback(self->callback_data);

    assert(self->surface != 0);
    void* data = 0;
    int dest_pitch = 0;
    self->surface->Lock(self->surface, DSLF_WRITE, &data, &dest_pitch);
    
#if GST_VERSION_MAJOR == 0
    int src_pitch = GST_BUFFER_SIZE(buf)/h;
#else
    int src_pitch = gst_buffer_get_sizes(buf, NULL, NULL)/h;
#endif

    int pitch = MIN(dest_pitch, src_pitch);
#if GST_VERSION_MAJOR == 0
    guint8* current = GST_BUFFER_DATA (buf);

    int i = 0;
    for (; i != h; i++)
    {
      memcpy (data, current, pitch);
      current += src_pitch;
      data = (char*)((data) + dest_pitch);
    }
#else
    size_t current = 0;
    int i = 0;
    for (; i != h; i++)
    {
      gst_buffer_extract(buf, current, data, pitch);
      current += src_pitch;
      data = (char*)((data) + dest_pitch);
    }
#endif
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
  printf("ghtv set_caps\n");
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

struct GstGhtvVideoSourceClass
{
  GstPushSrcClass base;
};

typedef struct GstGhtvVideoSinkClass GstGhtvVideoSinkClass;
typedef struct GstGhtvVideoSourceClass GstGhtvVideoSourceClass;

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

struct ghtv_video_source
{
  GstPushSrc base;
};
typedef struct ghtv_video_source ghtv_video_source;

static ghtv_video_source* video_source;

void gst_ghtv_video_source_init (ghtv_video_source * v)
{
  video_source = v;
}

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

void gst_ghtv_video_source_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_set_details_simple (element_class, "GHTV video source",
      "Source/Video",
      "GHTV video source", "GHTV");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&srctemplate));
}

GstElement* source_element = 0;

void init_plugin(GstElement* appsrc)
{
  g_object_set(appsrc, "is-live", (gboolean)1, NULL);
  /* g_object_set(appsrc, "do-timestamp", (gboolean)0, NULL); */
  g_object_set(appsrc, "emit-signals", (gboolean)0, NULL);
  g_object_set(appsrc, "format", GST_FORMAT_BUFFERS, NULL);
  g_object_set(appsrc, "block", (gboolean)1, NULL);
  g_object_set(appsrc, "blocksize", (gulong)64*1024, NULL);
  g_object_set(appsrc, "max-bytes", (uint64_t)1024*1024, NULL);
  source_element = appsrc;
}

static uint64_t first_timestamp = -1;
#if GST_VERSION_MAJOR == 1
static uint64_t first_dts_timestamp = -1;
#endif
extern struct timespec m_start_tm;
size_t size_in_second = 0;

void middleware_api_video_sections(const char* original_buffer, size_t original_size
                                   , middleware_api_sections_filter_t filter
                                   , void* state)
{
  {
    size_in_second += original_size;
    struct timespec tm;
    clock_gettime(CLOCK_REALTIME, &tm);
    if(tm.tv_sec - m_start_tm.tv_sec)
    {
      printf("demultiplexed in bytes: %d\n", (int)size_in_second);
      size_in_second = 0;
      m_start_tm = tm;
    }
  }

  assert(source_element != 0);
  size_t size = original_size;
  /* uint8_t alignment = 0; */
  const unsigned char* buffer = (const unsigned char*)original_buffer;
  /* while(size != 0) */
  /* { */
    assert(buffer[0] == 0);
    assert(buffer[1] == 0);
    assert(buffer[2] == 1);

    uint8_t stream_id = buffer[3];

    uint16_t pes_packet_length = 0;
    ((char*)&pes_packet_length)[1] = buffer[4];
    ((char*)&pes_packet_length)[0] = buffer[5];

    /* printf("stream id: %d pes packet length %d real length: %d\n", (int)stream_id */
    /*        , (int)pes_packet_length, (int)size); */
    assert(stream_id == 0xe0);

    /* if(!alignment || (stream_id != 9 && stream_id != 6)) */
    /* { */
    assert(((unsigned char)buffer[6] & 0xC0) == 0x80);

    /* alignment = (((unsigned char)buffer[6]) >> 2) & 1; */
    /* printf("alignment %d\n", (int)alignment); */
    uint8_t pts_dts_flags = (((unsigned char)buffer[7]) >> 6) & 3;
    uint8_t escr_flag = (((unsigned char)buffer[7]) >> 5) & 1;
    uint8_t es_rate_flag = (((unsigned char)buffer[7]) >> 4) & 1;
    uint8_t dsm_trick_mode_flag = (((unsigned char)buffer[7]) >> 3) & 1;
    uint8_t additional_copy_info_flag = (((unsigned char)buffer[7]) >> 2) & 1;
    uint8_t pes_crc_flag = (((unsigned char)buffer[7]) >> 1) & 1;
    /* uint8_t pes_extension_flag = ((unsigned char)buffer[7]) & 1; */
    /* uint8_t pes_header_data_length = (unsigned char)buffer[8]; */

    /* printf("pes_header_data_length: %d\n", (int)pes_header_data_length); */
    
    int off = 9, start = 9;
    /* int off_payload = 9 + pes_header_data_length; */
    /* int payload_size = size-off; */

    /* printf("PES buffer size %d\n", (int)payload_size); */

    int data_bytes_consumed = 0;
    uint64_t pts = -1, dts = 1;

    if(pts_dts_flags != 0)
    {
      /* printf("pts_dts_flags: %d\n", pts_dts_flags); */
      pts = 0;
      pts = ((uint64_t)(buffer[off] & 0xE)) << (uint64_t)29;
      unsigned int u = buffer[off+1];
      unsigned int d = (unsigned int)buffer[off+2] & 0xFE;
      pts |= ((u << 8) + d) << 14;
      u = buffer[off+3];
      d = (unsigned int)buffer[off+4] & 0xFE;
      pts |= ((u << 8) + d) >> 1;
      off += 5;
      data_bytes_consumed += 5;
      if(pts_dts_flags == 3)
      {
        dts = 0;
        dts = ((uint64_t)(buffer[off] & 0xE)) << (uint64_t)29;
        u = buffer[off+1];
        d = (unsigned int)buffer[off+2] & 0xFE;
        dts |= ((u << 8) + d) << 14;
        u = buffer[off+3];
        d = (unsigned int)buffer[off+4] & 0xFE;
        dts |= ((u << 8) + d) >> 1;
        off += 5;
        data_bytes_consumed += 5;
      }
    }
    else
      return;
    if(escr_flag)
    {
      /* printf("escr_flags\n"); */
      off += 6;
      data_bytes_consumed += 6;
    }
    if(es_rate_flag)
    {
      /* printf("e_rate_flags\n"); */
      off += 3;
      data_bytes_consumed += 3;
    }
    if(dsm_trick_mode_flag)
    {
      /* printf("dsm_trick_mode_flag\n"); */
      off++;
      data_bytes_consumed++;
    }
    if(additional_copy_info_flag)
    {
      /* printf("additional_copy_info_flag\n"); */
      off++;
      data_bytes_consumed++;
    }
    if(pes_crc_flag)
    {
      /* printf("pes_crc_flag\n"); */
      off += 2;
      data_bytes_consumed += 2;
    }
    /* if(pes_extension_flag) */
    /* { */
    /*   printf("pes_extension_flag\n"); */
    /* } */

    /* printf("data_bytes_consumed: %d\nsize %d\n", (int)data_bytes_consumed, pes_header_data_length - data_bytes_consumed); */
    
    /* printf("middleware_api_video_sections\n"); */

    assert (size >= start + data_bytes_consumed);

    assert(buffer == original_buffer);
    assert(size == original_size);
    size -= start + data_bytes_consumed;
    buffer += start + data_bytes_consumed;

    // feed gstreamer
    {
#if GST_VERSION_MAJOR == 0
      GstBuffer* element_buffer = gst_buffer_new ();
      GST_BUFFER_MALLOCDATA(element_buffer) = g_malloc(size);
      GST_BUFFER_DATA(element_buffer) = GST_BUFFER_MALLOCDATA(element_buffer);
      GST_BUFFER_SIZE(element_buffer) = size;
      memcpy(GST_BUFFER_DATA(element_buffer), buffer, size);
#else
      GstBuffer* element_buffer = gst_buffer_new_allocate(NULL, size, NULL);
      gst_buffer_fill(element_buffer, 0, buffer, size);
#endif

      /* { */
      /*   int file = open("pes-packet.264", O_WRONLY|O_APPEND); */
      /*   write(file, buffer, size); */
      /*   close(file); */
      /* } */

      /* printf("pts_dts_flags: %d\n", (int)pts_dts_flags); */
      assert(pts_dts_flags & 2);
      uint64_t timestamp = gst_util_uint64_scale(pts, GST_MSECOND/10, 9LL);
      if(first_timestamp == -1)
      {
        /* printf("Setting first_timestamp as: %p - 5*1000*1000*1000\n", (void*)timestamp); */
        first_timestamp = timestamp - (5ull*1000ull*1000ull*1000ull);
        /* printf("Setting first_timestamp as: %p\n", (void*)first_timestamp); */
      }
      /* if(first_timestamp == -1) */
      /*   first_timestamp = 0; */
      /* uint64_t timestamp = first_timestamp + (1000*1000*1000); */
      /* first_timestamp = timestamp; */

      /* printf("timestamp: %p pts: %p timestamp - base: %p\n", (void*)timestamp, (void*)pts, (void*)(timestamp - first_timestamp)); */
#if GST_VERSION_MAJOR == 0
      GST_BUFFER_TIMESTAMP(element_buffer) = timestamp - first_timestamp;
#else
      GST_BUFFER_PTS(element_buffer) = timestamp - first_timestamp;
      if(pts_dts_flags & 1)
      {
        /* uint64_t dts_timestamp = gst_util_uint64_scale(dts, GST_MSECOND/10, 9LL); */
        /* if(first_dts_timestamp == -1) */
        /* { */
        /*   printf("Setting first_timestamp as: %p - 5*1000*1000*1000\n", (void*)timestamp); */
        /*   first_dts_timestamp = dts_timestamp - (5ull*1000ull*1000ull*1000ull); */
        /*   printf("Setting first_timestamp as: %p\n", (void*)first_timestamp); */
        /* } */
        /* GST_BUFFER_DTS(element_buffer) = dts_timestamp - first_dts_timestamp; */
      }
#endif
      //      GST_BUFFER_DURATION(element_buffer) = /*GST_CLOCK_TIME_NONE*/1000*1000;

      GstFlowReturn ret = 0;
      /* printf("Going to emit signal\n"); */
      g_signal_emit_by_name (G_OBJECT(source_element), "push-buffer", element_buffer, &ret);
      if (ret != GST_FLOW_OK)
      {
        /* printf("Error %d\n", (int)ret); */
      }
    }
}

