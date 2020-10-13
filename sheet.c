/*
                          Simple table processor
                              Version: 1
Program to process tables from standard input and outputs it to standard output

                             Martin Douša
                             October 2020
*/
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CELL_LEN 100
#define MAX_LINE_LEN 10240
#define LINE_LENGTH_TEST_OFFSET 1
#define BLACKLISTED_DELIMS "\n\0\r"

const char *TABLE_EDIT_COMS[] = {"irow", "arow", "drow", "drows", "icol", "acol", "dcol", "dcols"};
#define NUMBER_OF_TABLE_EDIT_COMS 8
const char *DATA_EDIT_COMS[] = {"cset", "tolower", "toupper", "round", "int", "copy", "swap", "move"};
#define NUMBER_OF_DATA_EDIT_COMS 8
const char *AREA_SELECTOR_COMS[] = {"rows", "beginswith", "contains"};
#define NUMBER_OF_AREA_SELECTOR_COMS 3

enum Mode {PASS, TABLE_EDIT, DATA_EDIT};
enum ReturnCodes {SUCESS, MAX_LINE_LEN_EXCEDED, MAX_CELL_LEN_EXCEDED, ARG_ERROR, INPUT_ERROR};

struct line_struct
{
  char *line_string;
  char delim;
  int line_index;

  // Reference number of cols
  int num_of_cols;
  // Number of cols after editing
  int final_cols;

  int last_line_flag;
  int deleted;
  int process_flag;
};

struct selector_arguments
{
  int selector_type;
  char *a1, *a2, *str;
  int ai1, ai2;
};

void check_arguments(int argc, char *argv[])
{
  /*
  Check if lenght of every single argument is in limit and exit if max length is exceded

  params:
  :argc - length of argument array
  :argv - array of arguments
  */

  for (int i=1; i < argc; i++)
  {
    if (strlen(argv[i]) > MAX_CELL_LEN)
    {
      fprintf(stderr, "Argument %d exceded maximum allowed size! Maximum size is %d characters\n", i, MAX_CELL_LEN);
      exit(MAX_CELL_LEN_EXCEDED);
    }
  }
}

int are_strings_same(const char *s1, const char *s2)
{
  /*
  Check if two string are same

  params:
  :s1 - first string
  :s2 - second string
  
  :return - 1 if strings are same
          - 0 if strings are different
  */

  return strcmp(s1, s2) == 0;
}

int is_string_start_with(const char *base_string, const char *start_string)
{
  /*
  Check if string starts with other string

  params:
  :base_string - string where to look for substring
  :start_string - substring to look for at start of base_string

  :return - 1 if base_string starts with start_string, 0 if dont
  */

  if (strlen(start_string) > strlen(base_string))
    return 0;

  return strncmp(start_string, base_string, strlen(start_string)) == 0;
}

// command_selectors
/*
Selectors that will return index of command from arrays of commands

params:
:com - command string

:return - index of command in commands array if command is found
        - -1 if command not found
*/

int get_table_edit_com_index(char *com)
{
  for (int i=0; i < NUMBER_OF_TABLE_EDIT_COMS; i++)
  {
    if (are_strings_same(com, TABLE_EDIT_COMS[i]))
    {
      return i;
    }
  }
  return -1;
}

int get_data_edit_com_index(char *com)
{
  for (int i=0; i < NUMBER_OF_DATA_EDIT_COMS; i++)
  {
    if (are_strings_same(com, DATA_EDIT_COMS[i]))
    {
      return i;
    }
  }
  return -1;
}

int get_area_selector_com_index(char *com)
{
  for (int i=0; i < NUMBER_OF_AREA_SELECTOR_COMS; i++)
  {
    if (are_strings_same(com, AREA_SELECTOR_COMS[i]))
    {
      return i;
    }
  }
  return -1;
}
// command_selectors

