/* (c) Copyright 2012 Felipe Magno de Almeida
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef MIDDLEWARE_API_PROPOSAL_DSMCC_FS_H
#define MIDDLEWARE_API_PROPOSAL_DSMCC_FS_H

#include <middleware-api/declaration.h>

#ifdef __cplusplus
extern "C" {
#endif

struct middleware_api_dsmcc_fs_file;

MIDDLEWARE_API_MIDDLEWARE_DECL middleware_api_dsmcc_fs_file* middleware_api_dsmcc_fs_open
 (const char* name);

MIDDLEWARE_API_MIDDLEWARE_DECL size_t middleware_api_dsmcc_fs_size
 (middleware_api_dsmcc_fs_file* f);
MIDDLEWARE_API_MIDDLEWARE_DECL size_t middleware_api_dsmcc_fs_read
 (middleware_api_dsmcc_fs_file* f, size_t offset
  , char* buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif
