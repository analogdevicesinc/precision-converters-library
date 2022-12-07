/*!
 *****************************************************************************
  @file:  adi_console_menu.h

  @brief:   A simple console menu manager handler

  @details:
 -----------------------------------------------------------------------------
 Copyright (c) 2019-2022 Analog Devices, Inc.
 All rights reserved.

 This software is proprietary to Analog Devices, Inc. and its licensors.
 By using this software you agree to the terms of the associated
 Analog Devices Software License Agreement.

*****************************************************************************/

#ifndef ADI_CONSOLE_MENU_H_
#define ADI_CONSOLE_MENU_H_

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

/******************************************************************************/
/********************** Macros and Constants Definition ***********************/
/******************************************************************************/

#define MENU_ESCAPED          INT_MAX
#define MENU_CONTINUE         INT_MAX-1
#define MENU_DONE             INT_MAX-2

#define ESCAPE_KEY_CODE         (char)0x1B

#define EOL "\r\n"

/* ANSI VT100 escape sequence codes */
#define VT100_MOVE_UP_1_LINE		"\033[A"
#define VT100_MOVE_UP_N_LINES		"\x1B[%dA"
#define VT100_CLEAR_CURRENT_LINE	"\x1B[J"
#define VT100_CLEAR_CONSOLE         "\x1B[2J"
#define VT100_MOVE_TO_HOME          "\x1B[H"
#define VT100_COLORED_TEXT          "\x1B[%dm"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) ((sizeof (x)) / (sizeof ((x)[0])))
#endif

/*  ANSI VT100 color codes */
enum vt100_colors {
	VT_FG_DEFAULT = 0,
	VT_FG_RED = 31,
	VT_FG_GREEN = 32,
	VT_FG_YELLOW = 33,
	VT_FG_BLUE = 34,
	VT_FG_MAGENTA = 35,
	VT_FG_CYAN = 36,
	VT_FG_WHITE = 37
};

/******************************************************************************/
/********************** Variables and User Defined Data Types *****************/
/******************************************************************************/
/* This define the state of the console menu library*/
typedef struct {
	// Stores the error code from the last menu action.
	int32_t last_error_code;
} console_menu_state;

/* Type Definitions */
// Each menu item is defined by this struct
typedef struct {
	// String displayed for menu item
	char * text;
	// character that can be pressed to select menu item
	char  shortcutKey;
	// Function to be called when menu item is selected, if NULL, no function is called
	int32_t (*action)(uint32_t option);
	// Submenu to be called when menu item is selected, if NULL, no sub menu is displayed
	struct console_menu *submenu;
	// id value passed as the option value when calling menuAction
	uint32_t id;
} console_menu_item;

// This defines a complete menu with items
typedef struct {
	// String to be displayed as the menu title
	char * title;
	// Array of all the menu items
	console_menu_item * items;
	// Number of menuItems
	uint8_t itemCount;
	// Function alled before Menu title is displayed if defined
	void (*headerItem)(void);
	// Function called after menu items are displayed if defined
	void (*footerItem)(void);
	// Should the escape key to exit the menu be enabled?
	bool enableEscapeKey;
} console_menu;

/******************************************************************************/
/*****************************  Public Declarations ***************************/
/******************************************************************************/
int32_t adi_do_console_menu(const console_menu * menu);
int32_t adi_get_decimal_int(uint8_t input_len);
uint32_t adi_get_hex_integer(uint8_t input_len);
float adi_get_decimal_float(uint8_t input_len);
int32_t adi_handle_user_input_integer(const char* menu_prompt,
	uint16_t min_val,
	uint16_t max_val,
	uint16_t *input_val,
	uint8_t input_len,
	uint8_t max_attempts,
	uint8_t clear_lines);
int32_t adi_handle_user_input_float(const char* menu_prompt,
	float min_val,
	float max_val,
	float *input_val,
	uint8_t input_len,
	uint8_t max_attempts,
	uint8_t clear_lines);
void adi_clear_console(void);
void adi_clear_last_menu_error(void);
int32_t adi_get_last_menu_error(void);
void adi_press_any_key_to_continue(void);

extern console_menu_state adi_console_menu_state;

#endif /* ADI_CONSOLE_MENU_H_ */
