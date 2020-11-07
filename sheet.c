/**
 * @version V2
 * @file sheet.c
 * @author Martin Douša
 * @date October 2020
 * @brief Program to process tables from standard input and outputs it to standard output
 */

#define DEBUG /**< Debug flag, comment to dont print additional information of each line */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <float.h>

#define MAX_CELL_LEN 100 /**< Maximum number of normal characters of one cell or one argument */
#define MAX_LINE_LEN 10240 /**< Maximum length of one whole line (row) */

#define DEFAULT_DELIM " " /**< Default delim used when no delim is passed as argument */
#define BLACKLISTED_DELIMS "\r\n" /**< Character that are not allowed to use as delim character */

const char *TABLE_COMS[] = {"irow", "arow", "drow", "drows", "icol", "acol", "dcol", "dcols"}; /**< Reference array of implemented table editing commands */
#define NUMBER_OF_TABLE_COMS 8 /**< Number of implemented table editing commands */
const char *DATA_COMS[] = {"cset", "tolower", "toupper", "round", "int", "copy", "swap", "move", "csum", "cavg", "cmin", "cmax", "ccount", "cseq"}; /**< Reference array of implemented data editing commands */
#define NUMBER_OF_DATA_COMS 14 /**< Number of implemented data editing commands */
const char *SELECTOR_COMS[] = {"rows", "beginswith", "contains"}; /**< Referencce array of implemented row selector commands */
#define NUMBER_OF_SELECTOR_COMS 3 /**< Number of implemented row selector commands */

/**
 * @enum OperatingMode
 * @brief Flags for operating mode
 */
enum OperatingMode {
    PASS,                         /**< Pass thru mode of program - data or table will not be edited */
    TABLE_EDIT,                   /**< Only table edit commands will be processed */
    DATA_EDIT                     /**< Only data edit commands will be processed */
};

/**
 * @enum ErrorCodes
 * @brief Error flags
 *
 * Error flags for situations when program must be ended before finishing
 */
enum ErrorCodes {
    NO_ERROR,                     /**< No error detected (default flag) */
    MAX_LINE_LEN_EXCEDED,         /**< Max length of line is exceded */
    MAX_CELL_LEN_EXCEDED,         /**< Max length of single cell/argument is exceded */
    INPUT_ERROR,                  /**< No input data fed to program */
    ARG_ERROR                     /**< Invalid or unexpected argument */
};

/**
 * @enum SingleCellFunction
 * @brief Flags for functions to edit single cell
 */
enum SingleCellFunction {
    UPPER,                        /**< To upper case */
    LOWER,                        /**< To lower case */
    ROUND,                        /**< Round double */
    INT                           /**< Remove decimal places from number */
};

/**
 * @enum MultiCellFunction
 * @brief Flags for functions to edit multiple cells in row
 */
enum MultiCellFunction {
    SUM,                          /**< Sum number values of cells */
    MIN,                          /**< Get minimum number value */
    MAX,                          /**< Get maximum number value */
    AVG,                          /**< Get average of number values */
    COUNT                         /**< Count not empty cells */
};

/**
 * @struct Line
 * @brief Store for data of one line
 */
typedef struct
{
    char line_string[MAX_LINE_LEN + 2];                /**< String that contains one line loaded from stdin */
    char line_buffer[MAX_LINE_LEN + 2];                /**< Buffer for current line when adding new line before current */
    char unedited_line_string[MAX_LINE_LEN + 2];       /**< Backup of line string */
    char delim;                                        /**< Delim character for current table */
    int line_index;                                    /**< Index of current line */

    int num_of_cols;                                   /**< Reference number of cols */
    int final_cols;                                    /**< Number of cols after editing */

    int last_line_flag;                                /**< Flag if current line is last line */
    int deleted;                                       /**< Flag if current line is deleted */
    int process_flag;                                  /**< Flag if current line is selected to data process by selector */
    int error_flag;                                    /**< If error occured when processing line */
} Line;

/**
 * @struct Selector
 * @brief Structure to contain information about selector
 *
 * Selector is used to select lines to process by data edit commands
 */
typedef struct
{
    int selector_type;                    /**< Index of used selector */
    char *a1,                             /**< First string argument */
         *a2,                             /**< Second string argument */
         *str;                            /**< Third string argument */
    int ai1,                              /**< Int version of first argument (string converted to int) */
        ai2;                              /**< Int version of second argument (string converted to int) */
} Selector;

int round_double(double val)
{
    /**
     * @brief Round double value to int
     *
     * @param val Double value to round
     *
     * @return Rounded value (int)
     */

    return (int)(val + 0.5);
}

int strings_equal(const char *s1, const char *s2)
{
    /**
     * @brief Check if two string @p s1 and @p s2 are same
     *
     * @param s1 First string
     * @param s2 Second string
     *
     * @return 1 if string are equal, 0 if dont
     */

    return strcmp(s1, s2) == 0;
}

int string_start_with(const char *base_string, const char *start_string)
{
    /**
     * @brief Check if string starts with other string
     *
     * Check if @p base_string starts with @p start_string
     *
     * @param base_string String where to look for substring
     * @param start_string Substring to look for at start of base_string
     *
     * @return 1 if @p base_string starts with @p start_string, 0 if dont
     */

    if (strlen(start_string) > strlen(base_string))
        return 0;

    return strncmp(start_string, base_string, strlen(start_string)) == 0;
}

int chck_args(int argc, char **argv)
{
    /**
     * @brief Check if lenght of every single argument is in limit
     *
     * Iterate over all arguments in @p argv and check if leng of every single argument is less or equal than MAX_CELL_LEN
     *
     * @param argc Length of argument array
     * @param argv Array of arguments
     *
     * @return Error flag - NO_ERROR if all arguments are not larger than MAX_CELL_LENGTH else MAX_CELL_LEN_EXCEDED
     */

    for (int i = 1; i < argc; i++)
    {
        if (strlen(argv[i]) > MAX_CELL_LEN)
        {
            fprintf(stderr, "Argument %d exceded maximum allowed size! Maximum size is %d characters\n", i, MAX_CELL_LEN);
            return MAX_CELL_LEN_EXCEDED;
        }
    }

    return NO_ERROR;
}