int get_operating_mode(char *input_array[], int array_len)
{
  /*
  Check arguments to determinate operating mode of program

  params:
  :input_array - array of arguments
  :array_len - length of array with arguments

  :return - 0 if no valid argument found (pass thru mode)
          - 1 if first is table edit command
          - 2 if first is data edit command
  */

  for (int i=1; i < array_len; i++)
  {
    if (get_table_edit_com_index(input_array[i]) >= 0)
      return TABLE_EDIT;

    if (get_data_edit_com_index(input_array[i]) >= 0)
      return DATA_EDIT;

    if (get_area_selector_com_index(input_array[i]) >= 0)
      return DATA_EDIT;
  }
  
  return PASS;
}

void remove_newline_character(char *s) {
  /* 
  Function to remove new line characters
  Iterate over string until it new line character then replace it with 0

  params:
  :s - pointer to string (char array)
  */

  while(*s && *s != '\n' && *s != '\r')
    s++;
 
  *s = 0;
}

int check_delim_characters(char *delims)
{
  /*
  Check if in delims string isnt any blacklisted char

  params:
  :delims - string with delim characters

  :return 0 if all chars are right else -1
  */

  for (size_t i=0; i < strlen(delims); i++)
  {
    for (size_t j=0; j < strlen(BLACKLISTED_DELIMS); j++)
    {
      if (delims[i] == BLACKLISTED_DELIMS[j])
      {
        return -1;
      }
    }
  }

  return 0;
}

void get_delims(char *input_array[], int array_len, char *delim)
{
  /*
  Extract delims string from args

  params:
  :input_array - array of strings (args)
  :array_len - number of args
  :delim - return string
  */

  for (int i=1; i < array_len; i++)
  {
      if (are_strings_same(input_array[i], "-d"))
      {
        if ((i + 1) >= array_len)
        {
          fprintf(stderr, "Found delimiter flag without any value\n");
          exit(ARG_ERROR);
        }

        strcpy(delim, input_array[i + 1]);
        return;
      }
  }
}

void replace_unused_delims(char *string, char* delims)
{
  /*
  Iterate over string and replace delims that are not on 0 position in delims string with delim on 0 position

  params:
  :string - string where to replace delims
  :delims - string with delims
  */

  for (size_t i=0; i < strlen(string); i++)
  {
    for (size_t j=1; j < strlen(delims); j++)
    {
      // Ignore duplicates of first delim
      if (delims[j] == delims[0])
        continue;

      if (string[i] == delims[j])
      {
        string[i] = delims[0];
      }
    }
  }
}

int count_specific_chars(char *string, char ch)
{
  /*
  Count number of specific characters in string

  params:
  :string - input string where count chars
  :ch - char we want to count

  :return - number of chars we found
  */

  int delim_counter = 0;
  for (size_t i=0; i < strlen(string); i++)
  {
    if (string[i] == ch) delim_counter++;
  }

  return delim_counter;
}

int get_number_of_cells(char *string, char delim)
{
  /*
  Get number of cells in string
  Cell is substring separated by deliminator

  params:
  :string - input string with cells
  :delim - deliminator char
  */

  return count_specific_chars(string, delim) + 1;
}

int get_position_of_character(char *string, char ch, int index)
{
  /*
  Get position of character of certain index in string

  params:
  :string - string where to find delims
  :ch - character we are looking for
  :index - index of occurence of character in string

  :return - 0 if character position found else -1
  */

  if (index > (count_specific_chars(string, ch) - 1) || index < 0) return -1;

  int delim_counter = 0;
  for (size_t i=0; i < strlen(string); i++)
  {
    if (string[i] == ch)
    {
      delim_counter++;
      if ((delim_counter - 1) == index)
        return i;
    }
  }

  return -1;
}

int get_start_of_substring(struct line_struct *line, int index)
{
  /*
  Get start index of substring from line string limited by delim

  params:
  :line - structure with line data
  :index - index of substring in line

  :return - index of first char of substring if found
          - -1 on error
  */

  if (index < 0)
    return -1;

  if (index == 0)
  {
    // if we want first substring there is no delim before substring then we are starting from first char of string
    return 0;
  }
  else
  {
    // first character of substring after start delim
    return get_position_of_character(line->line_string, line->delim, index - 1) + 1;
  }
}

