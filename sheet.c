/*
                          Simple table processor
                              Version: 1
Program to process tables from standard input and outputs it to standard output

                             Martin Douša
                             October 2020
*/

#define DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MAX_CELL_LEN 100
#define MAX_LINE_LEN 10240
#define LINE_LENGTH_TEST_OFFSET 1
#define BLACKLISTED_DELIMS "\n\0\r"

const char *TABLE_EDIT_COMS[] = {"irow", "arow", "drow", "drows", "icol", "acol", "dcol", "dcols"};
#define NUMBER_OF_TABLE_EDIT_COMS 8
const char *DATA_EDIT_COMS[] = {"cset", "tolower", "toupper", "round", "int", "copy", "swap", "move", "csum", "cavg", "cmin", "cmax", "ccount", "cseq"};
#define NUMBER_OF_DATA_EDIT_COMS 14
const char *AREA_SELECTOR_COMS[] = {"rows", "beginswith", "contains"};
#define NUMBER_OF_AREA_SELECTOR_COMS 3

enum Mode {PASS, TABLE_EDIT, DATA_EDIT};
enum ReturnCodes {NO_ERROR, MAX_LINE_LEN_EXCEDED, MAX_CELL_LEN_EXCEDED, ARG_ERROR, INPUT_ERROR};
enum CellConversion {UPPER, LOWER, ROUND, INT};

struct line_struct
{
    char *line_string;
    char unedited_line_string[MAX_LINE_LEN + LINE_LENGTH_TEST_OFFSET + 1];
    char delim;
    int line_index;

    // Reference number of cols
    int num_of_cols;
    // Number of cols after editing
    int final_cols;

    int last_line_flag;
    int deleted;
    int process_flag;
    int error_flag;
};

struct selector_arguments
{
    int selector_type;
    char *a1, *a2, *str;
    int ai1, ai2;
};

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

void check_arguments(int argc, char *argv[], int *error_flag)
{
    /*
    Check if lenght of every single argument is in limit

    params:
    :argc - length of argument array
    :argv - array of arguments

    :return - SUCCESS if all arguments are in length limits
            - MAX_CELL_LEN_EXCEDED if some argument is longer
    */

    for (int i=1; i < argc; i++)
    {
        if (strlen(argv[i]) > MAX_CELL_LEN)
        {
            fprintf(stderr, "Argument %d exceded maximum allowed size! Maximum size is %d characters\n", i, MAX_CELL_LEN);
            (*error_flag) = MAX_CELL_LEN_EXCEDED;
            return;
        }
    }
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

void check_delim_characters(char *delims, int *error_flag)
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
                (*error_flag) = ARG_ERROR;
                return;
            }
        }
    }
}

