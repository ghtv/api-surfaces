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

#define MIDDLEWARE_API_SECTIONS_FILTER_SIZE 16
#define MIDDLEWARE_API_SECTIONS_CALLBACK_SIZE 16
#define MIDDLEWARE_API_SECTIONS_TABLE_ID_SIZE 16
#define MIDDLEWARE_API_SECTIONS_PACKET_SIZE 188


struct middleware_api_sections_filter
{
  char section[4096];
  unsigned int last;
  uint8_t continuity_counter;
  char is_section;
  uint8_t table_id[MIDDLEWARE_API_SECTIONS_TABLE_ID_SIZE];
  size_t table_id_size;
  middleware_api_sections_filter_callback_t callbacks[MIDDLEWARE_API_SECTIONS_CALLBACK_SIZE];
  void* states[MIDDLEWARE_API_SECTIONS_CALLBACK_SIZE];
};

struct filter_information
{
  uint16_t pid;
  char unused;
  middleware_api_sections_filter_t filter;
};

static struct filter_information section_filters[MIDDLEWARE_API_SECTIONS_FILTER_SIZE];

void middleware_api_sections_initialize()
{
  int i = 0;
  for(; i != MIDDLEWARE_API_SECTIONS_FILTER_SIZE; ++i)
    section_filters[i].unused = 1;
}

middleware_api_sections_filter_t middleware_api_sections_create_filter_for_pid
(uint16_t pid, middleware_api_sections_filter_callback_t callback, void* state)
{
  printf("middleware_api_sections_create_filter_for_pid %d\n", (int)pid);
  assert(callback != 0);
  int i = 0;
  while(i != MIDDLEWARE_API_SECTIONS_FILTER_SIZE && !section_filters[i].unused)
    ++i;
  if(i != MIDDLEWARE_API_SECTIONS_FILTER_SIZE)
  {
    printf("Using slot %d\n", i);
    section_filters[i].unused = 0;
    section_filters[i].pid = pid;
    section_filters[i].filter = (middleware_api_sections_filter_t)
      malloc(sizeof(struct middleware_api_sections_filter));
    section_filters[i].filter->last = 0;
    section_filters[i].filter->continuity_counter = 0;
    section_filters[i].filter->is_section = 1;
    section_filters[i].filter->table_id_size = 0;
    memset(section_filters[i].filter->table_id, 0
           , sizeof(section_filters[0].filter->table_id));
    memset(section_filters[i].filter->callbacks, 0
           , sizeof(section_filters[0].filter->callbacks));
    memset(section_filters[i].filter->states, 0
           , sizeof(section_filters[0].filter->states));
    section_filters[i].filter->callbacks[0] = callback;
    section_filters[i].filter->states[0] = state;
    return section_filters[i].filter;
  }
  else
    return 0;
}

middleware_api_sections_filter_t middleware_api_sections_create_filter_for_pid_and_table_id
(uint16_t pid, uint8_t table_id, middleware_api_sections_filter_callback_t callback
 , void* state)
{
  printf("middleware_api_sections_create_filter_for_pid_and_table_id %d %d\n", (int)pid, (int)table_id);
  assert(callback != 0);
  int i = 0;
  while(i != MIDDLEWARE_API_SECTIONS_FILTER_SIZE && !section_filters[i].unused)
    ++i;
  if(i != MIDDLEWARE_API_SECTIONS_FILTER_SIZE)
  {
    printf("Using slot %d\n", i);
    section_filters[i].unused = 0;
    section_filters[i].pid = pid;
    section_filters[i].filter = (middleware_api_sections_filter_t)
      malloc(sizeof(struct middleware_api_sections_filter));
    section_filters[i].filter->last = 0;
    section_filters[i].filter->continuity_counter = 0;
    section_filters[i].filter->is_section = 1;
    section_filters[i].filter->table_id_size = 1;
    memset(section_filters[i].filter->table_id, 0
           , sizeof(section_filters[0].filter->table_id));
    memset(section_filters[i].filter->callbacks, 0
           , sizeof(section_filters[0].filter->callbacks));
    memset(section_filters[i].filter->states, 0
           , sizeof(section_filters[0].filter->states));
    section_filters[i].filter->callbacks[0] = callback;
    section_filters[i].filter->states[0] = state;
    section_filters[i].filter->table_id[0] = table_id;
    return section_filters[i].filter;
  }
  else
    return 0;
}

