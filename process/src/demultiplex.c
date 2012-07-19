/* (c) Copyright 2012 Felipe Magno de Almeida
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#include <process/demultiplex.h>

#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

static int epoll_instance;

struct process_demultiplex_event_data
{
  int fd;
  void* state;
  process_demultiplex_callback callback;
};

void process_demultiplex_initialize()
{
  epoll_instance = epoll_create1(0);
  if(epoll_instance < 0)
  {
    printf("Couldn't create epoll instance error %d errno %d", epoll_instance, errno);
    abort();
  }
  else
    printf("epoll_instance %d\n", epoll_instance);
}

// TODO: Still leaks callback state information
void process_demultiplex_rm_fd(int fd)
{
  printf("process_demultiplex_rm_fd\n");
  close(fd);
}

void process_demultiplex_add_fd(int fd, process_demultiplex_callback callback, void* state)
{
  printf("process_demultiplex_add_fd\n");
  assert(fd >= 0);

  struct process_demultiplex_event_data* data
    = (struct process_demultiplex_event_data*)
    malloc(sizeof(struct process_demultiplex_event_data));
  assert(data != 0);
  data->fd = fd;
  data->callback = callback;
  data->state = state;

  struct epoll_event event = {EPOLLIN};
  event.data.ptr = data;

  int r = 0;
  do
  {
    r = epoll_ctl(epoll_instance, EPOLL_CTL_ADD, fd, &event);
  }
  while(r == -1 && errno == EINTR);
  if(r == -1)
  {
    printf("Error while calling epoll_ctl error %d errno %d\n", r, errno);
    abort();
  }
}

void process_demultiplex_events()
{
  do
  {
    struct epoll_event events[10];
    int r = epoll_wait(epoll_instance, &events[0], 1, -1);
    printf("Returned from epoll_wait return %d\n", r);
    if(r == 1)
    {
      printf("calling callback\n");
      struct process_demultiplex_event_data* data
        = (struct process_demultiplex_event_data*)events[0].data.ptr;
      assert(data != 0);
      data->callback(data->fd, data->state);
    }
    else if(r == -1)
    {
      printf("Error calling epoll_wait with error %d and errno %d\n", r, errno);
      printf("epoll_instance %d\n", epoll_instance);
      break;
    }
  }
  while(1);
}

