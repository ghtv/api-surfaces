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
  struct middleware_api_sections_filter* pat_filter;
  struct middleware_api_sections_filter* pmt_filter;
  unsigned int service_id;
};

void pmt_callback(const char* buffer, size_t size
                  , struct middleware_api_sections_filter* filter, void* state)
{
  printf ("pmt_callback\n");
  struct pat_state* p = (struct pat_state*)state;
  middleware_api_lifetime_start(p->service_id, p->pat_buffer, p->pat_size
                                , buffer, size);
  middleware_api_sections_remove_filter(p->pmt_filter);
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
      middleware_api_sections_remove_filter(p->pat_filter);
      p->pat_filter = 0;
      p->pmt_filter =
        middleware_api_sections_create_filter_for_pid(pmt_pid, &pmt_callback, state);
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
  middleware_api_sections_tune();
#endif

  char pat_buffer[4096];
  struct pat_state pat_state =
    {pat_buffer, 0u, 0, 0, 0u};

  pat_state.pat_filter = 
    middleware_api_sections_create_filter_for_pid(0, &pat_callback, &pat_state);

#ifndef MIDDLEWARE_API_IMPL_TS_FILE
  process_demultiplex_events();
#else
  pthread_t thread;
  pthread_create(&thread, 0, &demultiplex_thread, 0);

  middleware_api_sections_read();
#endif
  return 0;
}