// command_selectors
int get_table_com_index(char *com)
{
    /**
     * @brief Get index of command from table edit command array
     *
     * Iterate over all table edit command and check if inputed command @p com is int that array
     *
     * @param com Command string
     *
     * @return Index of command in array of table edit command or -1 if command is not in that array
     *
     * @see get_data_com_index
     */

    for (int i = 0; i < NUMBER_OF_TABLE_COMS; i++)
    {
        if (strings_equal(com, TABLE_COMS[i]))
        {
            return i;
        }
    }
    return -1;
}

int get_data_com_index(char *com)
{
    /**
     * @brief Get index of command from data edit command array
     *
     * Iterate over all data edit command and check if inputed command @p com is int that array
     *
     * @param com Command string
     *
     * @return Index of command in array of data edit command or -1 if command is not in that array
     *
     * @see get_table_com_index
     */

    for (int i = 0; i < NUMBER_OF_DATA_COMS; i++)
    {
        if (strings_equal(com, DATA_COMS[i]))
        {
            return i;
        }
    }
    return -1;
}
// command_selectors

int get_op_mode(char **input_array, int array_len)
{
    /**
     * @brief Get operating mode of program
     *
     * Check arguments to determinate operating mode of program
     * If first not delim argument is row selector or data editing command then its data editing mode
     * if its table editing command then its table editing mode and if there is no argument then its pass mode
     *
     * @param input_array Array of arguments
     * @param array_len Length of array with arguments
     *
     * @return Flag of mode to use
     */

    for (int i = 1; i < array_len; i++)
    {
        if (get_table_com_index(input_array[i]) >= 0)
            return TABLE_EDIT;

        if (get_data_com_index(input_array[i]) >= 0)
            return DATA_EDIT;

        for (int j = 0; j < NUMBER_OF_SELECTOR_COMS; j++)
        {
            if (strings_equal(input_array[i], SELECTOR_COMS[j]))
                return DATA_EDIT;
        }
    }

    return PASS;
}

void rm_newline_chars(char *s) {
    /**
     * @brief Removing new line character from string
     *
     * Iterate over string until it new line character then replace it with 0
     * @warning
     * Rest of the string is REMOVED!
     *
     * @param s Pointer to string from which is new line character removed
     */

    while(*s && *s != '\n' && *s != '\r')
        s++;

    *s = 0;
}

char *get_opt(int argc, char *argv[], char *opt_flag)
{
    /**
     * @brief Get optional argument from array of arguments
     *
     * Iterate over array of arguments until flag is found
     * then check if there is another argument after the flag and return it
     *
     * @param argc Number of arguments
     * @param argv Array of arguments
     * @param opt_flag Flag of optional argument to look for
     *
     * @return Argument found after the flag or NULL if @p opt_flag is NULL, flag not found or there is no argument after the flag
     */

    if (opt_flag == NULL)
        return NULL;

    for (int i = 0; i < argc; i++)
    {
        if (strings_equal(argv[i], opt_flag))
        {
            if ((i + 1) < argc)
            {
                return argv[i + 1];
            }
        }
    }

    return NULL;
}

char *get_delims(char *input_array[], int array_len)
{
    /**
     * @brief Get delims for current input data from argument of program or from DEFAULT_DELIM
     *
     * @param input_array Array of strings (args)
     * @param array_len Number of args
     *
     * @return Array of delim chars to use
     */

    char *arg_delims = get_opt(array_len, input_array, "-d");
    return arg_delims == NULL ? DEFAULT_DELIM : arg_delims;
}

