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

struct line_struct
{
  char *line_string;
  int line_index;
  int num_of_cols;
  char delim;
  int last_line;
  int del_offset;
};

int get_table_edit_com_index(char *com)
{
  for (int i=0; i < NUMBER_OF_TABLE_EDIT_COMS; i++)
  {
    if (strcmp(com, TABLE_EDIT_COMS[i]) == 0)
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
    if (strcmp(com, DATA_EDIT_COMS[i]) == 0)
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
    if (strcmp(com, AREA_SELECTOR_COMS[i]) == 0)
    {
      return i;
    }
  }
  return -1;
}

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

int getDelims(char *input_array[], int array_len, char *delim)
{
  /*
  Extract delims string from args

  params:
  :input_array - array of strings (args)
  :array_len - number of args
  :delim - return string
  
  :return - return 0 if delims are found
                   -1 if found flag but not value
                   -2 if size of delim string is too large
                   -3 if no delims found
  */

  for (int i=1; i < array_len; i++)
  {
      if (strcmp(input_array[i], "-d") == 0)
      {
        if ((i + 1) >= array_len)
        {
          return-1;
        }

        if (strlen(input_array[i + 1]) > MAX_CELL_LEN) return -2;

        strcpy(delim, input_array[i + 1]);
        return 0;
      }
  }

  return -3;
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

int get_sub_string(char *string, char delim, int index, char *substring, int max_length)
{
  /*
  Extract substring from string

  params:
  :string - input string
  :delim - deliminator for substrings
  :index - index of substring
  :substring - string for saving result
  :max_length - maximal length of one cell
  */

  int number_of_cells = get_number_of_cells(string, delim);

  if (index > (number_of_cells - 1)) return -2;

  int start_index;
  int end_index;

  if (index == 0)
  {
    // if we want first substring there is no delim before substring then we are starting from first char of string
    start_index = 0;
  }
  else
  {
    // get position of delim before that substring and offset to char after that delim
    start_index = get_position_of_character(string, delim, index - 1) + 1;
  }

  if (index == (number_of_cells - 1))
  {
    // if we are on the last cell we are going to the end of that line
    end_index = strlen(string);
  }
  else
  {
    // last character of substring is one char before position of wanted delim
    end_index = get_position_of_character(string, delim, index) - 1;
  }

  // return if length of substring is larger than maximum size of one cell
  if ((end_index - start_index) > max_length) return -1;
  
  // iterate over whole substring (we are recycling one and then we want to clear it)
  for (int i=0; i < max_length; i++, start_index++)
  {
    if (start_index <= end_index)
    {
      substring[i] = string[start_index];
    }
    else
    {
      substring[i] = 0;
      break;
    }
  }

  return 0;
}

void check_line_length(struct line_struct line)
{
  // Check if length of line isnt larger than max size of line
  if (strlen(line.line_string) > MAX_LINE_LEN)
  {
    fprintf(stderr, "\nLine %d exceded max memory size! Max length of line is %d characters (including delims)\n", line.line_index+1, MAX_LINE_LEN);
    exit(-5);
  }
}

int string_to_double(char *string, double *val)
{
  /*
  Check if input string could be double and then convert it to double

  params:
  :string - string to convertion
  :val - output double value

  :return - 0 if conversion is success, -1 if not
  */

  char *foo;
  (*val) = strtod(string, &foo);

  if (foo == string) return -1;
  return 0;
}

int string_to_int(char *string, int *val)
{
  /*
  Check if input string could be intiger and then convert it to intiger

  params:
  :string - string to convertion
  :val - output intiger value

  :return - 0 if conversion is success, -1 if not
  */

  for (size_t i=0; i < strlen(string); i++)
  {
    if(string[i] < '0' || string[i] > '9') return -1;
  }

  (*val) = atoi(string);

  return 0;
}

int argument_to_unsigned_int(char *input_array[], int array_len, int index)
{
  int val;
  if (index > (array_len + 1)) return -1;
  if (string_to_int(input_array[index], &val) != 0) return -2;
  if (val <= 0) return -3;

  return val;
}

void create_empty_row(int length, char delim, char *return_string)
{
  if (length <= 0)
  {
    return_string[0] = 0;
    return;
  }

  int i = 0;
  for (; i < length; i++)
  {
    if (i < (length - 1))
      return_string[i] = delim;
    else
    {
      return_string[i] = 0;
      break;
    }
    
  }
}

void print_line(struct line_struct *line)
{
  if (line->line_string[0] != 0)
  {
    printf("%s\n", line->line_string);
  }

  line->line_string[0] = 0;
  line->line_index++;
}

void delete_current_line(struct line_struct *line)
{
  if (line->line_string[0] != 0)
  {
    line->line_string[0] = 0;
    line->del_offset++;
  }
}

int insert_string(char base_string[], const char insert_string[], int index)
{
  size_t base_string_length = strlen(base_string);
  size_t insert_string_length = strlen(insert_string);

  if ((base_string_length + insert_string_length) > MAX_LINE_LEN) return -1;

  // If index is larger than basestring lenght then insert position is lenght of base string
  size_t pos = ((size_t)index < base_string_length) ? (size_t)index : base_string_length;

  char final_string[MAX_LINE_LEN];
  size_t i;

  // Add first part of base string
  for (i=0; i<pos; ++i)
    final_string[i] = base_string[i];

  // Add insert string
  for (i=0; i < insert_string_length; ++i)
    final_string[pos+i] = insert_string[i];

  // Add rest of base string
  for (i=pos; i < base_string_length; ++i)
    final_string[i + insert_string_length] = base_string[i];

  // Add terminate character to the end
  final_string[base_string_length + insert_string_length] = 0;

  // Copy new string to base string
  strcpy(base_string, final_string);
  return 0;
}

void process_line(struct line_struct *line, int argc, char *argv[], int operating_mode, int last_line_executed)
{
  // Check if line length is in boundaries
  check_line_length(*line);

  // Create buffer for cases when we are inserting new line
  char line_buffer[MAX_LINE_LEN + 1 + LINE_LENGTH_TEST_OFFSET];

  for (int i=1; i < argc; i++)
  {
    switch (operating_mode)
    {
    case TABLE_EDIT:

      switch (get_table_edit_com_index(argv[i]))
      {
      case 0:
        if (argument_to_unsigned_int(argv, argc, i+1) > 0 && argument_to_unsigned_int(argv, argc, i+1) == (line->line_index + 1))
        {
          strcpy(line_buffer, line->line_string);
          create_empty_row(line->num_of_cols, line->delim, line->line_string);
        }
        break;

      // In delete row functions there is offset to correct line index relative to position relative to NEW positions in case of another delete
      case 2:
        if (argument_to_unsigned_int(argv, argc, i+1) > 0 && (argument_to_unsigned_int(argv, argc, i+1) == (line->line_index + 1 + line->del_offset)))
        {
          delete_current_line(line);
        }
        break;

      case 3:
        if ((argument_to_unsigned_int(argv, argc, i+1) > 0) && (argument_to_unsigned_int(argv, argc, i+2) > 0))
        {
          if ((argument_to_unsigned_int(argv, argc, i+1) <= (line->line_index + 1)) && (argument_to_unsigned_int(argv, argc, i+2) >= (line->line_index + 1 + line->del_offset)))
          {
            delete_current_line(line);
          }
        }
        break;

      case 4:
        break;

      case 5:
        break;
      
      default:
        break;
      }

      break;

    case DATA_EDIT:
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
    process_line(line, argc, argv, operating_mode, 0);
  }

  if (line->last_line && !last_line_executed)
  {
    if (operating_mode == TABLE_EDIT)
    {
      for (int i=1; i < argc; i++)
      {
        if (get_table_edit_com_index(argv[i]) == 1)
        {
          create_empty_row(line->num_of_cols, line->delim, line->line_string);

          line->line_index++;

          process_line(line, argc, argv, operating_mode, 1);
        }
      }
    }
  }
}

int main(int argc, char *argv[])
{
  char delims[MAX_CELL_LEN] = " ";

  // Extract delims from args
  int delims_ret = getDelims(argv, argc, delims);
  switch (delims_ret)
  {
  case -1:
    fprintf(stderr, "Found delimiter flag without any value\n");
    return -1;

  case -2:
    fprintf(stderr, "Delimiter string max length exceded! Max length is %d characters\n", MAX_CELL_LEN);
    return -2;
  
  default:
    break;
  }

  // Check if there are no invalid delim chars
  if (check_delim_characters(delims) < 0)
  {
    fprintf(stderr, "Invalid characters in delim string!");
    return -3;
  }

  // Create buffer strings for line and cell
  char line[MAX_LINE_LEN + LINE_LENGTH_TEST_OFFSET];

  if (fgets(line, (MAX_LINE_LEN + LINE_LENGTH_TEST_OFFSET), stdin) == NULL)
  {
    printf("Input cant be empty");
    return -4;
  }

  int operating_mode = get_operating_mode(argv, argc);

  struct line_struct line_holder;
  line_holder.delim = delims[0];
  line_holder.num_of_cols = get_number_of_cells(line, delims[0]);

  // Iterate over lines
  while (!line_holder.last_line)
  {
    remove_newline_character(line);
    replace_unused_delims(line, delims);
    line_holder.line_string = line;

    process_line(&line_holder, argc, argv, operating_mode, 0);

    // After last line is loaded process it
    if ((line_holder.last_line = fgets(line, (MAX_LINE_LEN + LINE_LENGTH_TEST_OFFSET), stdin) == NULL))
    {
      remove_newline_character(line);
      replace_unused_delims(line, delims);
      line_holder.line_string = line;

      process_line(&line_holder, argc, argv, operating_mode, 0);
    }
  }

  //Debug
  printf("\n\nDebug:\n");

  printf("Num of cols: %d\n", line_holder.num_of_cols);

  printf("Args: ");
  for (int i=1; i < argc; i++)
  {
    printf("%s ", argv[i]);
  }
  // Debug end

  printf("\n");

  return 0;
}