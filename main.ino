#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

#include "icons.h"
#include "structs.h"

/*
// To-do: 
Add basic support for ssd1351 (UI will still be monochrome)
Migrate to PlatformIO
Add functionality to toggle IO interface access
Add function to destroy menus and entries
*/

// Hardware-specific code

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 23, /* data=*/ 5, /* reset=*/ U8X8_PIN_NONE);   // All Boards without Reset of the Display

menu *menu_1;
menu menus[64];

void drawOverlay(); // Function table
void drawMenu();
void switchMenu();

void draw() { // Renderer wrapper
  drawOverlay();
  drawMenu();
}

uint8_t getButtons() {
  if (digitalRead(26) == HIGH) // Button 1
    return 1;
  else if (digitalRead(18) == HIGH) // Button 2
    return 2;
  else if (digitalRead(19) == HIGH) // Button 3
    return 3;

  return 0; // Return 0 if no buttons are pressed
}

void menuEnter() {
  int i;
  uint16_t menuid;
  menuEntry tmp;

  for (i=0; i<menu_1->entries_count; i++) {
    tmp = menu_1->entries[i]; // Temp var for optimization
    
    if (menu_1->selection == i) {
      if (tmp.type == 4) { // If checkbox is checked
        tmp.data = 0; // Reset data value and
        tmp.type = 3; // Uncheck checkbox
      } else if (tmp.type == 3) { // If checkbox is unchecked
        tmp.data = 1; // Set data value to 1 and
        tmp.type = 4; // Check checkbox
      }
      menu_1->entries[i] = tmp; // Preprocess entry data depending on type of entry

      if (tmp.function != NULL) // Safety measure to prevent null pointer exception
        tmp.function(tmp.data);

      if (tmp.type == 1) {
        menuid = tmp.data;
        switchMenu(menuid);
        return;
      } else if (tmp.type == 2 && menu_1->parent != NULL) {
        menuid = menu_1->parent;
        switchMenu(menuid);
        return;
      }

      break; // Break to stop CPU cycles from being wasted
    } 
  }
}

void menuSelect() {
  uint8_t input;
  input = getButtons(); // Store input state into variable

  if (input == 1) {
    if (0 < menu_1->selection) // Check if selection can be moved up
      menu_1->selection--; // Move the selection up
    else {
      menu_1->selection = menu_1->entries_count - 1; // Loop back to last entry of menu
      menu_1->cursor = menu_1->entries_count - 3;
    }

    if (menu_1->selection < menu_1->cursor
        && 0 <= menu_1->cursor) // Check if cursor can be moved up
      menu_1->cursor--; // Move the cursor up
  }

  if (input == 3) {
    if (menu_1->selection + 1 < menu_1->entries_count) // Check if selection can be moved down
      menu_1->selection++; // Move the selection down
    else {
      menu_1->selection = 0; // Loop back to first entry of menu
      menu_1->cursor = 0;
    }

    if (menu_1->cursor+2 < menu_1->selection
        && menu_1->cursor+2 < menu_1->entries_count) // Check if cursor can be moved down
      menu_1->cursor++; // Move the cursor down
  }
}

void handleInput() {
  uint8_t input;
  input = getButtons(); // Store input state into variable

  if (input == 2) { // If middle button is clicked,
    menuEnter(); // call execution handler
  } else if (input > 0) { // If other buttons are clicked,
    menuSelect(); // call selection handler
  }
}

void drawByte(uint16_t x, uint16_t y, uint8_t byte) {
  if ((byte & 0x80) == 0x80) u8g2.drawPixel(x, y);   // Draw most significant bit
  if ((byte & 0x40) == 0x40) u8g2.drawPixel(x+1, y);
  if ((byte & 0x20) == 0x20) u8g2.drawPixel(x+2, y);
  if ((byte & 0x10) == 0x10) u8g2.drawPixel(x+3, y);
  if ((byte & 0x08) == 0x08) u8g2.drawPixel(x+4, y);
  if ((byte & 0x04) == 0x04) u8g2.drawPixel(x+5, y);
  if ((byte & 0x02) == 0x02) u8g2.drawPixel(x+6, y);
  if ((byte & 0x01) == 0x01) u8g2.drawPixel(x+7, y); // Draw least significant bit
}

void drawIcon(uint16_t x, uint16_t y, uint8_t *icon) {
  int i;

  for (i=0; i<8; i++)
    drawByte(x, y+i, *(icon+i)); // Print 8 bytes horizontally
}