int get_end_of_substring(struct line_struct *line, int index)
{
  /*
  Get last index of substring from line string limited by delim

  params:
  :line - structure with line data
  :index - index of substring in line

  :return - index of last char of substring if found
          - -1 on error
  */

  int number_of_cells = get_number_of_cells(line->line_string, line->delim);

  if (index > (number_of_cells - 1))
    return -1;

  if (index == (number_of_cells - 1))
  {
    // if we are on the last cell we are going to the end of that line
    return strlen(line->line_string);
  }
  else
  {
    // last character of substring before delim
    return get_position_of_character(line->line_string, line->delim, index) - 1;
  }
}

int get_value_of_cell(struct line_struct *line, int index, char *substring)
{
  /*
  Extract value of cell

  params:
  :line - structure with line data
  :index - index of cell
  :substring - string for saving result
  :max_length - maximal length of one cell

  :return - 0 on success
          - -1 on error
  */

  int number_of_cells = get_number_of_cells(line->line_string, line->delim);

  if (index > (number_of_cells - 1) || index < 0)
    return -1;

  int start_index = get_start_of_substring(line, index);
  int end_index = get_end_of_substring(line, index);

  if (start_index < 0 || end_index < 0)
    return -1;

  // exit if length of substring is larger than maximum size of one cell
  if ((end_index - start_index) > MAX_CELL_LEN)
  {
    fprintf(stderr, "\nCell %d on line %d exceded max memory size! Max length of cell is %d characters (exclude delims)\n", index + 1, line->line_index + 1, MAX_CELL_LEN);
    exit(MAX_CELL_LEN_EXCEDED);
  }
  
  // iterate over whole substring (we are recycling one and then we want to clear it)
  for (int i=0; i < MAX_CELL_LEN; i++, start_index++)
  {
    if (start_index <= end_index)
    {
      substring[i] = line->line_string[start_index];
    }
    else
    {
      substring[i] = 0;
      break;
    }
  }

  return 0;
}

void check_line_length(struct line_struct *line)
{
  /*
  Check if line is no longer than maximum allowed length of one line

  params:
  :line - structure with line data
  */

  if (strlen(line->line_string) > MAX_LINE_LEN)
  {
    fprintf(stderr, "\nLine %d exceded max memory size! Max length of line is %d characters (including delims)\n", line->line_index+1, MAX_LINE_LEN);
    exit(MAX_LINE_LEN_EXCEDED);
  }
}

int string_to_double(char *string, double *val)
{
  /*
  Check if input string could be double and then convert it to double

  params:
  :string - string to convertion
  :val - output double value

  :return - 0 if conversion is success
          - -1 if whole string is not double
  */

  char *foo;
  (*val) = strtod(string, &foo);

  if (foo[0] != 0)
    return -1;

  return 0;
}

int is_string_int(char *string)
{
  /*
  Check if input string is intiger

  params:
  :string - string to convertion

  :return - 1 if is intiger
          - 0 if not
  */

  char *foo;
  strtol(string, &foo, 10);

  if (foo[0] != 0)
    return 0;

  return 1;
}

int string_to_int(char *string, int *val)
{
  /*
  Check if input string could be intiger and then convert it to intiger

  params:
  :string - string to convertion
  :val - output intiger value

  :return - 0 if conversion is success
          - -1 if whole string is not int
  */

  char *foo;
  (*val) = strtol(string, &foo, 10);

  if (foo[0] != 0)
    return -1;

  return 0;
}

int argument_to_int(char *input_array[], int array_len, int index)
{
  /*
  Try to convert argument to int

  params:
  :input_array - array with all arguments
  :array_len - lenght of array with arguments
  :index - index of argument we want convert to int
  
  :return - int value of argument
          - on error 0
  */

  int val;

  if (index > (array_len - 1))
    return 0;

  if (string_to_int(input_array[index], &val) != 0)
    return 0;

  if (val <= 0)
    return 0;

  return val;
}

