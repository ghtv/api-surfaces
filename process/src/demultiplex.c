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
#include <time.h>
#include <signal.h>

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

static int timer_expired = 0;

static void timer_expired_f(int sig, siginfo_t* si, void *uc)
{
  timer_expired = 1;
}

void middleware_api_graphics_draw_frame();

void process_demultiplex_events()
{
#ifndef _POSIX_THREAD_CPUTIME
#error _POSIX_THREAD_CPUTIME is required
#endif
  /* struct sigaction sa; */
  /* sa.sa_flags = SA_SIGINFO; */
  /* sa.sa_sigaction = &timer_expired_f; */
  /* sigaction(SIGRTMIN, &sa, 0); */

  /* timer_t timer_id; */
  /* struct sigevent se; */
  /* se.sigev_notify = SIGEV_SIGNAL; */
  /* se.sigev_signo = SIGRTMIN; */
  /* int r = timer_create(CLOCK_REALTIME, &se, &timer_id); */
  /* assert(r ==0 ); */

  /* struct itimerspec its; */
  /* its.it_interval.tv_sec = 0; */
  /* its.it_interval.tv_nsec = 16666666; */
  /* its.it_value.tv_sec = 0; */
  /* its.it_value.tv_nsec = 16666666; */
  /* timer_settime(timer_id, 0, &its, 0); */
  
  /* sigset_t sigmask; */
  /* sigemptyset(&sigmask); */
  /* sigaddset(&sigmask, SIGRTMIN); */
  /* struct timespec start_tm; */
  /* clock_gettime(CLOCK_REALTIME, &start_tm); */
  /* int fps = 0; */
  do
  {
    /* do */
    /* { */
      /* struct epoll_event events[10]; */
      /* int r = epoll_wait(epoll_instance, &events[0], 1, 0); */
      /* if(r == 1) */
      /* { */
      /*   struct process_demultiplex_event_data* data */
      /*     = (struct process_demultiplex_event_data*)events[0].data.ptr; */
      /*   assert(data != 0); */
      /*   data->callback(data->fd, data->state); */
      /* } */
      /* else if(r == -1) */
      /* { */
      /*   printf("Error calling epoll_wait with error %d and errno %d\n", r, errno); */
      /*   printf("epoll_instance %d\n", epoll_instance); */
      /*   break; */
      /* } */
    /* } */
    /* while(!timer_expired); */
    /* timer_expired = 0; */

    sleep(100);

    /* middleware_api_graphics_draw_frame(); */
    /* ++fps; */

    /* struct timespec tm; */
    /* clock_nanosleep(CLOCK_REALTIME, &tm); */
    /* if(tm.tv_sec - start_tm.tv_sec) */
    /* { */
    /*   printf("FPS: %d\n", fps); */
    /*   fps = 0; */
    /*   start_tm = tm; */
    /* } */

    /* struct timespec tm; */
    /* clock_gettime(CLOCK_REALTIME, &tm); */
    /* if(tm.tv_sec - start_tm.tv_sec) */
    /* { */
    /*   printf("FPS: %d\n", fps); */
    /*   fps = 0; */
    /*   start_tm = tm; */
    /* } */
  }
  while(1);
}

