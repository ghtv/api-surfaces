/* (c) Copyright 2012 Felipe Magno de Almeida
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#include <process/demultiplex.h>
#include <middleware-api/lifetime.h>
#include <middleware-api/sections.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <pthread.h>

void middleware_api_graphics_initialize(int argc, char** argv);
int middleware_api_input_initialize();

#ifndef MIDDLEWARE_API_IMPL_TS_FILE
void middleware_api_sections_tune();
#else
void middleware_api_sections_read();
void middleware_api_sections_initialize();
#endif

void middleware_api_input_ready_fd(int fd, void* state);

void middleware_api_sections_add_fd(int fd, process_demultiplex_callback callback
                                    , void* state)
{
  assert(fd >= 0);
  process_demultiplex_add_fd(fd, callback, state);
}

void middleware_api_sections_rm_fd(int fd)
{
  process_demultiplex_rm_fd(fd);
}

struct pat_state
{
  char* pat_buffer;
  size_t pat_size;
  unsigned int service_id;
  int audio_pid, video_pid;
};

void middleware_api_video_sections(const char* buffer, size_t size
                                   , middleware_api_sections_filter_t filter
                                   , void* state);

void pmt_callback(const char* buffer, size_t size
                  , struct middleware_api_sections_filter* filter, void* state)
{
  printf ("pmt_callback\n");
  struct pat_state* p = (struct pat_state*)state;

  assert(size > 14);
  size_t pi_length = 0;
  ((char*)&pi_length)[3] = buffer[10];
  ((char*)&pi_length)[2] = buffer[11];
  pi_length &= 0xFFFFFF;
  printf("pi_length: %d\n", (int)pi_length);

  size_t video_pid = 0, audio_pid = 0;
  size_t off = 12 + pi_length;
  while((video_pid == 0 || audio_pid == 0)
        && off + 4 < size)
  {
    printf("stream type: %d\n", (int)buffer[off]);
    unsigned short pid = 0;
    ((char*)&pid)[1] = buffer[off+1];
    ((char*)&pid)[0] = buffer[off+2];
    pid &= 0x1FFF;
    printf("ePID: %d\n", (int)pid);
    unsigned short info_l = 0;
    ((char*)&info_l)[1] = buffer[off+3];
    ((char*)&info_l)[0] = buffer[off+4];
    info_l &= 0xFFF;
    printf("info length: %d\n", (int)info_l);

    if(buffer[off] == 27)
      video_pid = pid;
    else if(!audio_pid && buffer[off] == 17)
      audio_pid = pid;

    if(off + 5 + info_l < size)
      off += 5 + info_l;
    else
      break;
  }
  p->audio_pid = audio_pid;
  p->video_pid = video_pid;

  /* middleware_api_sections_create_filter_for_pid */
  /*   (video_pid, &middleware_api_video_sections, 0); */

  middleware_api_lifetime_start(p->service_id, p->pat_buffer, p->pat_size
                                , buffer, size);
  middleware_api_sections_remove_filter(filter);
}

void pat_callback(const char* buffer, size_t size
                  , struct middleware_api_sections_filter* filter, void* state)
{
  printf ("pat_callback\n");
  assert(size <= 4096);

  const char* iterator = buffer + 5
    , *end = buffer + size;
  if(*iterator & 1) // current next indicator
  {
    printf("pat_callback current_next_indicator\n");
    iterator += 3;
    
    while(iterator < end - 4 && !*iterator && !*(iterator+1))
      iterator += 4;

    if(iterator < end - 4)
    {
      uint16_t service_id, pmt_pid;
#if defined(BYTE_ORDER) && BYTE_ORDER == 4321
      memcpy(&service_id, iterator, sizeof(service_id));
      memcpy(&pmt_pid, iterator+2, sizeof(pmt_pid));
#else
      assert(sizeof(service_id) == 2);
      ((char*)&service_id)[1] = *(iterator+0);
      ((char*)&service_id)[0] = *(iterator+1);
      ((char*)&pmt_pid)[1] = *(iterator+2) & 0x1F;
      ((char*)&pmt_pid)[0] = *(iterator+3);
#endif
      printf("service_id %d pmt_pid %d\n", (int)service_id, (int)pmt_pid);
      struct pat_state* p = (struct pat_state*)state;
      p->pat_size = size;
      p->service_id = service_id;
      memcpy(p->pat_buffer, buffer, size);
      middleware_api_sections_remove_filter(filter);
      middleware_api_sections_create_filter_for_pid_and_table_id(pmt_pid, 0x02, &pmt_callback, state);
    }
  }
  else
    printf("! pat_callback current_next_indicator\n");
}

void* demultiplex_thread(void* p)
{
  (void)p;
  process_demultiplex_events();
  return 0;
}

int main(int argc, char** argv)
{
  process_demultiplex_initialize();
  middleware_api_graphics_initialize(argc, argv);
  int input_fd = middleware_api_input_initialize();
  process_demultiplex_add_fd(input_fd, &middleware_api_input_ready_fd, 0);
#ifndef MIDDLEWARE_API_IMPL_TS_FILE
  if(argc <= 1 || strcmp(argv[1], "--no-tune"))
    middleware_api_sections_tune();
#else
  middleware_api_sections_initialize();
#endif

  char pat_buffer[4096];
  struct pat_state pat_state = {pat_buffer, 0u, 0u};

  middleware_api_sections_create_filter_for_pid(0, &pat_callback, &pat_state);
  middleware_api_sections_create_pes_filter_for_pid
    (513, &middleware_api_video_sections, 0);

#ifndef MIDDLEWARE_API_IMPL_TS_FILE
  process_demultiplex_events();
#else
  pthread_t thread;
  pthread_create(&thread, 0, &demultiplex_thread, 0);

  middleware_api_sections_read();
#endif
  return 0;
}
