/* (c) Copyright 2012 Felipe Magno de Almeida
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef MIDDLEWARE_API_PROPOSAL_SECTIONS_H
#define MIDDLEWARE_API_PROPOSAL_SECTIONS_H

#define MIDDLEWARE_API_PROPOSAL_SECTIONS

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*middleware_api_sections_filter_callback_type)
  (const char* buffer, size_t size, void* state);

struct middleware_api_sections_filter;

middleware_api_sections_filter* middleware_api_sections_create_filter_for_pid
(uint16_t pid, middleware_api_sections_filter_callback_type callback);

middleware_api_sections_filter* middleware_api_sections_create_filter_for_pid_and_table_id 
(uint16_t pid, uint_t table_id, middleware_api_sections_filter_callback_type callback);

void middleware_api_sections_remove_filter(middleware_api_sections_filter* p);

#ifdef __cplusplus
}
#endif

#endif
