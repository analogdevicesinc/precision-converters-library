/*!
 *****************************************************************************
  @file:  adi_console_menu.c

  @brief: A simple console menu manager handler

  @details: A way to define using arrays of structs a set of menus that can
            be displayed to a user, easily, with all user interaction handled
            by the library, leaving only the implementation of the menu actions
            to be done by the library user.
 -----------------------------------------------------------------------------
 Copyright (c) 2019-2022 Analog Devices, Inc.
 All rights reserved.

 This software is proprietary to Analog Devices, Inc. and its licensors.
 By using this software you agree to the terms of the associated
 Analog Devices Software License Agreement.

*****************************************************************************/

/****************************************************************************/
/***************************** Include Files ********************************/
/****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "adi_console_menu.h"

/****************************************************************************/
/********************** Macros and Constants Definition *********************/
/****************************************************************************/
#define DIV_STRING "\t=================================================="

/******************************************************************************/
/*************************** Types Declarations *******************************/
/******************************************************************************/
// Save the state of console menu library
console_menu_state adi_console_menu_state = {
	.last_error_code = 0
};

/****************************************************************************/
/***************************** Function Definitions *************************/
/****************************************************************************/
/*!
 * @brief      displays the text of a console menu
 *
 * @details
 */
static void adi_display_console_menu(const console_menu * menu)
{
	adi_clear_console();

	// call headerItem to allow display of other content
	if (menu->headerItem != NULL) {
		menu->headerItem();
		printf(DIV_STRING EOL);
	}

	/*
	 * Display the menu title and  menuItems
	 * The shortcutKey is used to display '[A]' before the dispayText
	 */
	printf("\t%s" EOL "\t", menu->title);
	// show an underline to distinguish title from item
	for (uint8_t i = 0; i < strlen(menu->title); i++) {
		putchar('-');
	}
	// Extend underline past end of string, and then new line
	printf("--" EOL);

	// If the shortcutKey is not unique, first found is used
	for (uint8_t i = 0; i < menu->itemCount; i ++) {
		if (menu->items[i].shortcutKey == '\00') {
			// No shortcut key defined, but display item text if available
			printf("\t%s" EOL, menu->items[i].text);
		} else {
			printf("\t[%c] %s" EOL, toupper(menu->items[i].shortcutKey),
			       menu->items[i].text);
		}
	}
	if (menu->enableEscapeKey) {
		printf(EOL "\t[ESC] Exit Menu" EOL);
	}

	printf(EOL "\tPlease make a selection." EOL);

	// call footerItem to allow display of other content
	if (menu->footerItem != NULL) {
		printf(DIV_STRING EOL);
		menu->footerItem();
	}
}

/*!
 * @brief      Display a consoleMenu and handle User interaction
 *
 * @details    This displays the menuItems defined by the console menu, and
 *             handles all user interaction for the menu.
 *
 * @note       The function will return either the item selected or error code
 *             from the last action. One at a time. Either define the menu action
 *             or sub menu. If both are defined the function would return error.
 *             If both menu action and sub menu are defined NULL, then the
 *             function will return the item selected
 */
int32_t adi_do_console_menu(const console_menu * menu)
{
	int32_t itemSelected = MENU_ESCAPED;
	bool enableKeyScan = true;
	int32_t ret = MENU_DONE;

	adi_display_console_menu(menu);

	/*
	 *  Loop waiting for valid user input. menuItem index is returned if
	 *  user presses a valid menu option.
	 */
	do {
		char keyPressed = toupper(getchar());

		if (menu->enableEscapeKey) {
			if (keyPressed == ESCAPE_KEY_CODE) {
				itemSelected = MENU_ESCAPED;
				enableKeyScan = false;
				break;
			}
		}

		for (uint8_t i = 0; i < menu->itemCount; i ++) {
			if (toupper(menu->items[i].shortcutKey) == keyPressed) {
				itemSelected = i;

				// If the menuAction function pointer is NULL and
				// the sub console menu pointer is not NULL,
				// call the sub console menu.
				if (menu->items[i].action == NULL
				    && menu->items[i].submenu != NULL) {
					ret = adi_do_console_menu(menu->items[i].submenu);
				}
				// If the menuAction function pointer is not NULL and sub console menu
				// pointer is NULL, call the action.
				else if (menu->items[i].action != NULL && menu->items[i].submenu == NULL) {
					ret = menu->items[i].action(menu->items[i].id);
				}
				// If both are NULL, then return Not Supported action.
				else if (menu->items[i].action != NULL && menu->items[i].submenu != NULL) {
					ret = -1;
				}

				// Store the return value if it is negative.
				if (ret < 0) {
					adi_console_menu_state.last_error_code = ret;
					ret = MENU_CONTINUE;
				}

				switch (ret) {
				case MENU_DONE : {
					enableKeyScan = false;
					break;
				}
				case MENU_CONTINUE:
				default: {
					enableKeyScan = true;
					adi_display_console_menu(menu);
					break;
					}
				}

				break;
			}
		}

		// If both the action and sub menu are defined NULL, and the item selected
		// is not MENU_ESCAPED, then break the loop to return selected item.
		if(itemSelected != MENU_ESCAPED &&
		    menu->items[itemSelected].action == NULL &&
		    menu->items[itemSelected].submenu == NULL) {
			break;
		}

	} while (enableKeyScan);

	return (itemSelected);
}

