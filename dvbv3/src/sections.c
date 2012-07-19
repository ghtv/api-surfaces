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

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <linux/dvb/frontend.h>

typedef struct middleware_api_sections_filter middleware_api_sections_filter;

static int frontend_fd;
static const int frequency = 623142857;

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

  struct dvb_frontend_parameters p;
  memset(&p, 0, sizeof(p));
  p.frequency = frequency;
  p.inversion = INVERSION_AUTO;
  p.u.ofdm.bandwidth = BANDWIDTH_6_MHZ;
  p.u.ofdm.code_rate_HP = p.u.ofdm.code_rate_LP = FEC_AUTO;
  p.u.ofdm.constellation = QAM_AUTO;
  p.u.ofdm.transmission_mode = TRANSMISSION_MODE_AUTO;
  p.u.ofdm.guard_interval = GUARD_INTERVAL_AUTO;
  p.u.ofdm.hierarchy_information = HIERARCHY_AUTO;

  int r;
  do
  {
    r = ioctl(frontend_fd, FE_SET_FRONTEND, &p);
  } while(r == -1 && errno == EINTR);
  if(r == -1)
  {
    fprintf(stderr, "Failed tuning to frequency %d with error %d and errno %d\n"
            , frequency, r, errno);
    fflush(stderr);
    abort();
  }

  printf("Succesfully tuned to frequency %d\n", frequency);
}

middleware_api_sections_filter* middleware_api_sections_create_filter_for_pid
(uint16_t pid, middleware_api_sections_filter_callback_type callback, void* state)
{
  return 0;
}

void middleware_api_sections_remove_filter(middleware_api_sections_filter* p)
{
}

