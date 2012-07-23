/* (c) Copyright 2012 Felipe Magno de Almeida
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#include <middleware-api/sections.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <assert.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>

typedef struct middleware_api_sections_filter middleware_api_sections_filter;

struct middleware_api_sections_filter
{
  int fd;
};

static int frontend_fd;
static const int frequency = 623142857;
//623142857;//533142857;
//static const int frequency = 641142857;

typedef void(*process_demultiplex_callback)(int, void*);

void middleware_api_sections_add_fd(int fd, process_demultiplex_callback, void* state);
void middleware_api_sections_rm_fd(int fd);

void middleware_api_sections_tune()
{
  const char* frontend_device_path = "/dev/dvb/adapter0/frontend0";
  frontend_fd = open(frontend_device_path, O_RDWR);
  if(frontend_fd <= 0)
  {
    fprintf(stderr, "=== Couldn't open device %s, error %d and errno %d\n"
            , frontend_device_path, frontend_fd, errno);
    fflush(stderr);
    abort();
  }

  struct dtv_property property[2];
  memset(&property[0], 0, sizeof(property));
  property[0].cmd = DTV_FREQUENCY;
  property[0].u.data = frequency;
  property[1].cmd = DTV_TUNE;
  int r;
  do

  {
    struct dtv_properties properties = {2, &property[0]};
    r = ioctl(frontend_fd, FE_SET_PROPERTY, &properties);
  } while(r == -1 && errno == EINTR);
  if(r == -1)
  {
    fprintf(stderr, "Failed tuning to frequency %d with error %d and errno %d\n"
            , frequency, r, errno);
    fflush(stderr);
    abort();
  }

  printf("Succesfully tuned to frequency %d\n", frequency);

  fe_status_t s;
  int i = 0;
  for(; i != 10; ++i)
  {
    int r = 0;
    do
    {
      r = ioctl(frontend_fd, FE_READ_STATUS, &s);
    }
    while(r == -1 && errno == EINTR);
    if(r == -1)
    {
      fprintf(stderr, "Failed reading status with error %d and errno %d\n", r, errno);
      fflush(stderr);
      abort();
    }

    if(s & FE_HAS_LOCK)
      break;

    struct timespec ts = {1, 0};
    do
    {
      r = nanosleep(&ts, &ts);
    }
    while(r == -1 && errno == EINTR);
    if(r == -1)
    {
      fprintf(stderr, "Failed calling nanosleep with error %d and errno %d\n", r, errno);
      fflush(stderr);
      abort();
    }
  }
  if(!(s & FE_HAS_LOCK))
  {
    fprintf(stderr, "Timeout'ed locking on frontend\n");
    fflush(stderr);
    abort();
  }
  else
    printf("Locked on frontend\n");
}

struct fd_ready_callback_state
{
  struct middleware_api_sections_filter* filter;
  void* state;
  middleware_api_sections_filter_callback_t callback;
};

static void fd_ready_callback(int fd, void* state)
{
  //printf("fd_ready_callback\n");
  struct fd_ready_callback_state* callback_state
    = (struct fd_ready_callback_state*)state;
  middleware_api_sections_filter_callback_t callback = callback_state->callback;
  struct middleware_api_sections_filter* filter = callback_state->filter;
  void* other_state = callback_state->state;
  char buffer[4096];
  int r = 0;
  do
  {
    r = read(fd, buffer, sizeof(buffer));
  } while(r == -1 && errno == EINTR);
  if(r > 0)
  {
    callback(buffer, r, filter, other_state);
  }
  else
  {
    printf("Error reading error %d errno %d\n", r, errno);
    abort();
  }
}

static middleware_api_sections_filter*
create_filter_for_params(struct dmx_sct_filter_params p
                         , middleware_api_sections_filter_callback_t callback
                         , void* state)
{
  const char* demux_device_path = "/dev/dvb/adapter0/demux0";
  int fd = open(demux_device_path, O_RDWR);

  ioctl(fd, DMX_SET_BUFFER_SIZE, 4096*1000);

  int r = 0;
  do
  {
    r = ioctl(fd, DMX_SET_FILTER, &p);
  }
  while(r == -1 && errno == EINTR);
  if(r == -1)
  {
    printf("Error adding filter with error %d and errno %d", r, errno);
    abort();
  }

  middleware_api_sections_filter* result
    = (middleware_api_sections_filter*)
    malloc(sizeof(middleware_api_sections_filter));
  result->fd = fd;

  struct fd_ready_callback_state* callback_state
    = (struct fd_ready_callback_state*)
    malloc(sizeof(struct fd_ready_callback_state));
  callback_state->filter = result;
  callback_state->state = state;
  callback_state->callback = callback;
  middleware_api_sections_add_fd(fd, &fd_ready_callback, callback_state);

  return result;
}

middleware_api_sections_filter* middleware_api_sections_create_filter_for_pid
(uint16_t pid, middleware_api_sections_filter_callback_t callback, void* state)
{
  printf("middleware_api_sections_create_filter_for_pid pid %d\n", (int)pid);

  struct dmx_sct_filter_params p;
  memset(&p, 0, sizeof(p));
  p.pid = pid;
  p.flags = DMX_CHECK_CRC | DMX_IMMEDIATE_START;

  return create_filter_for_params(p, callback, state);
}

middleware_api_sections_filter* middleware_api_sections_create_filter_for_pid_and_table_id
(uint16_t pid, uint8_t table_id, middleware_api_sections_filter_callback_t callback
 , void* state)
{
  printf("middleware_api_sections_create_filter_for_pid pid %d table %d\n", (int)pid, (int)table_id);

  struct dmx_sct_filter_params p;
  memset(&p, 0, sizeof(p));
  p.pid = pid;
  p.flags = DMX_CHECK_CRC | DMX_IMMEDIATE_START;
  p.filter.filter[0] = table_id;
  p.filter.mask[0] = 0xFF;

  return create_filter_for_params(p, callback, state);
}

middleware_api_sections_filter*
middleware_api_sections_create_filter_for_pid_and_table_id_and_table_id_extension
(uint16_t pid, uint8_t table_id, uint16_t table_id_extension
 , middleware_api_sections_filter_callback_t callback
 , void* state)
{
  printf("middleware_api_sections_create_filter_for_pid pid %d table %d\n", (int)pid, (int)table_id);

  struct dmx_sct_filter_params p;
  memset(&p, 0, sizeof(p));
  p.pid = pid;
  p.flags = DMX_CHECK_CRC | DMX_IMMEDIATE_START;
  p.filter.filter[0] = table_id;
  p.filter.mask[0] = 0xFF;
  p.filter.filter[1] = (table_id_extension >> 8) & 0xFF;
  p.filter.mask[1] = 0xFF;
  p.filter.filter[2] = table_id_extension & 0xFF;
  p.filter.mask[2] = 0xFF;

  return create_filter_for_params(p, callback, state);
}

void middleware_api_sections_remove_filter(middleware_api_sections_filter* p)
{
  assert(p != 0);
  middleware_api_sections_rm_fd(p->fd);
}