void drawMenu() {
  int i, xoff, cursor = 0; // var init
  cursor = menu_1->cursor; // Load the cursor value

  menuEntry a;

  for (i=0; i<3; i++) {
    a = menu_1->entries[i+cursor]; // Temporary variable for optimization

    if (menu_1->selection == i+cursor) {
      u8g2.drawBox(0, 17+i*17, 128, 16); // Menu entry with white background and black text
      u8g2.setDrawColor(0);
    } else {
      u8g2.setDrawColor(1); // Menu entry with white outline and text
      u8g2.drawHLine(0, 34+i*17, 128);
    }

    xoff = 10; // Set X offset first
    if (a.icon > 0) // If icon pointer has valid value,
      drawIcon(3, 21+i*17, a.icon); // draw the icon
    else if (a.type > 0)   // If type is set,
      drawIcon(3, 21+i*17, icons_type[a.type - 1]); // select and draw appropriate icon
    else
      xoff = 0; // Otherwise, reset x offset

    u8g2.setCursor(3+xoff, 20+i*17); // Set cursor
    u8g2.printf(a.string); // Print the text
  }
  u8g2.setDrawColor(1); // Reset drawing color to prevent visual glitches
}

void switchMenu(uint16_t menuidx) {
  menu_1 = &(menus[menuidx]); // Replace menu access pointer with menuptr
}

void updateMenu(int idx, char name[32], int selection, int entries_count, uint16_t parent) {
  menu a = menus[idx]; // Load menu id idx

  if (name != NULL)
    strcpy(a.name, name); // If name is not null, replace name with [name]

  if (selection != NULL)
    a.selection = selection; // Same as above

  if (entries_count != NULL)
    a.entries_count = entries_count; // Same as above

  if (parent != NULL)
    a.parent = parent; // Same as above

  menus[idx] = a; // Write the value back
}

void updateMenuEntry(int idx, int entryidx, char string[64], void (*function)(int), uint8_t *icon, uint8_t type, int data) {
  menu a = menus[idx];
  menuEntry b = a.entries[entryidx]; // Create temp var to update values

  if (string != NULL)
    strcpy(b.string, string); // If string is not null, set b.string to string

  if (function != NULL)
    b.function = function; // Same as above

  if (icon != NULL)
    b.icon = icon; // Same as above

  if (type != NULL)
    b.type = type; // Same as above

  if (data != NULL)
    b.data = data; // Same as above

  if (entryidx > a.entries_count - 1) {
    menus[idx].entries_count++;
    menus[idx].entries[a.entries_count] = b;
  } else {
    menus[idx].entries[entryidx] = b; // Write updated value to memory
  }
}

void dummy3(int data) {
  Serial.printf("Menu2, %d\n", data);
}

void dummy4(int data) {
  Serial.print("Menu3\n");
}

void initialize(void) {
  // Init output devices
  
  Serial.begin(115200);

  u8g2.begin(); // Init display
  u8g2.setFont(u8g2_font_6x10_tf); // Init font
  u8g2.setFontMode(1); // Init font mode
  u8g2.setFontRefHeightExtendedText(); 
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0); // Load display settings

  // Init input devices

  pinMode(26, OUTPUT);
  pinMode(18, OUTPUT);
  pinMode(19, OUTPUT); // Init buttons
}

void drawOverlay() {
  u8g2.drawBox(0, 16, 128, 2); // Draw the separator line
  u8g2.setCursor(3, 2);
  u8g2.printf(menu_1->name); // Print the title of menu
}

void setup(void) {
  initialize();

  updateMenu(0, "Hello, World!", 0, 5, NULL);
  updateMenuEntry(0, 0, "test1", NULL, NULL, 1, 1);
  updateMenuEntry(0, 1, "test2", &dummy3, NULL, 0, 1);
  updateMenuEntry(0, 2, "test4", &dummy3, NULL, 3, 2);
  updateMenuEntry(0, 3, "test5", &dummy3, NULL, 0, 3);
  updateMenuEntry(0, 4, "test6", &dummy3, NULL, 0, 4);
  Serial.printf("");

  updateMenu(1, "Howdy, World!", 0, 2, 0);
  updateMenuEntry(1, 0, "Back", NULL, NULL, 2, 0);
  updateMenuEntry(1, 1, "test3", &dummy4, NULL, 0, NULL);

  switchMenu(0);
}

void loop(void) {
  u8g2.firstPage();
  do {
    draw(); // Output handler
    handleInput(); // Input handler
  } while ( u8g2.nextPage() );
}
