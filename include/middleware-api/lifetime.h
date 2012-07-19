/* (c) Copyright 2012 Felipe Magno de Almeida
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef MIDDLEWARE_API_PROPOSAL_LIFETIME_H
#define MIDDLEWARE_API_PROPOSAL_LIFETIME_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void middleware_api_lifetime_start(unsigned int service_id
                                   , const char* pat_section, size_t pat_size
                                   , const char* pmt_section, size_t pmt_size);
void middleware_api_lifetime_stop();

#ifdef __cplusplus
}
#endif

#endif
