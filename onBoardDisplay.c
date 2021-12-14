/*
 * onBoardDisplay.c
 *
 *  Created on: 19-Nov-2021
 *      Author: Jenish Rudani
 */

#include "onBoardDisplay.h"

static GLIB_Context_t glibContext;
static int currentLine = 0;

void memlcd_app_init(void)
{
  uint32_t status;

  // Enable the memory lcd
  status = sl_board_enable_display();
  EFM_ASSERT(status == SL_STATUS_OK);

  // Initialize the DMD support for memory lcd display
  status = DMD_init(0);
  EFM_ASSERT(status == DMD_OK);

  // Initialize the glib context
  status = GLIB_contextInit(&glibContext);
  EFM_ASSERT(status == GLIB_OK);

  glibContext.backgroundColor = White;
  glibContext.foregroundColor = Black;

  // Fill lcd with background color
  GLIB_clear(&glibContext);
  GLIB_setFont(&glibContext, (GLIB_Font_t *) &GLIB_FontNormal8x8);

  /* Draw text on the memory lcd display*/
  GLIB_drawStringOnLine(&glibContext,
                        "LF Decoding ...",
                        currentLine++,
                        GLIB_ALIGN_CENTER,
                        5,
                        5,
                        false);
  currentLine = 0;
  DMD_updateDisplay();
}

__attribute__((section(".ram")))
void printOnDisplay(uint16_t data){

  char str[20];
  sprintf(str, "TE ID: 0x%04x", data);

  GLIB_clear(&glibContext);
  GLIB_setFont(&glibContext, (GLIB_Font_t *) &GLIB_FontNarrow6x8);

  GLIB_drawStringOnLine(&glibContext,
                          str,
                          currentLine++,
                          GLIB_ALIGN_CENTER,
                          5,
                          5,
                          false);
  DMD_updateDisplay();
  if(currentLine > 9) currentLine = 0;

}
