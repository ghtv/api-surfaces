/* (c) Copyright 2012 Felipe Magno de Almeida
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef PROCESS_DEMULTIPLEX_H
#define PROCESS_DEMULTIPLEX_H

typedef void(*process_demultiplex_callback)(int, void*);

void process_demultiplex_initialize();
void process_demultiplex_add_fd(int fd, process_demultiplex_callback, void* state);
void process_demultiplex_rm_fd(int fd);
void process_demultiplex_events();

#endif

