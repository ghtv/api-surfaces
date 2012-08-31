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
#define MIDDLEWARE_API_SECTIONS_RPS(filter) (filter->packets)
#define MIDDLEWARE_API_SECTIONS_CC_RP(filter, n)                        \
  (n == filter->packets? filter->continuity_counter[1] : filter->continuity_counter[2])
#define MIDDLEWARE_API_SECTIONS_PUSH_RP(filter, packet)                 \
  filter->packets++;                                                    \
  filter->continuity_counter[0] = filter->continuity_counter[1];        \
  filter->continuity_counter[1] = packet[3] & 0xF;
#define MIDDLEWARE_API_SECTIONS_SUC_CC(cc) ((cc + 1) & 0xF)

struct middleware_api_sections_filter
{
  char section[MIDDLEWARE_API_SECTIONS_SECTION_SIZE*2];
  unsigned int section_last;
  unsigned int packets;
  uint8_t continuity_counter[2];
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
    section_filters[i].filter->continuity_counter[0] = 0;
    section_filters[i].filter->continuity_counter[1] = 0;
    section_filters[i].filter->packets = 0;
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
    section_filters[i].filter->continuity_counter[0] = 0;
    section_filters[i].filter->continuity_counter[1] = 0;
    section_filters[i].filter->packets = 0;
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

static void middleware_api_sections_notify_callback(middleware_api_sections_filter_t filter)
{
  /* if(filter->is_section) */
  /* { */
  if(filter->section_last)
  {
    printf("There was a packet %d\n"
           , (int)filter->section_last);
    if((filter->section[1] & 0xB0) == 0xB0)
    {
      printf("Well formed section\n");
      // 13818-2 2.4.4.11

/* #ifndef NDEBUG */
/*                         uint16_t section_size; */
/*                         ((char*)&section_size)[0] */
/*                           = filter->section[filter->section_first+2]; */
/*                         ((char*)&section_size)[1] */
/*                           = filter->section[filter->section_first+1] & 0xF; */
/*                         printf("section_size: %d\n", (int)section_size + 3); */
/*                         char dummy = 0; */
/*                         if(section_size + 3 < section_last - filter->section_first) */
/*                         { */
/*                           int first = filter->section_first + section_size + 3 */
/*                             , first_old = first; */
/*                           printf("section[%d]: %d\n" */
/*                                  , filter->section_first + section_size + 3 */
/*                                  , ((int)(unsigned char)filter->section[first])); */
/*                           while(first != section_last */
/*                                 && (((int)(unsigned char)filter->section[first]) == 0xff)) */
/*                             ++first; */
/*                           printf("first: %d first_old: %d\n", first, first_old); */
/*                           assert((int)(unsigned char)filter->section[first_old] == 0xff); */
/*                           if(first == section_last) */
/*                           { */
/*                             dummy = 1; */
/*                             printf("All other bytes are 0xff\n"); */
/*                           } */
/*                           section_last = section_size + 3 + filter->section_first; */
/*                         } */
/*                         assert(section_last - filter->section_first == section_size + 3 */
/*                                || dummy); */
/* #endif */

/*                         // filter->section_first = start-of-packet */
/*                         // section_last = past-the-end-of-packet */

      printf("table_id_size %d\n", (int)filter->table_id_size);
      if(filter->table_id_size == 0
         || (filter->table_id_size == 1
             && filter->table_id[0]
             == filter->section[0]))
      {
        printf("table_id: %d expected %d\n"
               , (int)filter->table_id[0]
               , (int)filter->section[0]);
        
        int i = 0;
        for(; i != MIDDLEWARE_API_SECTIONS_CALLBACK_SIZE
              && filter->callbacks[i]; ++i)
        {
          if(filter->callbacks[0])
            filter->callbacks[0](&filter->section[0]
                                 , filter->section_last
                                 , filter, filter->states[0]);
        }
      }
    }
  }
  
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
        /** proof

theory Walkthrough imports Main begin

theorem other_theorem: "B & A \<longrightarrow> A & B

proof
  assume "B & A"

then obtain A and B ..

then show B and A ..


**/

        // pre-condition: buffer[sync] = 0x47 ^ (offset + r) - sync >= 188
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

          const char* packet_first = &buffer[sync]
            , *packet_last = &buffer[sync + 188];


          // pre-condition: buffer[sync] = 0x47 ^ (offset + r) - sync >= 188
          if(!section_filters[filter_index].unused
             && section_filters[filter_index].pid == pid)
          {
            printf("Pid we care about %d\n", (int)pid);

            // filtering-packet = buffer[sync : sync + 188]

            // var filter->received-packet [ 1 : filter->packet_num ]
            //   of packets

            // continuity-counter ( packet ): integer
            //  return packet[3] & 0xF

            // continuity-property (k)
            //  (A k : 1 <= k < filter->packet_num :
            //    (continuity-counter(filter->received-packet [k])
            //     = continuity-counter(filter->received-packet [k+1])))

            // pre-condition:
            //  (Exists filter_index | filters[filter_index].pid == pid
            //    ^ !filters[filter_index].unused)
            //  ^ filter = filters[filter_index].filter
            // pre-condition: &filters[filter_index].filter->last
            //    = past-the-end section buffer

            middleware_api_sections_filter_t filter = section_filters[filter_index].filter;
            //printf("filter: %p\n", (void*)filter);

            // pre-condition: buffer[sync] = 0x47 ^ (offset + r) - sync >= 188
            // pre-condition: continuity-property

            // B_{1} = (rps f) = 0 /\ (payload_unit_start_indicator p)
            // B_{2} = (rps f) >= 1 cand (cc p) = (suc (cc (rp (rps f) f)))
            // B_{3} = (rps f) >= 2 cand ((cc p) = (cc (rp (rps f) f))
            //         /\ (cc (rp (rps f) f)) = (suc (cc (rp ((rps f) - 1) f)))
            // B_{4} = \neg B1 /\ \neg B2 /\ \neg B3

            // S_{1} = S_{2} /\ S_{2} = S_{3}
            // S_{3} = (rp f), rps := (rp f)[(suc (rps f)):= p], rps[f:= (suc (rps f))]
            
            int cc_packet = (packet_first[3] & 0xF);
            if(MIDDLEWARE_API_SECTIONS_RPS(filter) >= 2
               && (cc_packet == MIDDLEWARE_API_SECTIONS_CC_RP(filter, MIDDLEWARE_API_SECTIONS_RPS(filter))
                   && MIDDLEWARE_API_SECTIONS_CC_RP(filter, MIDDLEWARE_API_SECTIONS_RPS(filter)-1)
                   == MIDDLEWARE_API_SECTIONS_SUC_CC
                   (MIDDLEWARE_API_SECTIONS_CC_RP(filter, MIDDLEWARE_API_SECTIONS_RPS(filter)))))
            {
              printf("Repeated packet\n");
              MIDDLEWARE_API_SECTIONS_PUSH_RP(filter, packet_first);
            }
            else if((MIDDLEWARE_API_SECTIONS_RPS(filter) == 0 && (packet_first[1] & 0x40))
                    || (MIDDLEWARE_API_SECTIONS_RPS(filter) >= 1
                        && (cc_packet == MIDDLEWARE_API_SECTIONS_SUC_CC
                            (MIDDLEWARE_API_SECTIONS_CC_RP(filter, MIDDLEWARE_API_SECTIONS_RPS(filter))))))
            {

              // pre-condition: continuity-property
              // pre-condition: buffer[sync] = 0x47 ^ (offset + r) - sync >= 188
              // pre-condition:
              //  (Exists filter_index | filters[filter_index].pid == pid
              //    ^ !filters[filter_index].unused)
              //  ^ filter = filters[filter_index].filter
              // pre-condition: &filters[filter_index].filter->last
              //    = past-the-end section buffer


              printf("Should filter based on sequence %d %d or because we're starting a new packet %d\n"
                     , (int)(MIDDLEWARE_API_SECTIONS_CC_RP(filter, MIDDLEWARE_API_SECTIONS_RPS(filter)))
                     , (int)(cc_packet)
                     , (int)(packet_first[1] & 0x40)?1:0);

              MIDDLEWARE_API_SECTIONS_PUSH_RP(filter, packet_first);

              int adaptation_total_length = 0;
              if(packet_first[3] & 0x20)
                adaptation_total_length += (unsigned char)packet_first[4] + 1;
              if((packet_first[3] & 0x30 && adaptation_total_length <= 183)
                 || adaptation_total_length <= 184)
              {
                printf("adaptation_total_length makes sense\n");
                int payload_size = 184 - adaptation_total_length;

                assert(payload_size + adaptation_total_length == 184);
                assert(payload_size >= 0);
                assert(adaptation_total_length >= 0);

                // pre-condition:
                //  (adaptation_field_control == 0x11 && adaptation_length <= 183
                //    && payload_size == 184 - adaptation_length)
                //  || (adaptation_field_control == 0x10 && payload_size == 0
                //      && adaptation_length == 184)
                //  || (adaptation_field_control == 0x01 && payload_size == 184
                //      && adaptation_length == 0)

                unsigned int skip = 0;

                if(packet_first[1] & 0x40)
                {
                  printf("treat pointer\n");
                  unsigned char skip_c = packet_first[4+adaptation_total_length] + 1;
                  skip = skip_c;

                  if(filter->section_last)
                  {
                    printf("Is first and is last\n");

                    if(skip < payload_size && skip >= 1)
                    {
                      memcpy(&filter->section[filter->section_last]
                             , &packet_first[4+adaptation_total_length+1] /* +1 for pointer byte */
                             , skip-1);
                      filter->section_last += skip - 1;
                      middleware_api_sections_notify_callback(filter);
                    }
                    else
                      printf("Malformed skip\n");
                    filter->section_last = 0;
                  }
                }
                  
                assert(filter->section_last <= sizeof(filter->section));
                if(skip < payload_size)
                {
                  printf("skip(%d) < payload_size(%d)\n"
                         , (int)skip, (int)payload_size);
                  /* printf("filter->last: %d\n", filter->last); */

                  int length = sizeof(filter->section) - filter->section_last
                    < payload_size
                    ? sizeof(filter->section) - filter->section_last
                    : payload_size - skip;

                  printf("length: %d\n", length);

                  /* assert(length == payload_size-skip); */

                  if(length)
                  {
                    int padding_last = length
                      , beggining = sync+4+adaptation_total_length+skip;
                    while(padding_last != 0
                          && (unsigned char)buffer[padding_last+beggining-1] == 0xFF)
                      --padding_last;
                    length = padding_last;
                  }

                  memcpy(&filter->section[filter->section_last]
                         , &packet_first[4+adaptation_total_length+skip]
                         , length);

                  int old_last = filter->section_last;

                  filter->section_last += length;
/*                   filter->continuity_counter = buffer[sync+3] & 0xF; */

/*                   /\* printf("new last: %d\n", (int)length); *\/ */
/*                   /\* printf("new continuity counter: %d\n", (int)filter->continuity_counter); *\/ */

                }
                else
                {
                  /* printf("skip > payload_size skip: %d payload_size: %d\n" */
                  /*        , (int)skip, (int)payload_size); */
                  filter->section_last = 0;
                }
              }
            }
            else
            {
              printf("======== Discontinuity\n");
              filter->packets = 0;
              filter->section_last = 0;
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