void create_empty_row(struct line_struct *line)
{
  /*
  Create string with empty line format based on wanted line length and delim char
  Empty line is saved to line structure instead of current line string

  params:
  :line - structure with line data
  */

  if (line->num_of_cols <= 0)
  {
    line->line_string[0] = 0;
    return;
  }

  int i = 0;
  for (; i < line->num_of_cols; i++)
  {
    if (i < (line->num_of_cols - 1))
      line->line_string[i] = line->delim;
    else
    {
      line->line_string[i] = 0;
      break;
    }
    
  }
}

int is_line_empty(struct line_struct *line)
{
  /*
  Check if currentl line is empty

  params:
  :line - structure with line data

  :return - 1 if line is empty else 0
  */

  return (line->deleted || (line->final_cols == 0));
}

void print_line(struct line_struct *line)
{
  /*
  Print line and clear it from buffer

  params:
  :line - structure with line data
  */

  if (!is_line_empty(line))
  {
    printf("%s\n", line->line_string);
  }

  line->line_index++;
  line->line_string[0] = 0;
}

void delete_current_line(struct line_struct *line)
{
  /*
  Delete current line from buffer and increment delete offset

  params:
  :line - structure with line data
  */

  if (!is_line_empty(line))
  {
    line->line_string[0] = 0;
    line->deleted = 1;
  }
}

int insert_string(char *base_string, char *insert_string, int index)
{
  /*
  Insert one string to another to index position in base string

  params:
  :base_string - string to which we will insert string
  :insert_string - string that will be inserted to base_string
  :index - index of character from which insert_string will be inserted to base_string

  :return - 0 on success
          - -1 on fail
  */

  size_t base_string_length = strlen(base_string);
  size_t insert_string_length = strlen(insert_string);

  if ((base_string_length + insert_string_length) > MAX_LINE_LEN)
    return -1;

  // If index is larger than basestring lenght then insert position is lenght of base string
  size_t pos = ((size_t)index < base_string_length) ? (size_t)index : base_string_length;

  char final_string[MAX_LINE_LEN];

  // Add first part of base string
  for (size_t i=0; i < pos; ++i)
    final_string[i] = base_string[i];

  // Add insert string
  for (size_t i=0; i < insert_string_length; ++i)
    final_string[pos+i] = insert_string[i];

  // Add rest of base string
  for (size_t i=pos; i < base_string_length; ++i)
    final_string[i + insert_string_length] = base_string[i];

  // Add terminate character to the end
  final_string[base_string_length + insert_string_length] = 0;

  // Copy new string to base string
  strcpy(base_string, final_string);
  return 0;
}

int remove_substring(char *base_string, int start_index, int end_index)
{
  /*
  Remove substring from string base on input indexes

  params:
  :base_string - string from what will be substring removed
  :start_index - index of first removed char of substring
  :end_index - index of last removed char of substring

  :return - 0 on success
          - -1 on error
  */

  if (start_index < 0 || end_index < 0 || start_index > end_index)
    return -1;

  size_t string_len = strlen(base_string);

  char final_string[MAX_LINE_LEN];
  int i;
  size_t j;

  for (i=0; i < start_index; i++)
  {
    final_string[i] = base_string[i];
  }

  for (j=end_index + 1; j < string_len; ++j, ++i)
  {
    final_string[i] = base_string[j];
  }

  final_string[i] = 0;

  strcpy(base_string, final_string);
  return 0;
}

int clear_cell(struct line_struct *line, int index)
{
  /*
  Clear value from cell

  param:
  :line - structure with line data
  :index - index of cell to clear

  :return - 0 on success
          - -1 on error
  */

  if (index < 0)
    return -1;

  int start_index = get_start_of_substring(line, index);
  int end_index = get_end_of_substring(line, index);

  if (start_index < 0 || end_index < 0)
    return -1;

  return remove_substring(line->line_string, start_index, end_index);
}

int insert_to_cell(struct line_struct *line, int index, char *string)
{
  /*
  Insert string to column

  params:
  :line - structure with line data
  :index - index of column to insert string
  :string - string that will be inserted to cell

  :return - 0 on success
          - -1 on error
  */

  index = get_start_of_substring(line, index);
  if (index < 0)
    return -1;

  return insert_string(line->line_string, string, index);
}

