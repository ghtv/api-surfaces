/* (c) Copyright 2012 Felipe Magno de Almeida
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#include <middleware-api/input.h>

#include <directfb.h>

#include <stdio.h>

static IDirectFBInputDevice* keyboard;
static IDirectFBEventBuffer* keyboard_buffer;
extern IDirectFB* directfb;

typedef enum middleware_api_input_keys middleware_api_input_keys;

void middleware_api_input_initialize()
{
  DFBInputDeviceID device = DIDID_KEYBOARD;
  directfb->GetInputDevice (directfb, device, &keyboard);
  keyboard->CreateEventBuffer (keyboard, &keyboard_buffer);
}

void middleware_api_input_wait_event()
{
  do
  {
    DFBResult r = keyboard_buffer->WaitForEvent (keyboard_buffer);
    if(r == DFB_OK)
    {
      DFBInputEvent event;
      if ((r = keyboard_buffer->GetEvent (keyboard_buffer, DFB_EVENT(&event))) == DFB_OK)
      {
        middleware_api_input_keys key = middleware_api_input_invalid_key;
        switch(event.key_symbol)
        {
        case DIKS_CURSOR_LEFT:
          key = middleware_api_input_cursor_left_key;
          break;
        case DIKS_CURSOR_RIGHT:
          key = middleware_api_input_cursor_right_key;
          break;
        case DIKS_CURSOR_UP:
          key = middleware_api_input_cursor_up_key;
          break;
        case DIKS_CURSOR_DOWN:
          key = middleware_api_input_cursor_down_key;
          break;
        case DIKS_F1:
          key = middleware_api_input_red_key;
          break;
        case DIKS_F2:
          key = middleware_api_input_blue_key;
          break;
        case DIKS_F3:
          key = middleware_api_input_green_key;
          break;
        case DIKS_F4:
          key = middleware_api_input_yellow_key;
          break;
        case DIKS_ENTER:
          key = middleware_api_input_enter_key;
          break;
        }
        if(key != middleware_api_input_invalid_key)
          middleware_api_input_remote_control_key (key);
      }
    }
  }
  while(true);
}

