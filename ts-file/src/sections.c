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

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

typedef void(*process_demultiplex_callback)(int, void*);

struct middleware_api_sections_filter
{
  char section[4096];
  unsigned int last;
  uint8_t continuity_counter;
};

struct filter_information
{
  uint16_t pid;
  char unused;
  middleware_api_sections_filter_t filter;
};

#define MIDDLEWARE_API_SECTIONS_FILTER_SIZE 16
#define MIDDLEWARE_API_SECTIONS_PACKET_SIZE 188

static struct filter_information section_filters[MIDDLEWARE_API_SECTIONS_FILTER_SIZE];

middleware_api_sections_filter_t middleware_api_sections_create_filter_for_pid
(uint16_t pid, middleware_api_sections_filter_callback_t callback, void* state)
{
  return 0;
}

middleware_api_sections_filter_t middleware_api_sections_create_filter_for_pid_and_table_id
(uint16_t pid, uint8_t table_id, middleware_api_sections_filter_callback_t callback
 , void* state)
{
  return 0;
}

middleware_api_sections_filter_t
middleware_api_sections_create_filter_for_pid_and_table_id_and_table_id_extension
(uint16_t pid, uint8_t table_id, uint16_t table_id_extension
 , middleware_api_sections_filter_callback_t callback
 , void* state)
{
  return 0;
}

void middleware_api_sections_remove_filter(middleware_api_sections_filter_t p)
{
}

void middleware_api_sections_read()
{
  section_filters[0].unused = 0;
  section_filters[0].pid = 0;
  section_filters[0].filter = (middleware_api_sections_filter_t)
    malloc(sizeof(struct middleware_api_sections_filter));
  section_filters[0].filter->last = 0;
  section_filters[0].filter->continuity_counter = 0;

  {
    int i = 1;
    for(; i != MIDDLEWARE_API_SECTIONS_FILTER_SIZE; ++i)
    {
      section_filters[i].unused = 1;
    }
  }

  int fd = open("file.ts", O_RDONLY);
  if(fd < 0)
  {
    fprintf(stderr, "Error opening file file.ts\n");
    abort();
  }

  char buffer[1024];
  int r = 0, offset = 0, sync = 0, packet_number = 0;
  do
  {
    do
    {
      r = read(fd, &buffer[offset], sizeof(buffer) - offset);
    }
    while(r == -1 && errno == EINTR);

    if(r > 0)
    do
    {
      while((offset + r) - sync >= 188 && buffer[sync] != 0x47)
        ++sync;


      if((offset + r) - sync >= MIDDLEWARE_API_SECTIONS_PACKET_SIZE)
      {
        printf("packet %d\n", packet_number++);
        // pre-condition: buffer[sync] = 0x47 && (offset + r) - sync >= 188
        int filter_index = 0;
        for(; filter_index != MIDDLEWARE_API_SECTIONS_FILTER_SIZE
              && !section_filters[filter_index].unused; ++filter_index)
        {
          uint16_t pid;
#if 0
          memcpy(&pid, &buffer[sync+1], 2);
#else
          ((char*)&pid)[0] = buffer[sync+2];
          ((char*)&pid)[1] = buffer[sync+1];
#endif
          pid &= 0x5FF;
          
          printf("  pid: %d\n", (int)pid);

          if(!section_filters[filter_index].unused
             && section_filters[filter_index].pid == pid)
          {
            // pre-condition:
            //  (Exists filter_index | filters[filter_index].pid == pid
            //    && !filters[filter_index].unused)
            //  && filter = filters[filter_index].filter
              

            // pre-condition: &filters[filter_index].filter->last
            //    = past-the-end section buffer

            middleware_api_sections_filter_t filter = section_filters[filter_index].filter;
            if((filter->last == 0 && buffer[sync+1] & 0x40)
               && (((filter->continuity_counter + 1) & 0xF) == (buffer[sync+3] & 0xF)))
            {
              // pre-condition:
              //  current_section{pid} = pid
              //  && (continuity_counter = 0xf & (current_section_{continuity_counter} + 1)
              //      && !payload_unit_start_indicator)
              //  || (section_last == 0 && payload_unit_start_indicator)

              int adaptation_length = 0;
              if(buffer[sync+3] & 0x20)
                adaptation_length += buffer[sync+4] + 1;
              if((buffer[sync+3] & 0x30 && adaptation_length <= 183)
                 || adaptation_length <= 184)
              {
                int payload_size = 184 - adaptation_length;

                // pre-condition:
                //  (adaptation_field_control == 0x11 && adaptation_length <= 183
                //    && payload_size == 184 - adaptation_length)
                //  || (adaptation_field_control == 0x10 && payload_size == 0
                //      && adaptation_length == 184)
                //  || (adaptation_field_control == 0x01 && payload_size == 184
                //      && adaptation_length == 0)
                
                assert(payload_size + adaptation_length == 184);

                memcpy(&filter->section[filter->last]
                       , &buffer[sync+4+adaptation_length]
                       , payload_size);
                
                filter->last += payload_size;

                printf("Copied\n");

                  
              }                
            }
            else if(!(buffer[sync+2] & 0x40))
            {
              if((filter->continuity_counter & 0xF) == (buffer[sync+3] & 0xF))
                // repeated packet
                ;
              else if(((filter->continuity_counter + 1) & 0xF) < (buffer[sync+3] & 0xF))
              {
                // lost packet
              }
            }
          }
        }
      }
      
      sync += MIDDLEWARE_API_SECTIONS_PACKET_SIZE;
    }
    while((offset + r) - sync >= MIDDLEWARE_API_SECTIONS_PACKET_SIZE);

    memmove(&buffer[0], &buffer[sync], (offset+r)-sync);

    offset = (offset+r)-sync;
    sync = 0;
  }
  while(r != -1);

}