void normalize_delims(char *string, const char* delims)
{
    /**
     * @brief Replace all delims by first delim
     *
     * Iterate over string and replace delims that are not on 0 position in delims string with delim on 0 position
     *
     * @param string String where to replace delims
     * @param delims String with delims
     */

    for (size_t i = 0; string[i]; i++)
    {
        for (size_t j = 1; delims[j]; j++)
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

int count_specific_chars(const char *string, char ch)
{
    /**
     * @brief Count number of specific characters in string
     *
     * @param string Input string where count chars
     * @param ch Char we want to count
     *
     * @return Number of found characters
     */

    int delim_counter = 0;
    for (size_t i = 0; string[i]; i++)
    {
        if (string[i] == ch) delim_counter++;
    }

    return delim_counter;
}

int is_line_empty(Line *line)
{
    /**
     * @brief Check if currentl line is empty
     *
     * Line is empty when line is flagged as deleted or number of colms is 0
     *
     * @param line Structure with line data
     *
     * @return 1 if line is empty else 0
     */

    return (line->deleted || (line->final_cols == 0));
}

int get_number_of_cells(Line *line)
{
    /**
     * @brief Get number of cells in row
     *
     * If line is empty return 0, if not then count delims and add 1 to it
     * (Valid cells are on both sides of delim)
     * Cell is substring separated by deliminator
     *
     * @param line Structure with line data
     *
     * @return Number of colms detected in row
     */

    if (line->line_string[0] == 0)
        return 0;

    return count_specific_chars(line->line_string, line->delim) + 1;
}

int get_position_of_character(char *string, char ch, int index)
{
    /**
     * @brief Get position of character of certain index in string
     *
     * Iterate over each character in string and count character ocurences
     * If ocurence counter coresponse to @p index then return position of current char in string
     *
     * @param string String where to find char
     * @param ch Character we are looking for
     * @param index Index of occurence of character in string we want position for
     *
     * @return Position of @p index ocurence of @p ch in string if valid ocurence is found, if not -1
     *
     * @todo Rework to strchr
     */

    if (index > (count_specific_chars(string, ch) - 1) || index < 0) return -1;

    int counter = 0;
    for (size_t i = 0; string[i]; i++)
    {
        if (string[i] == ch)
        {
            counter++;
            if ((counter - 1) == index)
                return i;
        }
    }

    return -1;
}

int get_start_of_substring(Line *line, int index)
{
    /**
     * @brief Get start index of substring (cell) from line string limited by delim
     *
     * @param line Structure with line data
     * @param index Index of substring (cell) in line
     *
     * @return Index of first character of that wanted substring if found, if not -1
     */

    if (index < 0 || index > line->final_cols)
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

int get_end_of_substring(Line *line, int index)
{
    /**
     * @brief Get last index of substring (cell) from line string limited by delim
     *
     * @param line Structure with line data
     * @param index Index of substring (cell) in line
     *
     * @return Index of last character of that wanted substring if found, if not -1
     */

    if (index > (line->final_cols - 1))
        return -1;

    if (index >= (line->final_cols - 1))
    {
        // if we are on the last cell we are going to the end of that line to the index of last char
        return (int)strlen(line->line_string) - 1;
    }
    else
    {
        // last character of substring before delim
        // position of delim - 1
        return get_position_of_character(line->line_string, line->delim, index) - 1;
    }
}

int get_value_of_cell(Line *line, int index, char *substring)
{
    /**
     * @brief Extract value of cell
     *
     * Try to find cell in row and return its value to @p substring
     * @warning
     * Empty string is saved to substring if cell is not found
     *
     * @param line Structure with line data
     * @param index Index of cell
     * @param substring Pointer to string where to save result
     *
     * @return 0 on success, -1 on error
     */

    // Check if index of cell is valid
    if (index > (line->final_cols - 1) || index < 0)
        return -1;

    // Get indexes of substring
    int start_index = get_start_of_substring(line, index);
    int end_index = get_end_of_substring(line, index);

    // If both indexes are bad then its error
    if (start_index < 0 && end_index < 0)
        return -1;

    // if start index is 0 and end index is -1 it means its first cell in row and its empty
    // then its not problem only it needs to set substring to empty string
    if (start_index == 0 && end_index == -1)
    {
        substring[0] = 0;
        return 0;
    }

    // Return if length of substring is larger than maximum size of one cell
    if ((end_index - start_index + 1) > MAX_CELL_LEN)
    {
        fprintf(stderr, "\nCell %d on line %d exceded max memory size! Max length of cell is %d characters (exclude delims)\n", index + 1, line->line_index + 1, MAX_CELL_LEN);
        line->error_flag = MAX_CELL_LEN_EXCEDED;
        return -1;
    }

    /*
        for (int i=0; i < (MAX_CELL_LEN + 1) && start_index + i <= end_index; i++)
            substring[i] = line->line_string[start_index+i];
        substring[end_index-start_index+1] = '\0';
     */

    // Save wanted substring to substring array
    int length = end_index - start_index + 1;

    // Check if length is valid (only for case when the cell of inputed index doesnt exist)
    if (length < 0)
        return -1;

    strncpy(substring, &(line->line_string[start_index]), length);

    return 0;
}

int check_line_sanity(Line *line)
{
    /**
     * @brief Check if line is loaded in correct way and its in limits
     *
     * Check if line is no longer than maximum allowed length of one line and
     * check if each cell is not larger than maximum allowed length of one cell
     *
     * @param line Structure with line data
     *
     * @return 1 if line is loaded correctly, 0 if not
     */

    if (line->error_flag)
        return 0;

    if ((strlen(line->line_string) == MAX_LINE_LEN + 1) && (line->line_string[MAX_LINE_LEN + 1] != '\n'))
    {
        fprintf(stderr, "\nLine %d exceded max memory size! Max length of line is %d characters (including delims)\n", line->line_index+1, MAX_LINE_LEN);
        line->error_flag = MAX_LINE_LEN_EXCEDED;
        return 0;
    }

    char cell_buff[MAX_CELL_LEN + 1];
    int num_of_cells = get_number_of_cells(line);

    for (int i = 0; i < num_of_cells; i++)
    {
        get_value_of_cell(line, i, cell_buff);
        if (line->error_flag)
            return 0;
    }

    return 1;
}

int is_string_double(char *string)
{
    /**
     * @brief Check if input string is double
     *
     * @param string String to check
     *
     * @return - 1 if its double (any number because its could be double too), 0 if not
     */

    if (string[0] == 0)
        return 0;

    // Pointer to unprocessed part of string
    char *rest;
    strtod(string, &rest);

    if (rest[0] != 0)
        return 0;

    return 1;
}

int string_to_double(char *string, double *val)
{
    /**
     * @brief Convert string to double
     *
     * Check if input string could be converted to double and then convert it to double
     * @warning
     * Empty string is not 0! Its failed conversion.
     *
     * @param string String to convertion
     * @param val Pointer to double where result will be stored
     *
     * @return 0 if conversion is success,-1 if conversion failed
     */

    // we dont want to convert empty strings to 0
    if (string[0] == 0)
        return -1;

    // Pointer to unprocessed part of string
    char *rest;
    (*val) = strtod(string, &rest);

    // If rest of string is not empty then string cant be converted to double
    if (rest[0] != 0)
        return -1;

    return 0;
}

int is_string_int(char *string)
{
    /**
     * @brief Check if input string is intiger
     *
     * @param string String to check
     *
     * @return 1 if string is intiger, 0 if not
     */

    if (string[0] == 0)
        return 0;

    // Pointer to unprocessed part of string
    char *rest;
    strtol(string, &rest, 10);

    if (rest[0] != 0)
        return 0;

    return 1;
}

int string_to_int(char *string, int *val)
{
    /**
     * @brief Convert string to int
     *
     * Check if input string could be converted to intiger and then convert it to intiger
     * @warning
     * Empty string is not 0! Its failed conversion.
     *
     * @param string String to convertion
     * @param val Pointer to int where result will be stored
     *
     * @return 0 if conversion is success,-1 if conversion failed
     */

    // we dont want to convert empty strings to 0
    if (string[0] == 0)
        return -1;

    // Pointer to unprocessed part of string
    char *rest;
    (*val) = (int)strtol(string, &rest, 10);

    // If rest of string is not empty then string cant be converted to int
    if (rest[0] != 0)
        return -1;

    return 0;
}

int is_double_int(double val)
{
    /**
     * @brief Check if double could be converted to int without loss of precision
     *
     * @param val Double value to check
     *
     * @return 1 if it could be converted without loss of precision, 0 if not
     */

    if (val == 0) return 1;
    return (int)(val) == val;
}

void string_conversion(char *string, int conversion_flag)
{
    /**
     * @brief Converts string based on selected functions
     *
     * Supported functions: UPPER - string to uppercase
     *                      LOWER - string to lowercase
     * @warning
     * If invalid flag is inputed then no processing will happen
     *
     * @param string Input string
     * @param conversion_flag Flag of selected function
     */

    if (conversion_flag != UPPER && conversion_flag != LOWER)
        return;

    for (size_t i = 0; string[i]; i++)
    {
        // iterate over string
        while (*string)
        {
            if ((*string >= 'a' && *string <= 'z') || (*string >= 'A' && *string <= 'Z'))
            {
                if (conversion_flag == UPPER)
                    *string = (char)toupper(*string);
                else
                    *string = (char)tolower(*string);
            }

            string++;
        }
    }
}

int argument_to_int(char *input_array[], int array_len, int index)
{
    /**
     * @brief Try to convert argument to int
     *
     * @param input_array Array with arguments
     * @param array_len Lenght of array with arguments
     * @param index Index of argument we want convert to int
     *
     * @return Int value of argument or 0 on error
     */

    int val;

    if (index >= array_len || string_to_int(input_array[index], &val) != 0)
        return 0;

    return val;
}

void generate_empty_row(Line *line)
{
    /**
     * @brief Create empty line in @p line based on current number of cols
     *
     * Create string with empty line format based on wanted line length and delim char
     * Empty line is saved to line structure instead of current line string
     * Line will have same number of cells as saved number of cells from first line
     *
     * @param line Structure with line data
     */

    if (line->final_cols <= 0)
    {
        line->line_string[0] = 0;
        return;
    }

    int i = 0;
    for (; i < (line->final_cols - 1); i++)
    {
        line->line_string[i] = line->delim;
    }

    line->line_string[i] = 0;
}

void print_line(Line *line)
{
    /**
     * @brief Print line and clear it from buffer
     *
     * @warning
     * Line is only printed if its not flagged as deleted or have 0 colms
     *
     * @param line Structure with line data
     */

    if (!is_line_empty(line))
    {
#ifdef DEBUG
        printf("[Line debug] LI: %d, FC: %d, PF: %d Line data:\t\t", line->line_index, line->final_cols, line->process_flag);
#endif

        printf("%s\n", line->line_string);
    }

    line->line_index++;
    line->line_string[0] = 0;
}

void delete_line_content(Line *line)
{
    /**
     * @brief Delete line
     *
     * Delete line string and if line wasnt already empty switch delete flag to true
     *
     * @param line Structure with line data
     */

    if (!is_line_empty(line))
    {
        line->line_string[0] = 0;
        line->deleted = 1;
    }
}

int insert_string_to_line(Line *line, char *insert_string, int index)
{
    /**
     * @brief Insert value to line
     *
     * Insert string @p insert_string before character of @p index (Content will be preserved only shifted)
     *
     * @param line Structure with line data
     * @param insert_string String that will be inserted to base_string
     * @param index Index of character before which will be @p insert_string inserted
     *
     * @return - 0 on success, -1 on error
     *
     * @todo Split to working with string and working with line itself
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

    // Add first part of base string (exclude character on pos index)
    for (size_t i = 0; i < pos; ++i)
        final_string[i] = line->line_string[i];

    // Add insert string
    for (size_t i = 0; i < insert_string_length; ++i)
        final_string[pos+i] = insert_string[i];

    // Add rest of base string
    for (size_t i = pos; i < base_string_length; ++i)
        final_string[i + insert_string_length] = line->line_string[i];

    // Add terminate character to the end
    final_string[base_string_length + insert_string_length] = 0;

    // Copy new string to base string
    strcpy(line->line_string, final_string);
    return 0;
}

int remove_substring(char *base_string, int start_index, int end_index)
{
    /**
     * @brief Remove substring from string base on input indexes
     *
     * @param base_string String from what will be substring removed
     * @param start_index Index of first removed char of substring
     * @param end_index Index of last removed char of substring
     *
     * @return - 0 on success, -1 on error
     */

    if (start_index < 0 || end_index < 0 || start_index > end_index)
        return -1;

    size_t string_len = strlen(base_string);

    char final_string[MAX_LINE_LEN + 1];
    int i;

    // Add first part of string
    for (i = 0; i < start_index; i++)
    {
        final_string[i] = base_string[i];
    }

    // Skip middle part of string and add end part
    for (size_t j = end_index + 1; j < string_len; ++j, ++i)
    {
        final_string[i] = base_string[j];
    }

    final_string[i] = 0;

    strcpy(base_string, final_string);
    return 0;
}

int insert_to_cell(Line *line, int index, char *string)
{
    /**
     * @brief Insert string to cell
     *
     * Value of @p string will be injected to cell of @p index
     *
     * @param line Structure with line data
     * @param index Index of column to insert string
     * @param string String that will be inserted to cell
     *
     * @return - 0 on success, -1 on error
     */

    // Get position of first character in cell
    index = get_start_of_substring(line, index);
    if (index < 0)
        return -1;

    // This will insert string BEFORE the index
    return insert_string_to_line(line, string, index);
}

int insert_empty_cell(Line *line, int index)
{
    /**
     * @brief Insert new cell before the one selected by index
     *
     * @param line Structure with line data
     * @param index Index of cell before which will be inserted new cell
     *
     * @return - 0 on success, -1 on error
     */

    // When inserting to existing row of cells there is always delim
    char empty_col[2] = {line->delim, '\0'};

    // Insert string with delim in front of value in cell
    int ret = insert_to_cell(line, index, empty_col);
    if (ret == 0)
        line->final_cols++;

    return ret;
}

void append_empty_cell(Line *line)
{
    /**
     * @brief Insert empty cell on the end of the line
     *
     * @param line Structure with line data
     */

    if (line->final_cols < 1)
    {
        line->final_cols++;
        return;
    }

    line->final_cols++;

    char empty_col[2] = {line->delim, '\0'};

    // Insert new empty colm and check sanity of that line
    strcat(line->line_string, empty_col);
    check_line_sanity(line);
}

int remove_cell(Line *line, int index)
{
    /**
     * @brief Remove whole cell
     *
     * @warning
     * Cell itself will be deleted!
     *
     * @param line Structure with line data
     * @param index Index of cell to remove
     *
     * @return - 0 on success, -1 on error
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

int clear_cell(Line *line, int index)
{
    /**
     * @brief Remove current value from cell
     *
     * @param line Structure with line data
     * @param index Index of cell to clear
     *
     * @return - 0 on success, -1 on error
     */

    if (index < 0)
        return -1;

    int start_index = get_start_of_substring(line, index);
    int end_index = get_end_of_substring(line, index);

    if (start_index < 0 || end_index < 0)
        return -1;

    // Remove subring with value of cell
    return remove_substring(line->line_string, start_index, end_index);
}

int is_cell_index_valid(Line *line, int index)
{
    /**
     * @brief Check if index is valid index of cell in current row
     *
     * @param line Structure with line data
     * @param index Index of cell to check
     *
     * @return - 0 on success, -1 on error
     */

    return ((index > 0) && (index <= line->final_cols));
}

void get_selector(Selector *selector, int argc, char *argv[])
{
    /**
     * @brief Extract selector from arguments
     *
     * Get line selector from arguments
     * Only first valid selector is loaded
     * Selector could be anywhere
     *
     * @param selector Structor to save params for selector
     * @param argc Length of argument array
     * @param argv Argument array
     */

    // Offset -2 to be sure that there will be another 2 args after the selector flag
    for (int i = 1; i < (argc - 2); i++)
    {
        for (int j = 0; j < NUMBER_OF_SELECTOR_COMS; j++)
        {
            if (strcmp(argv[i], SELECTOR_COMS[j]) == 0)
            {
                switch (j)
                {
                    case 0:
                        // rows selector is valid when both arguments are int > 0 and a1 < a2 or -
                        if (((argument_to_int(argv, argc, i+1) > 0) || strings_equal(argv[i + 1], "-")) &&
                            ((argument_to_int(argv, argc, i+2) > 0) || strings_equal(argv[i + 2], "-")))
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
                        if ((argument_to_int(argv, argc, i+1) > 0) || strings_equal(argv[i + 1], "-"))
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

void validate_line_processing(Line *line, Selector *selector)
{
    /**
     * @brief Check if current line will be processed
     *
     * Check if current line is selected by selector
     *
     * @param line Structure with line data
     * @param selector Structure with selector params
     */

    switch (selector->selector_type)
    {
        case 0:
            // rows N M
            // If both args are - then allow only last line
            // If arg 1 is larger than 0 and arg 2 is - then check if current line index is larger or equal arg 1
            // If both args are numbers larger than 0 then check if current line index is between or equal
            if ((strings_equal(selector->a1, "-") && strings_equal(selector->a2, "-") && line->last_line_flag) ||
                (selector->ai1 > 0 && strings_equal(selector->a2, "-") && line->line_index >= (selector->ai1 - 1)) ||
                (selector->ai1 > 0 && selector->ai2 > 0 && line->line_index >= (selector->ai1 - 1) && line->line_index <= (selector->ai2 - 1)))
            {
                line->process_flag = 1;
                return;
            }
            break;

        case 1:
            // beginswith C STR
            if (is_cell_index_valid(line, selector->ai1))
            {
                char substr[MAX_CELL_LEN + 1];
                if (get_value_of_cell(line, selector->ai1 - 1, substr) == 0)
                {
                    // Check if selected cell starts with string from argument
                    if (string_start_with(substr, selector->str))
                    {
                        line->process_flag = 1;
                        return;
                    }
                }
            }
            break;

        case 2:
            // contains C STR
            if (is_cell_index_valid(line, selector->ai1))
            {
                char substr[MAX_CELL_LEN + 1];
                if (get_value_of_cell(line, selector->ai1 - 1, substr) == 0)
                {
                    // Check if cell contains string from argument
                    if (strstr(substr, selector->str) != NULL)
                    {
                        line->process_flag = 1;
                        return;
                    }
                }
            }
            break;

        default:
            // If there is no valid selector then set processing flag to 1
            line->process_flag = 1;
            return;
    }

    // If selector contains some garbage or the current line is not valid by selector then set processing flag to 0
    line->process_flag = 0;
}

int process_row_values(Line *line, int start_index, int end_index, double *ret_val, int function_flag)
{
    /**
     * @brief Process and return value of operation performed on row
     *
     * Supported functions: SUM - sum of selected number cells
     *                      MIN - minimum value of selected number cells
     *                      MAX - maximum value of selected number cells
     *                      COUNT - count number of non empty cells
     *
     * @param line Structure with line data
     * @param start_index Start of cell index interval
     * @param end_index End of cell index interval
     * @param ret_val Return pointer for double value
     * @param function_flag Flag of function used to aquire return value
     *
     * @return - number cells it run thru, -1 on fail
     */

    if (is_cell_index_valid(line, start_index) && end_index > 0 &&
        start_index <= end_index)
    {
        char cell_buff[MAX_CELL_LEN + 1];
        double return_value = 0;
        int cell_count = 0;

        // Set initial return numbers for min/max functions
        if (function_flag == MIN || function_flag == MAX)
            return_value = function_flag == MIN ? DBL_MAX : DBL_MIN;

        for (int i = start_index; i <= end_index; i++)
        {
            // Load value of cell
            if (get_value_of_cell(line, i - 1, cell_buff) == 0)
            {
                // Check if string is number
                if (is_string_double(cell_buff))
                {
                    double buf;

                    // Convert string to double
                    if (string_to_double(cell_buff, &buf) == 0)
                    {
                        switch (function_flag)
                        {
                            case AVG:
                            case SUM:
                                return_value += buf;
                                break;

                            case MIN:
                                if (buf < return_value)
                                    return_value = buf;
                                break;

                            case MAX:
                                if (buf > return_value)
                                    return_value = buf;
                                break;

                            default:
                                break;
                        }
                    }
                }

                // When we are in counting mode then cell count is our output value and we want count only not empty cells
                if (function_flag != COUNT || cell_buff[0] != 0)
                    cell_count++;
            }
            else if (line->error_flag)
                return -1;
        }

        // If return value is still at initial values then no valid value was processed
        if (return_value == DBL_MIN || return_value == DBL_MAX)
            return -1;

        // If mode is count then we want return cell count as output
        if (function_flag == COUNT)
            (*ret_val) = (double)cell_count;
        else
            (*ret_val) = return_value;

        return cell_count;
    }

    return -1;
}

void create_emty_row_at(Line *line, int index)
{
    /**
     * @brief Create empty row before row of @p index
     *
     * @param line Structure with line data
     * @param index Index of row where to create empty line
     */

    if ((index > 0) && (index == (line->line_index + 1)))
    {
        // Copy current line to unedited line buffer
        strcpy(line->line_buffer, line->unedited_line_string);
        // Create empty line in line object
        generate_empty_row(line);
    }
}

void delete_rows_in_interval(Line *line, int start_index, int end_index)
{
    /**
     * @brief Delete row if index of line is in passed interval
     *
     * Start and end indexes included
     *
     * @param line Structure with line data
     * @param start_index Start of row index interval
     * @param end_index End of row index interval
     */

    if (start_index > 0 && end_index > 0 &&
        start_index <= end_index)
    {
        // If line index is in interval delete it
        if (start_index <= (line->line_index + 1) && end_index >= (line->line_index + 1))
        {
            delete_line_content(line);
        }
    }
}

void insert_empty_cell_at(Line *line, int index)
{
    /**
     * @brief Insert empty cell before cell of passed index if that cell exist
     *
     * @param line Structure with line data
     * @param index Index of cell
     */

    // If cell index is valid insert empty cell before index
    if (is_cell_index_valid(line, index))
    {
        insert_empty_cell(line, index - 1);
    }
}

void delete_cells_in_interval(Line *line, int start_index, int end_index)
{
    /**
     * @brief Delete cells with indexes in input interval
     *
     * In loop delete first cell several times
     *
     * @param line Structure with line data
     * @param start_index Start of cell index interval
     * @param end_index End of cell index interval
     */

    if (is_cell_index_valid(line, start_index) && end_index > 0 && start_index <= end_index)
    {
        // I am lazy to count number of loops
        for (int j = start_index; j <= end_index; j++)
        {
            // If line is not empty try to remove cell
            if (!is_line_empty(line))
                remove_cell(line, start_index - 1);
        }
    }
}

int set_value_in_cell(Line *line, int index, char *value)
{
    /**
     * @brief Set new value to cell
     *
     * Clear content of cell and insert new value
     *
     * @param line Structure with line data
     * @param index Index of cell to set value
     * @param value String to set
     *
     * @return - 0 on sucess, -1 on error
     */

    if (is_cell_index_valid(line, index))
    {
        // Clear value from cell
        clear_cell(line, index - 1);
        // Set new value to cell
        return insert_to_cell(line, index - 1, value);
    }

    return -1;
}

void cell_value_editing(Line *line, int index, int processing_flag)
{
    /**
     * @brief Function to process value of single cell
     *
     * Supported functions: UPPER - to uppercase
     *                      LOWER - to lowercase
     *                      ROUND - round value if its number
     *                      INT - conver value to int if its number
     *
     * @param line Structure with line data
     * @param index Index of cell
     * @param processing_flag Flag of function
     */

    if (is_cell_index_valid(line, index))
    {
        char cell_buff[MAX_CELL_LEN + 1];

        // Load value from cell
        if (get_value_of_cell(line, index - 1, cell_buff) == 0)
        {
            // Check if the cell is not number (int should be double too)
            if (!is_string_double(cell_buff))
            {
                // Upper/lower conversion of string
                if (processing_flag == UPPER)
                    string_conversion(cell_buff, UPPER);
                else if (processing_flag == LOWER)
                    string_conversion(cell_buff, LOWER);
            }
            else if (processing_flag == ROUND || processing_flag == INT)
            {
                double cell_double;

                // Convert string to double
                if (string_to_double(cell_buff, &cell_double) == 0)
                {
                    // Round/int double processing
                    if (processing_flag == ROUND)
                        snprintf(cell_buff, MAX_CELL_LEN + 1, "%d", round_double(cell_double));
                    else
                        snprintf(cell_buff, MAX_CELL_LEN + 1, "%d", (int)cell_double);
                }
            }

            // Set processed value to cell
            set_value_in_cell(line, index, cell_buff);
        }
    }
}

void copy_cell_value_to(Line *line, int source_index, int target_index)
{
    /**
     * @brief Copy value from one cell to another
     *
     * @warning
     * @p source_index and @p target_index cant be same
     *
     * @param line Structure with line data
     * @param source_index Index of source cell
     * @param target_index Index of destination cell
     */

    if (is_cell_index_valid(line, source_index) && is_cell_index_valid(line, target_index) &&
        source_index != target_index)
    {
        char cell_buff[MAX_CELL_LEN + 1];

        // Load value of source cell
        if (get_value_of_cell(line, source_index - 1, cell_buff) == 0)
            // Set value to target cell
            set_value_in_cell(line, target_index, cell_buff);
    }
}

void swap_cell_values(Line *line, int index1, int index2)
{
    /**
     * @brief Swap values of cells
     *
     * @warning
     * @p index1 and @p index2 cant be same
     *
     * @param line Structure with line data
     * @param index1 Index of first cell
     * @param index2 Index of second cell
     */

    if (is_cell_index_valid(line, index1) && is_cell_index_valid(line, index2) &&
        index1 != index2)
    {
        char cell_buff1[MAX_CELL_LEN + 1];
        char cell_buff2[MAX_CELL_LEN + 1];

        // Load both cells
        if ((get_value_of_cell(line, index1 - 1, cell_buff1) == 0) && (get_value_of_cell(line, index2 - 1, cell_buff2) == 0))
        {
            // Set new values to cells
            set_value_in_cell(line, index1, cell_buff2);
            set_value_in_cell(line, index2, cell_buff1);
        }
    }
}

void move_cell_to(Line *line, int source_index, int target_index)
{
    /**
     * @brief Move cell of @p source_index before cell of @p target_index
     *
     * @warning
     * @p source_index and @p target_index cant be same
     *
     * @param line Structure with line data
     * @param source_index Index of cell to move
     * @param target_index Index of cell to move to
     */

    if (is_cell_index_valid(line, source_index) && is_cell_index_valid(line, target_index) &&
        source_index != target_index)
    {
        char cell_buff[MAX_CELL_LEN + 1];

        if (get_value_of_cell(line, source_index - 1, cell_buff) == 0)
        {
            if (source_index < target_index)
            {
                // Insert empty cell before target cell
                if (insert_empty_cell(line, target_index - 1) != 0)
                    return;

                // Remove original cell
                remove_cell(line, source_index - 1);
                // Insert value to before created empty cell
                insert_to_cell(line, target_index - 2, cell_buff);
            }
            else
            {
                // Insert empty cell before target cell
                if (insert_empty_cell(line, target_index - 1) != 0)
                    return;

                // Remove original cell
                remove_cell(line, source_index);
                // Insert value to before created empty cell
                insert_to_cell(line, target_index - 1, cell_buff);
            }
        }
    }
}

void row_values_processing(Line *line, int output_index, int start_index, int end_index, int function_flag)
{
    /**
     * @brief Process cells with index in inputed interval by function selected by flag
     *
     * Supported functions: SUM - sum of selected number cells
     *                      AVG - count average value of selected number cells
     *                      MIN - minimum value of selected number cells
     *                      MAX - maximum value of selected number cells
     *                      COUNT - count number of non empty cells
     *
     * @warning
     * @p output_index cant be in processing interval
     *
     * @param line Structure with line data
     * @param output_index Index of cell where to output processed value
     * @param start_index Start index of processing interval
     * @param end_index End index of processing interval
     * @param function_flag Flag of function to use
     */

    if (is_cell_index_valid(line, output_index) &&
        (output_index < start_index || output_index > end_index))
    {
        double setval;
        char cell_buff[MAX_CELL_LEN + 1];
        int processed_cells;

        if ((processed_cells = process_row_values(line, start_index, end_index, &setval, function_flag)) == -1)
            return;

        if (function_flag == AVG)
            setval /= processed_cells;

        // Check if double can be converted to int without loss
        if (is_double_int(setval))
            snprintf(cell_buff, MAX_CELL_LEN + 1, "%d", (int)setval);
        else
            snprintf(cell_buff, MAX_CELL_LEN + 1, "%lf", setval);

        set_value_in_cell(line, output_index, cell_buff);
    }
}

void row_sequence_gen(Line *line, int start_index, int end_index, int start_value)
{
    /**
     * @brief Create sequence of numbers in cells from inputed interval
     *
     * Create sequence of number starting from @p start_value in cells with index from @p start_index to @p end_index
     *
     * @param line Structure with line data
     * @param start_index Start index of processing interval
     * @param end_index End index of processing interval
     * @param start_value Start value of number sequence
     */

    if (is_cell_index_valid(line, start_index) && end_index > 0)
    {
        char cell_buff[MAX_CELL_LEN + 1];

        for (int i = start_index; i <= end_index; i++, start_value++)
        {
            snprintf(cell_buff, MAX_CELL_LEN + 1, "%d", start_value);
            if (set_value_in_cell(line, i, cell_buff) != 0)
                return;
        }
    }
}

void table_edit(Line *line, int argc, char *argv[], int com_index)
{
    /**
     * @brief Function to apply table edit command to line
     *
     *
     * @param line Structure with line data
     * @param argc Lenght of argument array
     * @param argv Argument array
     * @param com_index Index of current processing argument
     *
     * @todo Make row indexing consistent after removing/adding lines
     */

    if (line->error_flag)
        return;

    switch (get_table_com_index(argv[com_index]))
    {
        case 0:
            // irow R
            create_emty_row_at(line, argument_to_int(argv, argc, com_index + 1));
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

void data_edit(Line *line, int argc, char *argv[], int com_index)
{
    /**
     * @brief Function to apply data edit command to line
     *
     * @param line Structure with line data
     * @param argc Lenght of argument array
     * @param argv Argument array
     * @param com_index Index of current processing argument
     */

    if (line->error_flag)
        return;

    // Edit line data only when its flagged as line to edit
    if (line->process_flag)
    {
        switch (get_data_com_index(argv[com_index]))
        {
            case 0:
                // cset C STR
                if ((com_index + 2) < argc)
                    set_value_in_cell(line, argument_to_int(argv, argc, com_index + 1), argv[com_index + 2]);
                break;

            case 1:
                // tolower C
                cell_value_editing(line, argument_to_int(argv, argc, com_index + 1), LOWER);
                break;

            case 2:
                // toupper C
                cell_value_editing(line, argument_to_int(argv, argc, com_index + 1), UPPER);
                break;

            case 3:
                // round C
                cell_value_editing(line, argument_to_int(argv, argc, com_index + 1), ROUND);
                break;

            case 4:
                // int C
                cell_value_editing(line, argument_to_int(argv, argc, com_index + 1), INT);
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
                row_values_processing(line, argument_to_int(argv, argc, com_index + 1), argument_to_int(argv, argc, com_index + 2), argument_to_int(argv, argc, com_index + 3), SUM);
                break;

            case 9:
                // cavg C N M
                row_values_processing(line, argument_to_int(argv, argc, com_index + 1), argument_to_int(argv, argc, com_index + 2), argument_to_int(argv, argc, com_index + 3), AVG);
                break;

            case 10:
                // cmin C N M
                row_values_processing(line, argument_to_int(argv, argc, com_index + 1), argument_to_int(argv, argc, com_index + 2), argument_to_int(argv, argc, com_index + 3), MIN);
                break;

            case 11:
                // cmax C N M
                row_values_processing(line, argument_to_int(argv, argc, com_index + 1), argument_to_int(argv, argc, com_index + 2), argument_to_int(argv, argc, com_index + 3), MAX);
                break;

            case 12:
                // ccount C N M
                row_values_processing(line, argument_to_int(argv, argc, com_index + 1), argument_to_int(argv, argc, com_index + 2), argument_to_int(argv, argc, com_index + 3), COUNT);
                break;

            case 13:
                // cseq N M B
                row_sequence_gen(line, argument_to_int(argv, argc, com_index + 1), argument_to_int(argv, argc, com_index + 2), argument_to_int(argv, argc, com_index + 3));
                break;

            default:
                break;
        }
    }
}

void process_line(Line *line, Selector *selector, int argc, char *argv[], int operating_mode)
{
    /**
     * @brief Process loaded line data
     *
     * @param line Structure with line data
     * @param selector Structure with selector params
     * @param argc Length of argument array
     * @param argv Argument array
     * @param operating_mode Operating mode of program
     */

    // Initialize/clear line states
    line->deleted = 0;
    line->final_cols = line->num_of_cols;

    // Check if data in line should be processed
    validate_line_processing(line, selector);

    // Sanity of line
    if (!check_line_sanity(line))
        return;

    for (int i = 1; i < argc; i++)
    {
        // Perform actions based on operating mode (command from other operating modes will be ignored)
        switch (operating_mode)
        {
            case TABLE_EDIT:
                table_edit(line, argc, argv, i);
                break;

            case DATA_EDIT:
                data_edit(line, argc, argv, i);
                break;

            default:
                break;
        }

        // If there was some memory error return to main
        if (line->error_flag)
            return;
    }

    // Print line from line structure
    print_line(line);

    // Check if there is any line in buffer
    if (line->line_buffer[0] != 0)
    {
        // If there is line in buffer copy it to line structure, clear buffer and recursively call this function to process that line
        strcpy(line->line_string, line->line_buffer);
        line->line_buffer[0] = 0;
        process_line(line, selector, argc, argv, operating_mode);
    }

    // There will be processed appending of new rows
    if (line->last_line_flag)
    {
        if (operating_mode == TABLE_EDIT)
        {
            for (int i = 1; i < argc; i++)
            {
                if (get_table_com_index(argv[i]) == 1)
                {
                    // arow
                    generate_empty_row(line);
                    print_line(line);
                }
            }
        }
    }
}

int is_last_row()
{
    /**
     * @brief Check if current line is the last line of input
     * Check if next char of stdin (standart input) is EOF (end of file) and if not return the loaded character back to stream
     *
     * @return 1 if program is on last line, 0 if not
     */

    int c = fgetc(stdin);
    if (c == EOF)
        return 1;

    ungetc(c, stdin);
    return 0;
}

int main(int argc, char *argv[])
{
    /**
     * @brief Main function of program
     *
     * Base checking of args
     * Initialization of program
     * Loading lines (Rows of table)
     * Error handling
     *
     * @param argc Number of arguments passed to program
     * @param argv Array of arguments passed to program
     *
     * @return return code of program, 0 on success, anything else if error
     */

    // Check sanity of arguments
    int error_flag;
    if ((error_flag = chck_args(argc, argv)) != NO_ERROR)
        return error_flag;

    // Extract delims from args
    char *delims = get_delims(argv, argc);

    size_t number_of_delims = strlen(delims);
    size_t number_of_blacklisted_delims = strlen(BLACKLISTED_DELIMS);

    for (size_t i = 0; i < number_of_delims; i++)
    {
        for (size_t j = 0; j < number_of_blacklisted_delims; j++)
        {
            if (delims[i] == BLACKLISTED_DELIMS[j])
            {
                fprintf(stderr, "\nFound invalid delimiter!\n");
                return ARG_ERROR;
            }
        }
    }

    // Check operating mode of program based on inputed arguments
    int operating_mode = get_op_mode(argv, argc);

    // Get selector
    Selector selector;
    get_selector(&selector, argc, argv);

    // Init line hodler
    Line line_holder = { .delim = delims[0],
            .error_flag = NO_ERROR };

    // Iterate over lines
    while (fgets(line_holder.line_string, (MAX_LINE_LEN + 2), stdin) != NULL)
    {
        // Check if we are at last row
        line_holder.last_line_flag = is_last_row();

        // Go thru line and replace all delims with one
        normalize_delims(line_holder.line_string, delims);

        if (line_holder.line_index == 0)
            line_holder.num_of_cols = get_number_of_cells(&line_holder);

        // Remove new line character from line
        rm_newline_chars(line_holder.line_string);

        // Create backup of line
        strcpy(line_holder.unedited_line_string, line_holder.line_string);

        process_line(&line_holder, &selector, argc, argv, operating_mode);
        if (line_holder.error_flag)
            return line_holder.error_flag;
    }

    // If line index here is still 0 then no data was inputed
    if (line_holder.line_index == 0)
    {
        fprintf(stderr, "Input cant be empty!");
        return INPUT_ERROR;
    }

#ifdef DEBUG
    printf("\n\nDebug:\n");

    printf("Base cols: %d Final cols: %d\n", line_holder.num_of_cols, line_holder.final_cols);
    printf("Selector: type %d, a1: %s, a2: %s, str: %s\n", selector.selector_type, selector.a1, selector.a2, selector.str);
    printf("Delim: '%c'\n", line_holder.delim);

    printf("Args: ");
    for (int i = 1; i < argc; i++)
    {
        printf("%s ", argv[i]);
    }
#endif

    printf("\n");

    return 0;
}