int insert_empty_cell(struct line_struct *line, int index)
{
  /*
  Insert new cell before the one selected by index

  params:
  :line - structure with line data
  :index - index of cell before which will be inserted new cell

  :return - 0 on success
          - -1 on error
  */

  char empty_col[2] = {line->delim, '\0'};
  return insert_to_cell(line, index, empty_col);
}

int append_empty_cell(struct line_struct *line)
{
  /*
  Insert empty cell on the end of the line
  (Should be replaced by strcat)

  params:
  :line - structure with line data

  :return - 0 on success
          - -1 on errror
  */

  char empty_col[2] = {line->delim, '\0'};

  int index = get_end_of_substring(line, get_number_of_cells(line->line_string, line->delim) - 1);
  if (index < 0)
    return 0;

  return insert_string(line->line_string, empty_col, index);
}

int remove_cell(struct line_struct *line, int index)
{
  /*
  Remove whole cell

  params:
  :line - structure with line data
  :index - index of cell to remove

  :return - 0 on success
          - -1 on error
  */

  int start_index = get_start_of_substring(line, index);
  // offset to delim in front of substring if there is any to delete it
  if (index != 0 && (get_number_of_cells(line->line_string, line->delim) - 1) == index)
    start_index -= 1;

  // offset to delim char after the substring
  int end_index = get_end_of_substring(line, index) + 1;

  return remove_substring(line->line_string, start_index, end_index);
}

void get_selector(struct selector_arguments *selector, int argc, char *argv[])
{
  /*
  Get line selector from arguments
  Only first valid selector is loaded

  params:
  :selector - structor to save params for selector
  :argc - length of argument array
  :argv - argument array
  */

  // Offset -2 to be sure that there will be another 2 args after the selector flag
  for (int i=1; i < (argc - 2); i++)
  {
    for (int j=0; j < NUMBER_OF_AREA_SELECTOR_COMS; j++)
    {
      if (strcmp(argv[i], AREA_SELECTOR_COMS[j]) == 0)
      {
        switch (j)
        {
        case 0:
          // rows selector is valid when both arguments are int > 0 and a1 < a2 or -
          if (((argument_to_int(argv, argc, i+1) > 0) || are_strings_same(argv[i+1], "-")) &&
              ((argument_to_int(argv, argc, i+2) > 0) || are_strings_same(argv[i+2], "-")))
          {
            if (is_string_int(argv[i+1]) && is_string_int(argv[i+2]) &&
                (argument_to_int(argv, argc, i+1) > argument_to_int(argv, argc, i+2)))
            {
              break;
            }

            selector->selector_type = j;
            selector->a1 = argv[i+1];
            selector->a2 = argv[i+2];
            selector->ai1 = argument_to_int(argv, argc, i+1);
            selector->ai2 = argument_to_int(argv, argc, i+2);
            return;
          }
          break;

        case 1:
          if ((argument_to_int(argv, argc, i+1) > 0) || are_strings_same(argv[i+1], "-"))
          {
            selector->selector_type = j;
            selector->a1 = argv[i+1];
            selector->ai1 = argument_to_int(argv, argc, i+1);
            selector->str = argv[i+2];
            return;
          }
          break;

        case 2:
          if ((argument_to_int(argv, argc, i+1) > 0) || are_strings_same(argv[i+1], "-"))
          {
            selector->selector_type = j;
            selector->a1 = argv[i+1];
            selector->ai1 = argument_to_int(argv, argc, i+1);
            selector->str = argv[i+2];
            return;
          }
          break;
        
        default:
          break;
        }
      }
    }
  }

  selector->selector_type = -1;
}

