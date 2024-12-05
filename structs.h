typedef struct {
  char string[64];
  void (*function)(int);
  uint8_t *icon;
  uint8_t type;
  int data;
} menuEntry;

typedef struct {
  char name[32];
  menuEntry entries[4];
  int selection;
  int cursor;
  uint16_t parent;
  int entries_count;
} menu;

uint8_t *icons[] = {&menu_submenu[0], &menu_escape[0]};
uint8_t *icons_type[] = {&menu_submenu[0], &menu_escape[0], &checkbox_0[0], &checkbox_1[0]};