void middleware_api_sections_remove_filter(middleware_api_sections_filter_t p)
{
  printf("middleware_api_sections_remove_filter\n");
  int i = 0;
  while(i != MIDDLEWARE_API_SECTIONS_FILTER_SIZE && section_filters[i].filter != p)
    ++i;
  printf("Removing slot %d\n", i);
  assert(i != MIDDLEWARE_API_SECTIONS_FILTER_SIZE);
  int j = i + 1, q = MIDDLEWARE_API_SECTIONS_FILTER_SIZE;
  
  for(;j != q; ++j, ++i)
  {
    section_filters[i] = section_filters[j];
  }
  section_filters[MIDDLEWARE_API_SECTIONS_FILTER_SIZE-1].unused = 1;
}

void middleware_api_sections_read()
{
  int fd = open("file.ts", O_RDONLY);
  if(fd < 0)
  {
    fprintf(stderr, "Error opening file file.ts\n");
    abort();
  }

  char buffer[1024];
  int r = 0, offset = 0, sync = 0;
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
        //printf("packet %d\n", packet_number++);
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
          ((char*)&pid)[1] = buffer[sync+1] & 0x1F;
#endif
          
          /* printf("  pid: %d\n", (int)pid); */

          if(!section_filters[filter_index].unused
             && section_filters[filter_index].pid == pid)
          {
            //printf("Pid we care about %d\n", (int)pid);

            // pre-condition:
            //  (Exists filter_index | filters[filter_index].pid == pid
            //    && !filters[filter_index].unused)
            //  && filter = filters[filter_index].filter
              

            // pre-condition: &filters[filter_index].filter->last
            //    = past-the-end section buffer

            middleware_api_sections_filter_t filter = section_filters[filter_index].filter;
            if((filter->last == 0 && buffer[sync+1] & 0x40)
               || (((filter->continuity_counter + 1) & 0xF) == (buffer[sync+3] & 0xF)
                   && filter->last != 0))
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
                assert(payload_size >= 0);
                assert(adaptation_length >= 0);

                assert(filter->last <= 4096);
                int length = 4096 - filter->last < payload_size
                  ? 4096 - filter->last : payload_size;

                memcpy(&filter->section[filter->last]
                       , &buffer[sync+4+adaptation_length]
                       , length);
                
                filter->last += length;
                filter->continuity_counter = buffer[sync+3] & 0xF;

                if(filter->is_section)
                {
                  int skip = (unsigned int)filter->section[0] + 1;
                  if(skip < filter->last)
                  {
                    if(filter->last - skip > 3)
                    {
                      if((filter->section[skip+1] & 0xB0) == 0xB0)
                      {
                        uint16_t section_size;
#if 0

#else
                        ((char*)&section_size)[0] = filter->section[skip+2];
                        ((char*)&section_size)[1] = filter->section[skip+1] & 0xF;
#endif
                        if(section_size <= filter->last - skip)
                        {
                          printf("table_id_size %d\n", (int)filter->table_id_size);
                          if(filter->table_id_size == 0
                             || (filter->table_id_size == 1
                                 && filter->table_id[0] == filter->section[skip]))
                          {
                            int i = 0;
                            for(; i != MIDDLEWARE_API_SECTIONS_CALLBACK_SIZE
                                  && filter->callbacks[i]; ++i)
                            {
                              filter->callbacks[i](&filter->section[skip]
                                                   , section_size
                                                   , filter, filter->states[i]);
                            }
                          }
                          filter->last = 0;
                        }
                      }
                    }
                  }
                }

                if(filter->last == 4096)
                  filter->last = 0;
              }                
            }
            else if(!(buffer[sync+2] & 0x40))
            {
              if((filter->continuity_counter & 0xF) == (buffer[sync+3] & 0xF))
                // repeated packet
                //printf("repeated packet %d\n", (int)(filter->continuity_counter & 0xF))
                  ;
              else if(((filter->continuity_counter + 1) & 0xF) < (buffer[sync+3] & 0xF))
              {
                // lost packet
                //printf("Lost packet somewhere\n");
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