void validate_line_processing(struct line_struct *line, struct selector_arguments *selector)
{
  /*
  Check if current line is valid for processing (selected by selector)

  params:
  :line - structure with line data
  :selector - structure with selector params
  */

  switch (selector->selector_type)
  {
  case 0:
    if ((are_strings_same(selector->a1, "-") && are_strings_same(selector->a2, "-") && line->last_line_flag) ||
        (selector->ai1 > 0 && are_strings_same(selector->a2, "-") && line->line_index >= (selector->ai1 - 1)) ||
        (selector->ai1 > 0 && selector->ai2 > 0 && line->line_index >= (selector->ai1 - 1) && line->line_index <= (selector->ai2 - 1)))
    {
      line->process_flag = 1;
      return;
    }
    break;

  case 1:
    if (selector->ai1 > 0 && selector->ai1 <= line->num_of_cols)
    {
      char substr[MAX_CELL_LEN];
      get_value_of_cell(line, selector->ai1 - 1, substr);

      if (is_string_start_with(substr, selector->str))
      {
        line->process_flag = 1;
        return;
      }
    }
    break;

  case 2:
    if (selector->ai1 > 0 && selector->ai1 <= line->num_of_cols)
    {
      char substr[MAX_CELL_LEN];
      get_value_of_cell(line, selector->ai1 - 1, substr);

      if (strstr(substr, selector->str) != NULL)
      {
        line->process_flag = 1;
        return;
      }
    }
    break;
  
  default:
    line->process_flag = 1;
    return;
  }

  line->process_flag = 0;
}

void process_line(struct line_struct *line, struct selector_arguments *selector, int argc, char *argv[], int operating_mode, int last_line_executed)
{
  /*
  Process loaded line data

  params:
  :line - structure with line data
  :selector - structure with selector params
  :argc - length of argument array
  :argv - argument array
  :operating_mode - operating mode of program
  :last_line_executed - flag to indicate that last line is executed
  */

  // Check if data in line should be processed
  validate_line_processing(line, selector);

  // Check if line length is in boundaries
  check_line_length(line);

  // Create buffer for cases when we are inserting new line
  char line_buffer[MAX_LINE_LEN + LINE_LENGTH_TEST_OFFSET];

  line->deleted = 0;
  line->final_cols = line->num_of_cols;

  for (int i=1; i < argc; i++)
  {
    switch (operating_mode)
    {
    case TABLE_EDIT:

      switch (get_table_edit_com_index(argv[i]))
      {
      case 0:
        if (argument_to_int(argv, argc, i+1) > 0 && argument_to_int(argv, argc, i+1) == (line->line_index + 1))
        {
          strcpy(line_buffer, line->line_string);
          create_empty_row(line);
        }
        break;

      case 2:
        if (argument_to_int(argv, argc, i+1) > 0 && !is_line_empty(line) && (argument_to_int(argv, argc, i+1) == (line->line_index + 1)))
        {
          delete_current_line(line);
        }
        break;

      case 3:
        if ((argument_to_int(argv, argc, i+1) > 0) && !is_line_empty(line) && (argument_to_int(argv, argc, i+2) > 0))
        {
          if ((argument_to_int(argv, argc, i+1) <= (line->line_index + 1)) && (argument_to_int(argv, argc, i+2) >= (line->line_index + 1)))
          {
            delete_current_line(line);
          }
        }
        break;

      // TODO: Rework colms to work with relative position to input not per step!
      case 4:
        if ((argument_to_int(argv, argc, i+1) > 0) && !is_line_empty(line) && (get_number_of_cells(line->line_string, line->delim) >= argument_to_int(argv, argc, i+1)))
        {
          if (insert_empty_cell(line, argument_to_int(argv, argc, i+1) - 1) < 0)
          {
            fprintf(stderr, "\nLine %d exceded max memory size! Max length of line is %d characters (including delims)\n", line->line_index+1, MAX_LINE_LEN);
            exit(MAX_LINE_LEN_EXCEDED);
          }

          line->final_cols++;
        }
        break;

      case 5:
        if (append_empty_cell(line) < 0 && !is_line_empty(line))
        {
          fprintf(stderr, "\nLine %d exceded max memory size! Max length of line is %d characters (including delims)\n", line->line_index+1, MAX_LINE_LEN);
          exit(MAX_LINE_LEN_EXCEDED);
        }

        line->final_cols++;
        break;

      case 6:
        if ((argument_to_int(argv, argc, i+1) > 0) && !is_line_empty(line) && (get_number_of_cells(line->line_string, line->delim) >= argument_to_int(argv, argc, i+1)))
        {
          if (remove_cell(line, (argument_to_int(argv, argc, i+1) - 1)) == 0)
          {
            line->final_cols--;
          }
        }
        break;

      case 7:
        if ((argument_to_int(argv, argc, i+1) > 0) && (argument_to_int(argv, argc, i+2) > 0) && !is_line_empty(line))
        {
          for (int j=argument_to_int(argv, argc, i+1); j <= argument_to_int(argv, argc, i+2); j++)
          {
            if (remove_cell(line, argument_to_int(argv, argc, i+1) - 1) == 0)
            {
              line->final_cols--;
            }
          }
        }
        break;
      
      default:
        break;
      }

      break;

    case DATA_EDIT:
      // Edit line data only when its flagged as line to edit
      if (line->process_flag)
      {
        switch (get_data_edit_com_index(argv[i]))
        {
        case 0:
          if ((argument_to_int(argv, argc, i+1) > 0) && (i + 2) < argc)
          {
            clear_cell(line, argument_to_int(argv, argc, i+1) - 1);
            insert_to_cell(line, argument_to_int(argv, argc, i+1) - 1, argv[i + 2]);
          }
          break;
        
        default:
          break;
        }
      }
      break;

    default:
      break;
    }
  }

  // Print line from line structure
  print_line(line);

  // Check if there is any line in buffer
  if (line_buffer[0] != 0)
  {
    // If there is line in buffer copy it to line structure, clear buffer and recursively call this function to process that line
    strcpy(line->line_string, line_buffer);
    line_buffer[0] = 0;
    process_line(line, selector, argc, argv, operating_mode, 0);
  }

  // There will be processed appending of new rows
  if (line->last_line_flag && !last_line_executed)
  {
    if (operating_mode == TABLE_EDIT)
    {
      for (int i=1; i < argc; i++)
      {
        if (get_table_edit_com_index(argv[i]) == 1)
        {
          create_empty_row(line);
          process_line(line, selector, argc, argv, operating_mode, 1);
        }
      }
    }
  }
}

