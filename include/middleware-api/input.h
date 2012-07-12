// (c) Copyright 2012 Felipe Magno de Almeida
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MIDDLEWARE_API_PROPOSAL_INPUT_H
#define MIDDLEWARE_API_PROPOSAL_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

enum keys
{
  middleware_api_input_red_key
  , middleware_api_input_green_key
  , middleware_api_input_yellow_key
  , middleware_api_input_blue_key
  , middleware_api_input_a_key
  , middleware_api_input_b_key
  , middleware_api_input_c_key
  , middleware_api_input_d_key
  , middleware_api_input_e_key
  , middleware_api_input_f_key
  , middleware_api_input_g_key
  , middleware_api_input_h_key
  , middleware_api_input_cursor_left_key
  , middleware_api_input_cursor_up_key
  , middleware_api_input_cursor_right_key
  , middleware_api_input_cursor_down_key
  , middleware_api_input_enter_key
  , middleware_api_input_info_key
};

void middleware_api_input_remote_control_key (keys key);

#ifdef __cplusplus
}
#endif

#endif