/*!
 * @brief      Reads a decimal string from the user
 *
 * @param      input_len max number of character to accept from the user
 *
 * @return      The integer value entered
 *
 * @details    Allows a user to type in number, echoing back to the user,
 *             up to input_len chars
 *
 *  @note      Only positive integer numbers are supported currently
 */
int32_t adi_get_decimal_int(uint8_t input_len)
{
	char buf[20] = {0};
	uint8_t buf_index = 0;
	char ch;
	bool loop = true;

	assert(input_len < 19);

	do  {
		ch = getchar();
		if (isdigit(ch) && buf_index < (input_len)) {
			//  echo and store it as buf not full
			buf[buf_index++] = ch;
			putchar(ch);
		}
		if ((ch == '\x7F') && (buf_index > 0)) {
			//backspace and at least 1 char in buffer
			buf[buf_index--] = '\x00';
			putchar(ch);
		}
		if ((ch == '\x0D') || (ch == '\x0A')) {
			// return key pressed, all done, null terminate string
			buf[buf_index] = '\x00';
			loop = false;
		}
	} while(loop);

	return atoi(buf);
}

/*!
 * @brief      Reads a hexadecimal number from the user
 *
 * @param      input_len max number of character to accept from the user
 *
 * @return     The integer value entered
 *
* @details     Allows a user to type in a hexnumber, echoing back to the user,
 *             up to input_len chars
 */
uint32_t adi_get_hex_integer(uint8_t input_len)
{
	char buf[9] = {0};
	uint8_t buf_index = 0;
	char ch;
	bool loop = true;

	assert(input_len < 8);

	do  {
		ch = getchar();
		if (isxdigit(ch) && buf_index < (input_len)) {
			//  echo and store it as buf not full
			buf[buf_index++] = ch;
			putchar(ch);
		}
		if ((ch == '\x7F') && (buf_index > 0)) {
			//backspace and at least 1 char in buffer
			buf[buf_index--] = '\x00';
			putchar(ch);
		}
		if ((ch == '\x0D') || (ch == '\x0A')) {
			// return key pressed, all done, null terminate string
			buf[buf_index] = '\x00';
			loop = false;
		}
	} while(loop);

	return strtol(buf, NULL, 16);
}

/*!
 * @brief      Reads a floating string from the user
 *
 * @param      input_len max number of character to accept from the user
 *
 * @return      The float value entered
 *
 * @details    Allows a user to type in number, echoing back to the user,
 *             up to input_len chars
 *
 *  @note      Only positive floating point numbers are supported currently
 */
float adi_get_decimal_float(uint8_t input_len)
{
	char buf[20] = { 0 };
	uint8_t buf_index = 0;
	char ch;
	bool loop = true;

	assert(input_len < 19);

	do {
		ch = getchar();
		if ((isdigit(ch) || (ch == '.')) && buf_index < (input_len)) {
			//  echo and store it as buf not full
			buf[buf_index++] = ch;
			putchar(ch);
		}
		if ((ch == '\x7F') && (buf_index > 0)) {
			//backspace and at least 1 char in buffer
			buf[buf_index--] = '\x00';
			putchar(ch);
		}
		if ((ch == '\x0D') || (ch == '\x0A')) {
			// return key pressed, all done, null terminate string
			buf[buf_index] = '\x00';
			loop = false;
		}
	} while (loop);

	return atof(buf);
}

/**
 * @brief 	Handles the integer type input from the user by displaying
 *          the menu message and provides a set number of
 *          input attempts for the user.
 * @param   menu_prompt[in] - User specified prompt.
 * @param   min_val[in] - minimum input value.
 * @param   max_val[in] - maximum input value.
 * @param   input_val[in, out] - User provided input value.
 * @param   input_len[in] - User provided input length.
 * @param   max_attempts[in] - Maximum number of input attempts.
 * @param   clear_lines[in] - lines to clear in case of
                              invalid input.
 * @return	0 in case of success. -1 otherwise.
 */
