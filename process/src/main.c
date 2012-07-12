/* (c) Copyright 2012 Felipe Magno de Almeida
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

void middleware_api_graphics_initialize(int argc, char** argv);
void middleware_api_input_initialize();
void middleware_api_input_wait_event();

int main(int argc, char** argv)
{
  middleware_api_graphics_initialize(argc, argv);
  middleware_api_input_initialize();

  middleware_api_input_wait_event();
  
  return 0;
}
