#ifndef DEBUG_PANEL_H
#define DEBUG_PANEL_H

#include <stdint.h>

void debug_panel_init(int pmm_status, int vmm_status);
void debug_panel_render(void);

#endif