int32_t adi_handle_user_input_integer(const char* menu_prompt,
			      uint16_t min_val,
			      uint16_t max_val,
			      uint16_t *input_val,
				  uint8_t input_len,
			      uint8_t max_attempts,
			      uint8_t clear_lines)
{
	uint8_t count = 0;

	if (menu_prompt == NULL || input_val == NULL) {
		return -1;
	}

	do {
		/* Gets the input from the user and allows
		 * reattempts in-case of incorrect input */
		printf("%s (%d - %d): ", menu_prompt, min_val, max_val);
		*input_val = (uint16_t)adi_get_decimal_int(input_len);

		if ((*input_val >= min_val) && (*input_val <= max_val)) {
			// break out of the loop in-case of a correct input
			break;
		} else {
			if (count == max_attempts) {
				printf(EOL "Maximum try limit exceeded" EOL);
				adi_press_any_key_to_continue();
				return -1;
			}

			printf(EOL "Please enter a valid selection" EOL);
			adi_press_any_key_to_continue();
			/* Moves up the cursor by specified lines and
			 * clears the lines below it */
			for (uint8_t i = 0; i < clear_lines; i++) {
				printf(VT100_CLEAR_CURRENT_LINE);
				printf(VT100_MOVE_UP_N_LINES, 1);
			}
		}
	} while (++count <= max_attempts);

	return 0;
}

/**
 * @brief 	Handles the float type input from the user by displaying
 *          the menu message and provides a set number of
 *          input attempts for the user.
 * @param   menu_prompt[in] - User specified prompt.
 * @param   min_val[in] - minimum input value.
 * @param   max_val[in] - maximum input value.
 * @param   input_val[in, out] - User provided input value.
 * @param   input_len[in] - User provided input length.
 * @param   max_attempts[in] - Maximum number of input attempts.
 * @param   clear_lines[in] - lines to clear in case of
                              invalid input.
 * @return	0 in case of success. -1 otherwise.
 */
int32_t adi_handle_user_input_float(const char* menu_prompt,
	float min_val,
	float max_val,
	float *input_val,
	uint8_t input_len,
	uint8_t max_attempts,
	uint8_t clear_lines)
{
	uint8_t count = 0;

	if (menu_prompt == NULL || input_val == NULL) {
		return -1;
	}

	do {
		/* Gets the input from the user and allows
		 * reattempts in-case of incorrect input */
		printf("%s (%0.3f - %0.3f): ", menu_prompt, min_val, max_val);
		*input_val = adi_get_decimal_float(input_len);

		if ((*input_val >= min_val) && (*input_val <= max_val)) {
			// break out of the loop in-case of a correct input
			break;
		}
		else {
			if (count == max_attempts) {
				printf(EOL "Maximum try limit exceeded" EOL);
				adi_press_any_key_to_continue();
				return -1;
			}

			printf(EOL "Please enter a valid selection" EOL);
			adi_press_any_key_to_continue();
			/* Moves up the cursor by specified lines and
			 * clears the lines below it */
			for (uint8_t i = 0; i < clear_lines; i++) {
				printf(VT100_CLEAR_CURRENT_LINE);
				printf(VT100_MOVE_UP_N_LINES, 1);
			}
		}
	} while (++count <= max_attempts);

	return 0;
}

/*!
 * @brief      Clears the console terminal
 *
 * @details    Clears the console terminal using VT100 escape code, or can be changed to
 *             output blank lines if serial link doesn't support VT100.
 */
void adi_clear_console(void)
{
	/*
	 * clear console and move cursor to home location, followed by move to home location.
	 *  Dedicated call to move home is because sometimes first move home doesn't work
	 *  \r\n required to flush the uart buffer.
	 */
	printf(VT100_CLEAR_CONSOLE VT100_MOVE_TO_HOME EOL);

   /*
	* if VT100 is not supported, this can be enabled instead, but menu display may not work well
	*/
//    for (uint8_t = 0; i < 100; i++)
//      printf("\r\n\r");
}

/*!
 * @brief  Clears the error code from the last menu
 *
 * @details
 */
void adi_clear_last_menu_error(void)
{
	adi_console_menu_state.last_error_code = 0;
}

/*!
 * @brief  Returns the error code from the last menu
 *
 * @return The error code value
 */
int32_t adi_get_last_menu_error(void)
{
	return adi_console_menu_state.last_error_code;
}

/*!
 * @brief      waits for any key to be pressed, and displays a prompt to the user
 *
 * @details
 */
void adi_press_any_key_to_continue(void)
{
	printf("\r\nPress any key to continue...\r\n");
	getchar();
}