void get_delims(char *input_array[], int array_len, char *delim)
{
    /*
    Extract delims string from args

    params:
    :input_array - array of strings (args)
    :array_len - number of args
    :delim - return string

     :return - SUCCESS on success parsing
             - ARG_ERROR on failed parsing
    */

    for (int i=1; i < array_len; i++)
    {
        if (are_strings_same(input_array[i], "-d"))
        {
            if ((i + 1) >= array_len)
            {
                fprintf(stderr, "Found delimiter flag without any value\n");
                return;
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

int get_number_of_cells(struct line_struct *line)
{
    /*
    Get number of cells in row
    Cell is substring separated by deliminator

    params:
    :line - structure with line data
    */

    return count_specific_chars(line->line_string, line->delim) + 1;
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

    if (index > (line->final_cols - 1))
        return -1;

    if (index == (line->final_cols - 1))
    {
        // if we are on the last cell we are going to the end of that line
        return (int)strlen(line->line_string);
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

    // Clear output string
    substring[0] = 0;

    if (index > (line->final_cols - 1) || index < 0)
        return -1;

    int start_index = get_start_of_substring(line, index);
    int end_index = get_end_of_substring(line, index);

    if (start_index < 0 && end_index < 0)
    {
        return -1;
    }

    // if start index is 0 and end index is -1 it means its first cell in row and its empty
    // then its not problem only it needs to set substring to empty string
    if (start_index == 0 && end_index == -1)
    {
        substring[0] = 0;
        return 0;
    }

    // exit if length of substring is larger than maximum size of one cell
    if ((end_index - start_index) > MAX_CELL_LEN)
    {
        fprintf(stderr, "\nCell %d on line %d exceded max memory size! Max length of cell is %d characters (exclude delims)\n", index + 1, line->line_index + 1, MAX_CELL_LEN);
        line->error_flag = MAX_CELL_LEN_EXCEDED;
        return -1;
    }

    // iterate over whole substring (we are recycling one and then we want to clear it)
    for (int i=0; i < (MAX_CELL_LEN + 1); i++, start_index++)
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

void check_line_sanity(struct line_struct *line)
{
    /*
    Check if line is no longer than maximum allowed length of one line and
    check if each cell is not larger than maximum allowed length of one cell

    params:
    :line - structure with line data
    */


    if (strlen(line->line_string) > MAX_LINE_LEN)
    {
        fprintf(stderr, "\nLine %d exceded max memory size! Max length of line is %d characters (including delims)\n", line->line_index+1, MAX_LINE_LEN);
        line->error_flag = MAX_LINE_LEN_EXCEDED;
        return;
    }

    char cell_buff[MAX_CELL_LEN + 1];
    for (int i=0; i < get_number_of_cells(line); i++)
    {
        get_value_of_cell(line, i, cell_buff);
        if (line->error_flag)
            return;
    }
}

int is_string_double(char *string)
{
    /*
    Check if input string is double

    params:
    :string - string to convertion

    :return - 1 if is double
            - 0 if not
    */

    if (string[0] == 0)
        return 0;

    char *foo;
    strtod(string, &foo);

    if (foo[0] != 0)
        return 0;

    return 1;
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

    // we dont want to convert empty strings to 0
    if (string[0] == 0)
        return -1;

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

    if (string[0] == 0)
        return 0;

    char *foo;
    strtol(string, &foo, 10);

    if (foo[0] != 0)
        return 0;

    return 1;
}

int is_double_int(double val)
{
    return floor(val) == val;
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

    // we dont want to convert empty strings to 0
    if (string[0] == 0)
        return -1;

    char *foo;
    (*val) = strtol(string, &foo, 10);

    if (foo[0] != 0)
        return -1;

    return 0;
}

void string_to_upper(char *string)
{
    /*
    Converts string to upper case

    params:
    :string - input string
    */

    for (size_t i=0; i < strlen(string); i++)
    {
        while (*string)
        {
            if (*string >= 'a' && *string <= 'z')
            {
                *string = toupper(*string);
            }

            string++;
        }
    }
}

void string_to_lower(char *string)
{
    /*
    Converts string to lower case

    params:
    :string - input string
    */

    for (size_t i=0; i < strlen(string); i++)
    {
        while (*string)
        {
            if (*string >= 'A' && *string <= 'Z')
            {
                *string = tolower(*string);
            }

            string++;
        }
    }
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

void generate_empty_row(struct line_struct *line)
{
    /*
    Create string with empty line format based on wanted line length and delim char
    Empty line is saved to line structure instead of current line string
    Line will have same number of cells as saved number of cells from first line

    params:
    :line - structure with line data
    */

    if (line->final_cols <= 0)
    {
        line->line_string[0] = 0;
        return;
    }

    int i = 0;
    for (; i < line->final_cols; i++)
    {
        if (i < (line->final_cols - 1))
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
#ifdef DEBUG
        printf("[Line debug] LI: %d, FC: %d Line data:\t\t", line->line_index, line->final_cols);
#endif

        printf("%s\n", line->line_string);
    }

    line->line_index++;
    line->line_string[0] = 0;
}

void delete_line_content(struct line_struct *line)
{
    /*
    Delete line string and if line wasnt already empty switch delete flag to true

    params:
    :line - structure with line data
    */

    if (!is_line_empty(line))
    {
        line->line_string[0] = 0;
        line->deleted = 1;
    }
}

int insert_string_to_line(struct line_struct *line, char *insert_string, int index)
{
    /*
    Insert string to line string

    params:
    :line - structure with line data
    :insert_string - string that will be inserted to base_string
    :index - index of character from which insert_string will be inserted to base_string

     :return - 0 on success
             - -1 on error
    */

    size_t base_string_length = strlen(line->line_string);
    size_t insert_string_length = strlen(insert_string);

    if ((base_string_length + insert_string_length) > MAX_LINE_LEN)
    {
        fprintf(stderr, "\nLine %d exceded max memory size! Max length of line is %d characters (including delims)\n", line->line_index+1, MAX_LINE_LEN);
        line->error_flag = MAX_LINE_LEN_EXCEDED;
        return -1;
    }

    // If index is larger than basestring lenght then insert position is lenght of base string
    size_t pos = ((size_t)index < base_string_length) ? (size_t)index : base_string_length;

    char final_string[MAX_LINE_LEN + 1];

    // Add first part of base string
    for (size_t i=0; i < pos; ++i)
        final_string[i] = line->line_string[i];

    // Add insert string
    for (size_t i=0; i < insert_string_length; ++i)
        final_string[pos+i] = insert_string[i];

    // Add rest of base string
    for (size_t i=pos; i < base_string_length; ++i)
        final_string[i + insert_string_length] = line->line_string[i];

    // Add terminate character to the end
    final_string[base_string_length + insert_string_length] = 0;

    // Copy new string to base string
    strcpy(line->line_string, final_string);
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

    char final_string[MAX_LINE_LEN + 1];
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

    return insert_string_to_line(line, string, index);
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
    int ret = insert_to_cell(line, index, empty_col);
    if (ret == 0)
        line->final_cols++;

    return ret;
}

void append_empty_cell(struct line_struct *line)
{
    /*
    Insert empty cell on the end of the line
    (Should be replaced by strcat)

    params:
    :line - structure with line data
    */

    if (line->final_cols < 1)
    {
        line->final_cols++;
        return;
    }

    line->final_cols++;

    char empty_col[2] = {line->delim, '\0'};

    int index = get_end_of_substring(line, line->final_cols - 1);
    if (index < 0)
    {
        return;
    }

    insert_string_to_line(line, empty_col, index);
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
    if (index != 0 && (line->final_cols - 1) == index)
        start_index -= 1;

    // offset to delim char after the substring
    int end_index = get_end_of_substring(line, index) + 1;
    int ret = remove_substring(line->line_string, start_index, end_index);

    if (ret == 0)
        line->final_cols--;

    return ret;
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

    // If valid selector not found set selector type to -1
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
                char substr[MAX_CELL_LEN + 1];
                if (get_value_of_cell(line, selector->ai1 - 1, substr) == 0)
                {
                    if (is_string_start_with(substr, selector->str))
                    {
                        line->process_flag = 1;
                        return;
                    }
                }

                if (line->error_flag)
                    return;
            }
            break;

        case 2:
            if (selector->ai1 > 0 && selector->ai1 <= line->num_of_cols)
            {
                char substr[MAX_CELL_LEN + 1];
                if (get_value_of_cell(line, selector->ai1 - 1, substr) == 0)
                {
                    if (strstr(substr, selector->str) != NULL)
                    {
                        line->process_flag = 1;
                        return;
                    }
                }

                if (line->error_flag)
                    return;
            }
            break;

        default:
            line->process_flag = 1;
            return;
    }

    line->process_flag = 0;
}

int is_cell_index_valid(struct line_struct *line, int index)
{
    /*
    Check if input index is valid index of cell in row

    params:
    :line - structure with line data
    :index - index of cell to check

    :return - 1 if is valid
            - 0 if not
    */

    return ((index > 0) && (index <= line->final_cols));
}

int get_sum_of_cells(struct line_struct *line, int start_index, int end_index, double *ret_val)
{
    /*
    Sum numbers in cells from inputed interval

    params:
    :line - structure with line data
    :start_index - start index of sum interval
    :end_index - end index of sum interval
    :ret_val - return pointer for value

    :return - number of summed number on success
            - -1 on fail
    */

    if (is_cell_index_valid(line, start_index) && end_index > 0 &&
        start_index <= end_index)
    {
        char cell_buff[MAX_CELL_LEN + 1];
        double sum_val = 0;
        int summed_counter = 0;

        for (int i=start_index; i <= end_index; i++)
        {
            if (get_value_of_cell(line, i - 1, cell_buff) == 0)
            {
                if (cell_buff[0] != 0 && is_string_double(cell_buff))
                {
                    double buf;
                    if (string_to_double(cell_buff, &buf) == 0)
                    {
                        sum_val += buf;
                        summed_counter++;
                    }
                }
            }
            else
            {
                if (line->error_flag)
                    return -1;
            }
        }

        (*ret_val) = sum_val;
        return summed_counter;
    }

    return -1;
}

void create_emty_row_at(struct line_struct *line, char *line_buffer, int index)
{
    /*
    Create empty row before index row in line string

    params:
    :line - structure with line data
    :line_buffer - output buffer for current content of line
    :index - index of row where to create empty line
    */

    if ((index > 0) && (index == (line->line_index + 1)))
    {
        strcpy(line_buffer, line->unedited_line_string);
        generate_empty_row(line);
    }
}

void delete_rows_in_interval(struct line_struct *line, int start_index, int end_index)
{
    /*
    Delete row if index of line is in passed interval
    Start and end indexes included

    params:
    :line - structure with line data
    :start_index - start index value
    :end_index - end index value
    */

    if (start_index > 0 && end_index > 0 &&
        start_index <= end_index)
    {
        if (start_index <= (line->line_index + 1) && end_index >= (line->line_index + 1))
        {
            delete_line_content(line);
        }
    }
}

void insert_empty_cell_at(struct line_struct *line, int index)
{
    /*
    Insert empty cell before cell of passed index if that cell exist

    params:
    :line - structure with line data
    :index - input index of cell
    */

    if (is_cell_index_valid(line, index))
    {
        insert_empty_cell(line, index - 1);
    }
}

void delete_cells_in_interval(struct line_struct *line, int start_index, int end_index)
{
    /*
    Delete cells with indexes in input interval
    In loop delete first cell several times

    params:
    :line - structure with line data
    :start_index - start index value
    :end_index - end index value
    */

    if (is_cell_index_valid(line, start_index) && end_index > 0 && start_index <= end_index)
    {

        // I am lazy to count number of loops
        for (int j=start_index; j <= end_index; j++)
        {
            if (!is_line_empty(line))
                remove_cell(line, start_index - 1);
        }
    }
}

void set_value_in_cell(struct line_struct *line, int index, char *value)
{
    /*
    Clear content of cell and insert new one

    :line - structure with line data
    :index - index of cell set value
    :value - string to set
    */

    if (is_cell_index_valid(line, index))
    {
        clear_cell(line, index - 1);
        insert_to_cell(line, index - 1, value);
    }
}

void cell_value_processing(struct line_struct *line, int index, int processing_flag)
{
    /*
    Function to process value of single cell
    Supported functions: UPPER - to uppercase
                         LOWER - to lowercase
                         ROUND - round value if its number
                         INT - conver value to int if its number

    params:
    :line - structure with line data
    :index - index of cell
    :processing_flag - flag of function
    */

    if (is_cell_index_valid(line, index))
    {
        char cell_buff[MAX_CELL_LEN + 1];

        if (get_value_of_cell(line, index - 1, cell_buff) == 0)
        {
            // Check if the cell is not number (int should be double too)
            if (!is_string_double(cell_buff))
            {
                if (processing_flag == UPPER)
                    string_to_upper(cell_buff);
                else if (processing_flag == LOWER)
                    string_to_lower(cell_buff);
            }
            else if (processing_flag == ROUND || processing_flag == INT)
            {
                double cell_double;

                if (string_to_double(cell_buff, &cell_double) == 0)
                {
                    if (processing_flag == ROUND)
                        snprintf(cell_buff, MAX_CELL_LEN + 1, "%d", (int)round(cell_double));
                    else
                        snprintf(cell_buff, MAX_CELL_LEN + 1, "%d", (int) cell_double);
                }
            }

            set_value_in_cell(line, index, cell_buff);
        }
    }
}

void copy_cell_value_to(struct line_struct *line, int source_index, int target_index)
{
    /*
    Copy value from one cell to another
    Source and target index cant be same

    params:
    :line - structure with line data
    :source_index - index of source cell
    :target_index - index of destination cell
    */

    if (is_cell_index_valid(line, source_index) && is_cell_index_valid(line, target_index) &&
        source_index != target_index)
    {
        char cell_buff[MAX_CELL_LEN + 1];

        if (get_value_of_cell(line, source_index - 1, cell_buff) == 0)
            set_value_in_cell(line, target_index, cell_buff);
    }
}

void swap_cell_values(struct line_struct *line, int index1, int index2)
{
    /*
    Swap values of cells
    Index1 and index2 can be same

    params:
    :line - structure with line data
    :index1 - index of first cell
    :index2 - index of second cell
    */

    if (is_cell_index_valid(line, index1) && is_cell_index_valid(line, index2) &&
        index1 != index2)
    {
        char cell_buff1[MAX_CELL_LEN + 1];
        char cell_buff2[MAX_CELL_LEN + 1];

        if ((get_value_of_cell(line, index1 - 1, cell_buff1) == 0) && (get_value_of_cell(line, index2 - 1, cell_buff2) == 0))
        {
            set_value_in_cell(line, index1, cell_buff2);
            set_value_in_cell(line, index2, cell_buff1);
        }
    }
}

void move_cell_to(struct line_struct *line, int source_index, int target_index)
{
    /*
    Move cell of source index before cell of target index
    Source and target index cant be same

    params:
    :line - structure with line data
    :source_index - index of cell to move
    :target_index - index of cell to move to
    */

    if (is_cell_index_valid(line, source_index) && is_cell_index_valid(line, target_index) &&
        source_index != target_index)
    {
        char cell_buff[MAX_CELL_LEN + 1];

        if (get_value_of_cell(line, source_index - 1, cell_buff) == 0)
        {
            if (source_index < target_index)
            {
                insert_empty_cell(line, target_index - 1);
                if (line->error_flag)
                    return;

                remove_cell(line, source_index - 1);
                insert_to_cell(line, target_index - 2, cell_buff);
            }
            else
            {
                insert_empty_cell(line, target_index - 1);
                if (line->error_flag)
                    return;

                remove_cell(line, source_index);
                insert_to_cell(line, target_index - 1, cell_buff);
            }
        }
    }
}

void sum_cells(struct line_struct *line, int output_index, int start_index, int end_index)
{
    /*
    Sum numbers in cells from inputed interval and save it to output cell
    Output index cant be in sum interval

    params:
    :line - structure with line data
    :output_index - index of cell where sum value will be saved
    :start_index - start index of sum interval
    :end_index - end index of sum interval
    */

    if (is_cell_index_valid(line, output_index) &&
        (output_index < start_index || output_index > end_index))
    {
        double sum_val;

        if (get_sum_of_cells(line, start_index, end_index, &sum_val) >= 0)
        {
            char cell_buff[MAX_CELL_LEN + 1];

            if (is_double_int(sum_val))
                snprintf(cell_buff, MAX_CELL_LEN + 1, "%d", (int)sum_val);
            else
                snprintf(cell_buff, MAX_CELL_LEN + 1, "%lf", sum_val);

            set_value_in_cell(line, output_index, cell_buff);
        }
    }
}

void avg_cells(struct line_struct *line, int output_index, int start_index, int end_index)
{
    if (is_cell_index_valid(line, output_index) &&
        (output_index < start_index || output_index > end_index))
    {
        double sum_val;
        int summed_cells = get_sum_of_cells(line, start_index, end_index, &sum_val);
        if (line->error_flag)
            return;

        if (summed_cells != -1)
        {
            sum_val /= summed_cells;
            char cell_buff[MAX_CELL_LEN + 1];

            if (is_double_int(sum_val))
                snprintf(cell_buff, MAX_CELL_LEN + 1, "%d", (int)sum_val);
            else
                snprintf(cell_buff, MAX_CELL_LEN + 1, "%lf", sum_val);

            set_value_in_cell(line, output_index, cell_buff);
        }
    }
}

void table_edit(struct line_struct *line, char *line_buffer, int argc, char *argv[], int com_index)
{
    switch (get_table_edit_com_index(argv[com_index]))
    {

        // TODO: Make row indexing consistent after removing/adding lines
        case 0:
            // irow R
            create_emty_row_at(line, line_buffer, argument_to_int(argv, argc, com_index + 1));
            break;

        case 2:
            // drow R
            delete_rows_in_interval(line, argument_to_int(argv, argc, com_index + 1), argument_to_int(argv, argc, com_index + 1));
            break;

        case 3:
            // drows N M
            delete_rows_in_interval(line, argument_to_int(argv, argc, com_index + 1), argument_to_int(argv, argc, com_index + 2));
            break;

        case 4:
            // icol C
            insert_empty_cell_at(line, argument_to_int(argv, argc, com_index + 1));
            break;

        case 5:
            // acol
            append_empty_cell(line);
            break;

        case 6:
            // dcol C
            delete_cells_in_interval(line, argument_to_int(argv, argc, com_index + 1), argument_to_int(argv, argc, com_index + 1));
            break;

        case 7:
            // dcols N M
            delete_cells_in_interval(line, argument_to_int(argv, argc, com_index + 1), argument_to_int(argv, argc, com_index + 2));
            break;

        default:
            break;
    }
}

void data_edit(struct line_struct *line, int argc, char *argv[], int com_index)
{
    // Edit line data only when its flagged as line to edit
    if (line->process_flag)
    {
        switch (get_data_edit_com_index(argv[com_index]))
        {
            case 0:
                // cset C STR
                if ((com_index + 2) < argc)
                    set_value_in_cell(line, argument_to_int(argv, argc, com_index + 1), argv[com_index + 2]);
                break;

            case 1:
                // tolower C
                cell_value_processing(line, argument_to_int(argv, argc, com_index + 1), LOWER);
                break;

            case 2:
                // toupper C
                cell_value_processing(line, argument_to_int(argv, argc, com_index + 1), UPPER);
                break;

            case 3:
                // round C
                cell_value_processing(line, argument_to_int(argv, argc, com_index + 1), ROUND);
                break;

            case 4:
                // int C
                cell_value_processing(line, argument_to_int(argv, argc, com_index + 1), INT);
                break;

            case 5:
                // copy N M
                copy_cell_value_to(line, argument_to_int(argv, argc, com_index + 1), argument_to_int(argv, argc, com_index + 2));
                break;

            case 6:
                // swap N M
                swap_cell_values(line, argument_to_int(argv, argc, com_index + 1), argument_to_int(argv, argc, com_index + 2));
                break;

            case 7:
                // move N M
                move_cell_to(line, argument_to_int(argv, argc, com_index + 1), argument_to_int(argv, argc, com_index + 2));
                break;

            case 8:
                // csum C N M
                sum_cells(line, argument_to_int(argv, argc, com_index + 1), argument_to_int(argv, argc, com_index + 2), argument_to_int(argv, argc, com_index + 3));
                break;

            case 9:
                // cavg C N M
                avg_cells(line, argument_to_int(argv, argc, com_index + 1), argument_to_int(argv, argc, com_index + 2), argument_to_int(argv, argc, com_index + 3));
                break;

            case 10:
                // cmin C N M
                break;

            case 11:
                // cmax C N M
                break;

            case 12:
                // ccount C N M
                break;

            case 13:
                // cseq N M B
                break;

            default:
                break;
        }
    }
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
    if (line->error_flag)
        return;

    // Sanity of line
    check_line_sanity(line);
    if (line->error_flag)
        return;

    // Create copy of line
    strcpy(line->unedited_line_string, line->line_string);

    // Create buffer for cases when we are inserting new line
    char line_buffer[MAX_LINE_LEN + LINE_LENGTH_TEST_OFFSET + 1];

    // Initialize/clear line states
    line->deleted = 0;
    line->final_cols = line->num_of_cols;

    for (int i=1; i < argc; i++)
    {
        switch (operating_mode)
        {
            case TABLE_EDIT:
                table_edit(line, line_buffer, argc, argv, i);
                break;

            case DATA_EDIT:
                data_edit(line, argc, argv, i);
                break;

            default:
                break;
        }

        if (line->error_flag)
            return;
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
                    // arow
                    generate_empty_row(line);
                    print_line(line);
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    char delims[MAX_CELL_LEN + 1] = " ";
    int error_flag = NO_ERROR;

    // Check sanity of arguments
    check_arguments(argc, argv, &error_flag);
    if (error_flag)
        return error_flag;

    // Extract delims from args
    get_delims(argv, argc, delims);

    // Check if there are no invalid delim chars
    check_delim_characters(delims, &error_flag);
    if (error_flag)
        return error_flag;

    // Create buffer strings for line and cell
    char line[MAX_LINE_LEN + LINE_LENGTH_TEST_OFFSET + 1];
    char buffer_line[MAX_LINE_LEN + LINE_LENGTH_TEST_OFFSET + 1];

    if (fgets(buffer_line, (MAX_LINE_LEN + LINE_LENGTH_TEST_OFFSET + 1), stdin) == NULL)
    {
        fprintf(stderr, "Input cant be empty");
        return INPUT_ERROR;
    }

    struct selector_arguments selector;
    int operating_mode = get_operating_mode(argv, argc);

    get_selector(&selector, argc, argv);

    // Init line hodler
    struct line_struct line_holder;
    line_holder.delim = delims[0];
    line_holder.error_flag = NO_ERROR;

    // Iterate over lines
    while (!line_holder.last_line_flag)
    {
        strcpy(line, buffer_line);
        line_holder.last_line_flag = fgets(buffer_line, (MAX_LINE_LEN + LINE_LENGTH_TEST_OFFSET + 1), stdin) == NULL;

        remove_newline_character(line);
        replace_unused_delims(line, delims);
        line_holder.line_string = line;

        if (line_holder.line_index == 0)
            line_holder.num_of_cols = get_number_of_cells(&line_holder);

        process_line(&line_holder, &selector, argc, argv, operating_mode, 0);
        if (line_holder.error_flag)
            return line_holder.error_flag;
    }

#ifdef DEBUG
    printf("\n\nDebug:\n");

    printf("Base cols: %d Final cols: %d\n", line_holder.num_of_cols, line_holder.final_cols);
    printf("Selector: type %d, a1: %s, a2: %s, str: %s\n", selector.selector_type, selector.a1, selector.a2, selector.str);

    printf("Args: ");
    for (int i=1; i < argc; i++)
    {
        printf("%s ", argv[i]);
    }
#endif

    printf("\n");

    return 0;
}