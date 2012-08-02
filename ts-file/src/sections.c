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
#define MIDDLEWARE_API_SECTIONS_SECTION_SIZE 4096


struct middleware_api_sections_filter
{
  char section[MIDDLEWARE_API_SECTIONS_SECTION_SIZE*2];
  unsigned int section_first, section_last;
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
    section_filters[i].filter->section_last = 0;
    section_filters[i].filter->section_first = 0;
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
    section_filters[i].filter->section_last = 0;
    section_filters[i].filter->section_first = 0;
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
        printf("packet %d\n", ++packet_number);
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
            printf("Pid we care about %d\n", (int)pid);

            // pre-condition:
            //  (Exists filter_index | filters[filter_index].pid == pid
            //    && !filters[filter_index].unused)
            //  && filter = filters[filter_index].filter
              

            // pre-condition: &filters[filter_index].filter->last
            //    = past-the-end section buffer

            middleware_api_sections_filter_t filter = section_filters[filter_index].filter;
            //printf("filter: %p\n", (void*)filter);
            int packet_continuity_counter = (buffer[sync+3] & 0xF)
              , next_continuity_counter = (filter->continuity_counter + 1) & 0xF
              , is_start_unit = buffer[sync+1] & 0x40;
            if((is_start_unit
                // repeated packet
                && !(packet_continuity_counter != filter->continuity_counter))
               || (packet_continuity_counter == next_continuity_counter
                   && filter->section_last != filter->section_first))
            {
              if(packet_continuity_counter != next_continuity_counter)
                filter->section_last = filter->section_first = 0;

              printf("Should filter based on sequence %d %d or because we're starting a new packet %d\n"
                     , (int)((filter->continuity_counter) & 0xF)
                     , (int)(buffer[sync+3] & 0xF)
                     , (int)(buffer[sync+1] & 0x40)?1:0);

              // pre-condition:
              //  (continuity_counter = 0xf & (current_section_{continuity_counter} + 1)
              //  || payload_unit_start_indicator

              int adaptation_length = 0;
              if(buffer[sync+3] & 0x20)
                adaptation_length += (unsigned char)buffer[sync+4] + 1;
              if((buffer[sync+3] & 0x30 && adaptation_length <= 183)
                 || adaptation_length <= 184)
              {
                //printf("adaptation_length makes sense\n");
                int payload_size = 184 - adaptation_length;

                // pre-condition:
                //  (adaptation_field_control == 0x11 && adaptation_length <= 183
                //    && payload_size == 184 - adaptation_length)
                //  || (adaptation_field_control == 0x10 && payload_size == 0
                //      && adaptation_length == 184)
                //  || (adaptation_field_control == 0x01 && payload_size == 184
                //      && adaptation_length == 0)

                unsigned int section_start = 0; // no start

                if(buffer[sync+1] & 0x40)
                {
                  section_start = (unsigned int)(unsigned char)buffer[sync+4+adaptation_length] + 1;
                }
                
                assert(payload_size + adaptation_length == 184);
                assert(payload_size >= 0);
                assert(adaptation_length >= 0);

                assert(filter->section_last <= sizeof(filter->section));
                if(section_start < payload_size)
                {
                  printf("section_start(%d) < payload_size(%d)\n", (int)section_start, (int)payload_size);
                  /* printf("filter->last: %d\n", filter->last); */

                  int length = sizeof(filter->section) - filter->section_last < payload_size
                    ? sizeof(filter->section) - filter->section_last : payload_size;

                  printf("length: %d\n", length);

                  /* assert(length == payload_size-skip); */

                  if(length)
                  {
                    /* int padding_last = length */
                    /*   , beggining = sync+4+adaptation_length+skip; */
                    /* while(padding_last != 0 */
                    /*       && (unsigned char)buffer[padding_last+beggining-1] == 0xFF) */
                    /*   --padding_last; */
                    /* /\* if(padding_last != length) *\/ */
                    /* /\*   printf("padding %d\n", (int)((length) - padding_last)); *\/ */
                    /* /\* else if(padding_last) *\/ */
                    /* /\*   printf("last byte: %d\n", (unsigned char)buffer[padding_last+beggining-1]); *\/ */
                    /* length = padding_last; */
                  }

                  memcpy(&filter->section[filter->section_last]
                         , &buffer[sync+4+adaptation_length]
                         , length);

                  int old_last = filter->section_last;

                  filter->section_last += length;
                  filter->continuity_counter = buffer[sync+3] & 0xF;

                  /* printf("new last: %d\n", (int)length); */
                  /* printf("new continuity counter: %d\n", (int)filter->continuity_counter); */

                  if(filter->is_section)
                  {
                    int section_last = old_last + section_start - 1;
                    printf("partial last packet %d %d %d\n", filter->section_first
                           , section_last
                           , section_last - filter->section_first);

                    if(section_start)
                    {
                      if(filter->section_first != old_last)
                      {
                        printf("There was a packet %d %d\n"
                               , (int)filter->section_first
                               , (int)(section_last - filter->section_first));
                        if((filter->section[filter->section_first+1] & 0xB0) == 0xB0)
                        {
                          printf("Well formed section\n");
                          // 13818-2 2.4.4.11

#ifndef NDEBUG
                          uint16_t section_size;
                          ((char*)&section_size)[0]
                            = filter->section[filter->section_first+2];
                          ((char*)&section_size)[1]
                            = filter->section[filter->section_first+1] & 0xF;
                          printf("section_size: %d\n", (int)section_size + 3);
                          char dummy = 0;
                          if(section_size + 3 < section_last - filter->section_first)
                          {
                            int first = filter->section_first + section_size + 3
                              , first_old = first;
                            printf("section[%d]: %d\n"
                                   , filter->section_first + section_size + 3
                                   , ((int)(unsigned char)filter->section[first]));
                            while(first != section_last
                                  && (((int)(unsigned char)filter->section[first]) == 0xff))
                              ++first;
                            printf("first: %d first_old: %d\n", first, first_old);
                            assert((int)(unsigned char)filter->section[first_old] == 0xff);
                            if(first == section_last)
                            {
                              dummy = 1;
                              printf("All other bytes are 0xff\n");
                            }
                            section_last = section_size + 3 + filter->section_first;
                          }
                          assert(section_last - filter->section_first == section_size + 3
                                 || dummy);
#endif                          

                          // filter->section_first = start-of-packet
                          // section_last = past-the-end-of-packet

                          printf("table_id_size %d\n", (int)filter->table_id_size);
                          if(filter->table_id_size == 0
                             || (filter->table_id_size == 1
                                 && filter->table_id[0] 
                                 == filter->section[filter->section_first]))
                          {
                            printf("table_id: %d expected %d\n"
                                   , (int)filter->table_id[0]
                                   , (int)filter->section[filter->section_first]);

                            /* int i = 0; */
                            /* for(; i != MIDDLEWARE_API_SECTIONS_CALLBACK_SIZE */
                            /*       && filter->callbacks[i]; ++i) */
                            /* { */
                            if(filter->callbacks[0])
                              filter->callbacks[0](&filter->section
                                                   [filter->section_first]
                                                   , section_last - filter->section_first
                                                   , filter, filter->states[0]);
                            /* } */
                          }

                          int new_section_first = section_start + old_last;

                          printf("moving %d to %d with size %d %d\n"
                                 , new_section_first, 0
                                 , filter->section_last - new_section_first
                                 , filter->section_last);

                          memmove(&filter->section[0]
                                  , &filter->section[new_section_first]
                                  , filter->section_last - new_section_first);
                          filter->section_first = 0;
                          filter->section_last -= new_section_first;

                          printf("new first %d last %d\n", 0
                                 , filter->section_last);
                        }
                      }
                      else
                        filter->section_first = section_start;
                    }
                  }
                  
                  if(filter->section_last == 4096)
                    filter->section_last = filter->section_first = 0;
                }                
                else
                {
                  /* printf("skip > payload_size skip: %d payload_size: %d\n" */
                  /*        , (int)skip, (int)payload_size); */
                  filter->section_last = filter->section_first = 0;
                }
              }
            }
            else if(packet_continuity_counter != filter->continuity_counter
                    && packet_continuity_counter != next_continuity_counter)
            {
              printf("Discontinuity\n");
              filter->section_last = filter->section_first = 0;
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

