/* (c) Copyright 2012 Felipe Magno de Almeida
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef MIDDLEWARE_API_PROPOSAL_DSMCC_FS_H
#define MIDDLEWARE_API_PROPOSAL_DSMCC_FS_H

struct middleware_api_dsmcc_fs_file;

middleware_api_dsmcc_fs_file* middleware_api_dsmcc_fs_open(const char* name);

size_t middleware_api_dsmcc_fs_size(middleware_api_dsmcc_fs_file* f);
size_t middleware_api_dsmcc_fs_read(middleware_api_dsmcc_fs_file* f, size_t offset
                                    , char* buffer, size_t size);

#endif