int main(int argc, char *argv[])
{
  check_arguments(argc, argv);

  char delims[MAX_CELL_LEN] = " ";

  // Extract delims from args
  get_delims(argv, argc, delims);

  // Check if there are no invalid delim chars
  if (check_delim_characters(delims) < 0)
  {
    fprintf(stderr, "Invalid characters in delim string!\n");
    return ARG_ERROR;
  }

  // Create buffer strings for line and cell
  char line[MAX_LINE_LEN + LINE_LENGTH_TEST_OFFSET];
  char buffer_line[MAX_LINE_LEN + LINE_LENGTH_TEST_OFFSET];

  if (fgets(buffer_line, (MAX_LINE_LEN + LINE_LENGTH_TEST_OFFSET), stdin) == NULL)
  {
    printf("Input cant be empty");
    return INPUT_ERROR;
  }

  struct selector_arguments selector;
  int operating_mode = get_operating_mode(argv, argc);
  if (operating_mode == DATA_EDIT)
    get_selector(&selector, argc, argv);
  else
    selector.selector_type = -1;  

  struct line_struct line_holder;
  line_holder.delim = delims[0];
  line_holder.num_of_cols = get_number_of_cells(buffer_line, delims[0]);

  // Iterate over lines
  while (!line_holder.last_line_flag)
  {
    strcpy(line, buffer_line);
    line_holder.last_line_flag = fgets(buffer_line, (MAX_LINE_LEN + LINE_LENGTH_TEST_OFFSET), stdin) == NULL;

    remove_newline_character(line);
    replace_unused_delims(line, delims);
    line_holder.line_string = line;

    process_line(&line_holder, &selector, argc, argv, operating_mode, 0);
  }

  //Debug
  printf("\n\nDebug:\n");

  printf("Num of cols: %d\n", line_holder.num_of_cols);
  printf("Selector: type %d, a1: %s, a2: %s, str: %s\n", selector.selector_type, selector.a1, selector.a2, selector.str);

  printf("Args: ");
  for (int i=1; i < argc; i++)
  {
    printf("%s ", argv[i]);
  }
  // Debug end

  printf("\n");

  return 0;
}