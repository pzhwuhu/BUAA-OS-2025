/*
 * Enhanced Interactive Shell Implementation for MOS Operating System
 * Features: Command parsing, variable expansion, history management, I/O redirection
 * Authors: Advanced Shell Development Team
 * Version: 2.1
 */

#include <args.h>
#include <lib.h>

// Configuration constants and character definitions
#define WHITESPACE_CHARS " \t\r\n"
#define OPERATOR_SYMBOLS "<|>&;()"
#define HISTORY_BUFFER_SIZE 20
#define MAX_ARGUMENTS 128
#define TEMP_STRING_SIZE 256
#define LARGE_BUFFER_SIZE 1024
#define PATH_BUFFER_SIZE 128

// Global state management
static char command_history_buffer[HISTORY_BUFFER_SIZE][LARGE_BUFFER_SIZE] = {{0}};
static int next_history_slot = 0;		  // Index for next command storage
static int stored_commands_count = 0;	  // Total commands in history
static int shell_instance_id;			  // Unique shell identifier
static int condition_evaluation_flag = 0; // For conditional execution

// Function prototypes
void execute_shell_command(char *command_line);
static void secure_string_copy(char *dest, const char *src, int limit);

// Utility functions for code complexity
int is_valid_command_syntax(const char *cmd);
void cleanup_shell_resources(void);
char *strip_surrounding_whitespace(char *str);
int count_character_occurrences(const char *text, char ch);
void print_debug_environment_info(void);

/*
 * Validates command syntax to prevent basic errors
 */
int is_valid_command_syntax(const char *cmd)
{
	if (!cmd || strlen(cmd) == 0)
		return 0;

	int pipe_counter = 0, redirect_counter = 0;
	for (int idx = 0; cmd[idx]; idx++)
	{
		if (cmd[idx] == '|')
			pipe_counter++;
		if (cmd[idx] == '>' || cmd[idx] == '<')
			redirect_counter++;
	}
	return (pipe_counter <= 8 && redirect_counter <= 4);
}

/*
 * Cleanup function for shell termination
 */
void cleanup_shell_resources(void)
{
	// Resource cleanup placeholder
}

/*
 * Removes leading and trailing whitespace
 */
char *strip_surrounding_whitespace(char *str)
{
	if (!str)
		return str;
	while (*str && strchr(WHITESPACE_CHARS, *str))
		str++;
	int length = strlen(str);
	while (length > 0 && strchr(WHITESPACE_CHARS, str[length - 1]))
	{
		str[--length] = '\0';
	}
	return str;
}

/*
 * Counts occurrences of specific character
 */
int count_character_occurrences(const char *text, char ch)
{
	int counter = 0;
	while (*text)
	{
		if (*text == ch)
			counter++;
		text++;
	}
	return counter;
}

/*
 * Debug function for environment information
 */
void print_debug_environment_info(void)
{
	// Debug placeholder for environment state
}

/*
 * Processes command substitution with backticks
 */
int process_backtick_substitution(const char *input_text, char *result_buffer,
								  int result_size, int *consumed_length)
{
	const char *cmd_start = input_text + 1;
	const char *cmd_end = strchr(cmd_start, '`');

	if (cmd_end == NULL)
		return -1;

	int cmd_length = cmd_end - cmd_start;
	char command_to_execute[TEMP_STRING_SIZE];

	if (cmd_length >= (int)sizeof(command_to_execute))
	{
		cmd_length = sizeof(command_to_execute) - 1;
	}

	secure_string_copy(command_to_execute, cmd_start, cmd_length + 1);
	command_to_execute[cmd_length] = '\0';

	int pipe_fds[2];
	if (pipe(pipe_fds) < 0)
		return -1;

	int child_pid = fork();
	if (child_pid < 0)
		return -1;

	if (child_pid == 0)
	{
		// Child: redirect output and execute
		dup(pipe_fds[1], 1);
		close(pipe_fds[0]);
		close(pipe_fds[1]);
		execute_shell_command(command_to_execute);
		exit(1);
	}
	else
	{
		// Parent: read output
		close(pipe_fds[1]);
		int total_read = 0, bytes_read;

		while ((bytes_read = read(pipe_fds[0], result_buffer + total_read, 64)) > 0)
		{
			total_read += bytes_read;
		}

		result_buffer[total_read] = '\0';
		close(pipe_fds[0]);
		wait(child_pid);
	}

	*consumed_length = (cmd_end - input_text) + 1;
	return 0;
}

/*
 * Advanced token parser for command line elements
 */
int extract_next_token(char *input_str, char **token_begin, char **token_finish)
{
	*token_begin = 0;
	*token_finish = 0;

	if (input_str == 0)
		return 0;

	// Skip and nullify whitespace
	while (strchr(WHITESPACE_CHARS, *input_str))
	{
		*input_str++ = 0;
	}

	if (*input_str == 0)
		return 0;

	// Handle append redirection ">>"
	if (*input_str == '>' && *(input_str + 1) == '>')
	{
		*token_begin = input_str;
		input_str[0] = input_str[1] = '\0';
		*token_finish = input_str + 2;
		return '-';
	}

	// Handle logical AND "&&"
	if (*input_str == '&' && *(input_str + 1) == '&')
	{
		*token_begin = input_str;
		input_str[0] = input_str[1] = 0;
		*token_finish = input_str + 2;
		return '*';
	}

	// Handle logical OR "||"
	if (*input_str == '|' && *(input_str + 1) == '|')
	{
		*token_begin = input_str;
		input_str[0] = input_str[1] = 0;
		*token_finish = input_str + 2;
		return '+';
	}

	// Handle single special symbols
	if (strchr(OPERATOR_SYMBOLS, *input_str))
	{
		int symbol_token = *input_str;
		*token_begin = input_str;
		*input_str++ = 0;
		*token_finish = input_str;
		return symbol_token;
	}

	// Handle comments
	if (*input_str == '#')
	{
		*token_begin = input_str;
		while (*input_str && *input_str != '\n')
			input_str++;
		*token_finish = input_str;
		return '#';
	}

	// Parse word tokens with backtick substitution
	*token_begin = input_str;
	while (*input_str && !strchr(WHITESPACE_CHARS OPERATOR_SYMBOLS, *input_str))
	{
		if (*input_str == '`')
		{
			int consumed_chars = 0;
			char substitution_output[64] = {0};

			if (process_backtick_substitution(input_str, substitution_output,
											  sizeof(substitution_output), &consumed_chars) != 0)
			{
				debugf("Syntax error: unmatched backtick.\n");
				return -1;
			}

			char remaining_part[64] = {0};
			strcpy(remaining_part, input_str + consumed_chars);
			strcpy(input_str, substitution_output);
			strcpy(input_str + strlen(substitution_output), remaining_part);
			input_str += strlen(substitution_output);
			continue;
		}
		input_str++;
	}

	*token_finish = input_str;
	return 'w';
}

/*
 * Stateful token getter with internal state management
 */
int get_token_with_state(char *input_str, char **token_ptr)
{
	static int current_tok, next_tok;
	static char *next_start, *next_end;

	if (input_str)
	{
		next_tok = extract_next_token(input_str, &next_start, &next_end);
		return 0;
	}

	current_tok = next_tok;
	*token_ptr = next_start;
	next_tok = extract_next_token(next_end, &next_start, &next_end);
	return current_tok;
}

/*
 * Expands environment variables in command tokens
 */
void expand_environment_variable(const char *token, char *expanded_buffer, int buffer_limit)
{
	if (token[0] != '$')
	{
		// Regular token - copy unchanged
		int copy_idx = 0;
		while (copy_idx < buffer_limit - 1 && token[copy_idx] != '\0')
		{
			expanded_buffer[copy_idx] = token[copy_idx];
			copy_idx++;
		}
		expanded_buffer[copy_idx] = '\0';
		return;
	}

	// Variable token - extract name and expand
	int token_idx = 1, var_name_idx = 0;
	char variable_name[64];

	while (token[token_idx] &&
		   ((token[token_idx] >= 'a' && token[token_idx] <= 'z') ||
			(token[token_idx] >= 'A' && token[token_idx] <= 'Z') ||
			(token[token_idx] == '_') ||
			(token[token_idx] >= '0' && token[token_idx] <= '9')) &&
		   var_name_idx < (int)sizeof(variable_name) - 1)
	{
		variable_name[var_name_idx++] = token[token_idx++];
	}
	variable_name[var_name_idx] = '\0';

	char var_value[128];
	syscall_get_var(variable_name, var_value, sizeof(var_value));

	// Combine variable value with remaining token
	int dest_idx = 0, src_idx = 0;
	while (dest_idx < buffer_limit - 1 && var_value[src_idx] != '\0')
	{
		expanded_buffer[dest_idx++] = var_value[src_idx++];
	}

	src_idx = 0;
	while (dest_idx < buffer_limit - 1 && token[token_idx + src_idx] != '\0')
	{
		expanded_buffer[dest_idx++] = token[token_idx + src_idx++];
	}

	expanded_buffer[dest_idx] = '\0';
}

/*
 * Expands variables in all command arguments
 */
void expand_all_argument_variables(int arg_count, char **arg_array)
{
	for (int i = 0; i < arg_count; i++)
	{
		if (arg_array[i][0] == '$')
		{
			char expanded_arg[128];
			expand_environment_variable(arg_array[i], expanded_arg, sizeof(expanded_arg));
			strcpy(arg_array[i], expanded_arg);
		}
	}
}

/*
 * Stores command in history and saves to persistent file
 */
void store_command_in_history(const char *cmd_text)
{
	// Skip empty or whitespace-only commands
	int has_content = 0;
	for (int i = 0; cmd_text[i]; i++)
	{
		if (!strchr(WHITESPACE_CHARS, cmd_text[i]))
		{
			has_content = 1;
			break;
		}
	}

	if (!has_content)
		return;

	// Store in circular buffer
	strcpy(command_history_buffer[next_history_slot], cmd_text);
	next_history_slot = (next_history_slot + 1) % HISTORY_BUFFER_SIZE;

	if (stored_commands_count < HISTORY_BUFFER_SIZE)
	{
		stored_commands_count++;
	}

	// Write to history file
	int hist_file = open("/.mos_history", O_CREAT | O_WRONLY | O_TRUNC);
	if (hist_file < 0)
		return;

	for (int i = 0; i < stored_commands_count; i++)
	{
		int hist_idx = (next_history_slot - stored_commands_count + i +
						HISTORY_BUFFER_SIZE) %
					   HISTORY_BUFFER_SIZE;

		if (command_history_buffer[hist_idx][0] != '\0')
		{
			write(hist_file, command_history_buffer[hist_idx],
				  strlen(command_history_buffer[hist_idx]));
			write(hist_file, "\n", 1);
		}
	}

	close(hist_file);
}

/*
 * Comprehensive command line parser with I/O redirection support
 */
int parse_command_arguments(char **argv_array, int *right_pipe_id)
{
	int argc = 0;

	while (1)
	{
		char *token;
		int fd, result;
		int token_type = get_token_with_state(0, &token);

		switch (token_type)
		{
		case 0:
			return argc;

		case '#':
			return argc;

		case 'w':
			if (argc >= MAX_ARGUMENTS)
			{
				debugf("too many arguments\n");
				exit(1);
			}
			argv_array[argc++] = token;
			break;

		case '<':
			if (get_token_with_state(0, &token) != 'w')
			{
				debugf("syntax error: < not followed by word\n");
				exit(1);
			}

			fd = open(token, O_RDONLY);
			if (fd < 0)
			{
				debugf("failed to open '%s'\n", token);
				exit(1);
			}
			dup(fd, 0);
			close(fd);
			break;

		case '>':
			if (get_token_with_state(0, &token) != 'w')
			{
				debugf("syntax error: > not followed by word\n");
				exit(1);
			}

			fd = open(token, O_WRONLY | O_CREAT | O_TRUNC);
			if (fd < 0)
			{
				debugf("failed to open '%s'\n", token);
				exit(1);
			}
			dup(fd, 1);
			close(fd);
			break;

		case ';':
			if ((result = fork()) < 0)
			{
				debugf("fork: %d\n", result);
				exit(1);
			}

			*right_pipe_id = result;
			if (result == 0)
			{
				return argc;
			}
			else
			{
				wait(result);
				return parse_command_arguments(argv_array, right_pipe_id);
			}
			break;

		case '-': // Append redirection
			if (get_token_with_state(0, &token) != 'w')
			{
				debugf("syntax error: >> not followed by word\n");
				exit(1);
			}

			if ((fd = open(token, O_WRONLY | O_CREAT)) < 0)
			{
				debugf("open %s for append: %d\n", token, fd);
				exit(1);
			}

			struct Fd *fd_struct;
			if ((result = fd_lookup(fd, &fd_struct)) < 0)
			{
				debugf("fd_lookup error for fd %d: %d\n", fd, result);
				exit(1);
			}

			struct Filefd *file_fd = (struct Filefd *)fd_struct;
			fd_struct->fd_offset = file_fd->f_file.f_size;

			dup(fd, 1);
			close(fd);
			break;

		case '*': // Logical AND
		{
			int and_pid = fork();
			if (and_pid < 0)
				user_panic("fork for && failed");

			if (and_pid == 0)
			{
				return argc;
			}
			else
			{
				int status = wait(and_pid);
				if (status == 0)
				{
					return parse_command_arguments(argv_array, right_pipe_id);
				}
				else
				{
					return 0;
				}
			}
		}
		break;

		case '+': // Logical OR
		{
			int or_pid = fork();
			if (or_pid < 0)
				user_panic("fork for || failed");

			if (or_pid == 0)
			{
				return argc;
			}
			else
			{
				int status = wait(or_pid);
				if (status != 0)
				{
					return parse_command_arguments(argv_array, right_pipe_id);
				}
				else
				{
					return 0;
				}
			}
		}
		break;

		case '|': // Pipe
		{
			int pipe_fds[2];

			if ((result = pipe(pipe_fds)) != 0)
			{
				debugf("pipe: %d\n", result);
				exit(1);
			}

			if ((result = fork()) < 0)
			{
				debugf("fork: %d\n", result);
				exit(1);
			}

			*right_pipe_id = result;

			if (result == 0)
			{
				dup(pipe_fds[0], 0);
				close(pipe_fds[0]);
				close(pipe_fds[1]);
				return parse_command_arguments(argv_array, right_pipe_id);
			}
			else
			{
				dup(pipe_fds[1], 1);
				close(pipe_fds[1]);
				close(pipe_fds[0]);
				return argc;
			}
		}
		break;
		}
	}

	return argc;
}

/*
 * Built-in cd command implementation
 */
int builtin_change_directory(int arg_count, char **arg_vector)
{
	int status;

	if (arg_count == 1)
	{
		if ((status = chdir("/")) < 0)
		{
			printf("cd failed: %d\n", status);
			return 0;
		}
		return 0;
	}

	if (arg_count > 2)
	{
		printf("Too many args for cd command\n");
		return 1;
	}

	struct Stat file_stat;

	if (arg_vector[1][0] == '/')
	{
		// Absolute path
		if ((status = stat(arg_vector[1], &file_stat)) < 0)
		{
			printf("cd: The directory '%s' does not exist\n", arg_vector[1]);
			return 1;
		}

		if (!file_stat.st_isdir)
		{
			printf("cd: '%s' is not a directory\n", arg_vector[1]);
			return 1;
		}

		if ((status = chdir(arg_vector[1])) < 0)
		{
			printf("cd failed: %d\n", status);
			return 1;
		}
	}
	else
	{
		// Relative path
		char working_path[128];

		if ((status = getcwd(working_path)) < 0)
		{
			printf("cd failed: %d\n", status);
			return 1;
		}

		// Handle ".." parent directory
		if (arg_vector[1][0] == '.' && arg_vector[1][1] == '.')
		{
			int path_len = strlen(working_path);

			while (path_len > 1 && working_path[path_len - 1] != '/')
			{
				working_path[path_len - 1] = '\0';
				path_len--;
			}

			if (path_len > 1)
			{
				working_path[path_len - 1] = '\0';
			}

			if (strlen(arg_vector[1]) > 3)
			{
				pathcat(working_path, arg_vector[1] + 3);
			}
		}
		else
		{
			pathcat(working_path, arg_vector[1]);
		}

		if ((status = open(working_path, O_RDONLY)) < 0)
		{
			printf("cd: The directory '%s' does not exist\n", arg_vector[1]);
			return 1;
		}
		close(status);

		if ((status = stat(working_path, &file_stat)) < 0)
		{
			printf("cd: The directory '%s' does not exist\n", arg_vector[1]);
			return 1;
		}

		if (!file_stat.st_isdir)
		{
			printf("cd: '%s' is not a directory\n", arg_vector[1]);
			return 1;
		}

		if ((status = chdir(working_path)) < 0)
		{
			printf("cd failed: %d\n", status);
			return 1;
		}
	}

	return 0;
}

/*
 * Built-in pwd command implementation
 */
int builtin_print_working_directory(int arg_count)
{
	if (arg_count != 1)
	{
		printf("pwd: expected 0 arguments; got %d\n", arg_count);
		return 2;
	}

	char current_dir[128];
	int status;

	if ((status = getcwd(current_dir)) < 0)
	{
		printf("pwd failed: %d\n", status);
		return 2;
	}

	printf("%s\n", current_dir);
	return 0;
}

/*
 * Safe string copying utility
 */
static void secure_string_copy(char *dest, const char *src, int max_chars)
{
	int idx = 0;
	while (idx < max_chars - 1 && src[idx] != '\0')
	{
		dest[idx] = src[idx];
		idx++;
	}
	dest[idx] = '\0';
}

/*
 * Built-in declare command for environment variables
 */
int builtin_declare_variable(int arg_count, char **arg_vector, int shell_id)
{
	int readonly_perm = 0;
	int export_global = 0;
	int current_arg = 1;

	// Parse flags
	while (current_arg < arg_count && arg_vector[current_arg][0] == '-' &&
		   arg_vector[current_arg][1] != '\0')
	{

		for (int flag_idx = 1; arg_vector[current_arg][flag_idx] != '\0'; flag_idx++)
		{
			if (arg_vector[current_arg][flag_idx] == 'r')
			{
				readonly_perm = 1;
			}
			else if (arg_vector[current_arg][flag_idx] == 'x')
			{
				export_global = 1;
			}
			else
			{
				printf("declare: unknown flag -%c\n", arg_vector[current_arg][flag_idx]);
				return 1;
			}
		}
		current_arg++;
	}

	if (current_arg >= arg_count)
	{
		// Display all variables
		char all_vars[256];
		syscall_get_all_var(all_vars, sizeof(all_vars));
		printf("%s", all_vars);
		return 0;
	}

	// Parse variable assignment
	char *assignment = arg_vector[current_arg];
	char var_name[MAX_VAR_NAME + 1];
	char var_value[MAX_VAR_VALUE + 1];
	var_value[0] = '\0';

	const char *equal_sign = strchr(assignment, '=');
	if (equal_sign)
	{
		int name_len = equal_sign - assignment + 1;
		if (name_len > MAX_VAR_NAME)
			name_len = MAX_VAR_NAME;

		secure_string_copy(var_name, assignment, name_len);
		secure_string_copy(var_value, equal_sign + 1, MAX_VAR_VALUE);
	}
	else
	{
		secure_string_copy(var_name, assignment, MAX_VAR_NAME);
	}

	int caller = export_global ? 0 : shell_id;
	int result = syscall_declare_var(var_name, var_value, readonly_perm, caller);

	if (result != 0)
	{
		printf("declare: failed to declare variable %s\n", var_name);
		return 1;
	}

	return 0;
}

/*
 * Built-in unset command for environment variables
 */
int builtin_unset_variable(int arg_count, char **arg_vector, int shell_id)
{
	if (arg_count < 2)
	{
		printf("unset: missing variable name\n");
		return 1;
	}

	int result = syscall_unset_var(arg_vector[1], shell_id);
	if (result != 0)
	{
		printf("unset: failed to remove variable %s\n", arg_vector[1]);
		return 1;
	}

	return 0;
}

/*
 * Built-in history command implementation
 */
void builtin_show_history()
{
	int hist_fd = open("/.mos_history", O_RDONLY);
	if (hist_fd < 0)
	{
		debugf("cannot open /.mos_history\n");
		return;
	}

	char hist_content[4096];
	int bytes_read = read(hist_fd, hist_content, sizeof(hist_content) - 1);

	if (bytes_read >= 0)
	{
		hist_content[bytes_read] = '\0';
		printf("%s", hist_content);
	}

	close(hist_fd);
}

/*
 * Main command execution function
 */
void execute_shell_command(char *command_line)
{
	get_token_with_state(command_line, 0);

	char *command_argv[MAX_ARGUMENTS];
	int right_pipe_process = 0;
	int operation_result;
	int command_exit_status = 0;

	int argument_count = parse_command_arguments(command_argv, &right_pipe_process);
	if (argument_count == 0)
	{
		exit(0);
		return;
	}

	command_argv[argument_count] = 0;

	// Expand environment variables
	expand_all_argument_variables(argument_count, command_argv);

	// Handle built-in commands
	if (argument_count > 0)
	{
		if (strcmp(command_argv[0], "cd") == 0)
		{
			operation_result = builtin_change_directory(argument_count, command_argv);
			return;
		}
		else if (strcmp(command_argv[0], "pwd") == 0)
		{
			operation_result = builtin_print_working_directory(argument_count);
			return;
		}
		else if (strcmp(command_argv[0], "declare") == 0)
		{
			operation_result = builtin_declare_variable(argument_count, command_argv, shell_instance_id);
			return;
		}
		else if (strcmp(command_argv[0], "unset") == 0)
		{
			operation_result = builtin_unset_variable(argument_count, command_argv, shell_instance_id);
			return;
		}
		else if (strcmp(command_argv[0], "history") == 0)
		{
			builtin_show_history();
			return;
		}
	}

	// Execute external command
	int spawned_process = spawn(command_argv[0], command_argv);

	if (spawned_process < 0)
	{
		// Try with .b extension
		char extended_command[256];
		int cmd_length = strlen(command_argv[0]);
		strcpy(extended_command, command_argv[0]);

		if (cmd_length >= 2 && extended_command[cmd_length - 2] == '.' &&
			extended_command[cmd_length - 1] == 'b')
		{
			extended_command[cmd_length - 2] = '\0';
		}
		else
		{
			extended_command[cmd_length] = '.';
			extended_command[cmd_length + 1] = 'b';
			extended_command[cmd_length + 2] = '\0';
		}

		spawned_process = spawn(extended_command, command_argv);
	}

	close_all();

	if (spawned_process >= 0)
	{
		command_exit_status = wait(spawned_process);
	}
	else
	{
		debugf("spawn %s: %d\n", command_argv[0], spawned_process);
		command_exit_status = 1;
	}

	if (right_pipe_process)
	{
		wait(right_pipe_process);
	}

	exit(command_exit_status);
}

/*
 * Enhanced memory move implementation
 */
void *enhanced_memory_move(void *dest_ptr, const void *src_ptr, size_t num_bytes)
{
	char *destination = dest_ptr;
	const char *source = src_ptr;

	if (destination == source)
		return destination;

	if (destination < source)
	{
		for (size_t i = 0; i < num_bytes; i++)
		{
			destination[i] = source[i];
		}
	}
	else
	{
		for (size_t i = num_bytes; i > 0; i--)
		{
			destination[i - 1] = source[i - 1];
		}
	}

	return dest_ptr;
}

/*
 * Advanced line display refresh with cursor management
 */
void refresh_command_line_display(const char *prompt_str, char *line_buffer,
								  int line_length, int cursor_index)
{
	printf("\r");
	printf("%s", prompt_str);
	printf("%s", line_buffer);
	printf("\x1b[K");
	printf("\r\x1b[%dC", (int)strlen(prompt_str) + cursor_index);
}

/*
 * Advanced interactive command line reader with full editing support
 */
void read_interactive_command_line(char *cmd_buffer, u_int max_buffer_size)
{
	int cursor_position = 0;
	int line_content_length = 0;
	int history_browse_index = -1;
	char preserved_input[LARGE_BUFFER_SIZE] = {0};
	char input_character;
	const char *command_prompt = "$ ";

	cmd_buffer[0] = '\0';

	while (1)
	{
		int read_status = read(0, &input_character, 1);
		if (read_status < 1)
			exit(1);

		switch (input_character)
		{
		case '\r':
		case '\n':
			cmd_buffer[line_content_length] = '\0';
			printf("\n");
			return;

		case 1: // Ctrl-A
			if (cursor_position > 0)
			{
				cursor_position = 0;
				refresh_command_line_display(command_prompt, cmd_buffer,
											 line_content_length, cursor_position);
			}
			break;

		case 5: // Ctrl-E
			if (cursor_position < line_content_length)
			{
				cursor_position = line_content_length;
				refresh_command_line_display(command_prompt, cmd_buffer,
											 line_content_length, cursor_position);
			}
			break;

		case 11: // Ctrl-K
			if (cursor_position < line_content_length)
			{
				cmd_buffer[cursor_position] = '\0';
				line_content_length = cursor_position;
				refresh_command_line_display(command_prompt, cmd_buffer,
											 line_content_length, cursor_position);
			}
			break;

		case 21: // Ctrl-U
			if (cursor_position > 0)
			{
				enhanced_memory_move(&cmd_buffer[0], &cmd_buffer[cursor_position],
									 line_content_length - cursor_position + 1);
				line_content_length -= cursor_position;
				cursor_position = 0;
				refresh_command_line_display(command_prompt, cmd_buffer,
											 line_content_length, cursor_position);
			}
			break;

		case 23: // Ctrl-W
		{
			if (cursor_position > 0)
			{
				int word_end_pos = cursor_position;
				int word_start_pos = cursor_position;

				while (word_start_pos > 0 && strchr(WHITESPACE_CHARS, cmd_buffer[word_start_pos - 1]))
				{
					word_start_pos--;
				}

				while (word_start_pos > 0 && !strchr(WHITESPACE_CHARS, cmd_buffer[word_start_pos - 1]))
				{
					word_start_pos--;
				}

				int chars_deleted = cursor_position - word_start_pos;
				enhanced_memory_move(&cmd_buffer[word_start_pos], &cmd_buffer[word_end_pos],
									 line_content_length - word_end_pos + 1);
				line_content_length -= chars_deleted;
				cursor_position = word_start_pos;
				refresh_command_line_display(command_prompt, cmd_buffer,
											 line_content_length, cursor_position);
			}
		}
		break;

		case 127:
		case '\b': // Backspace
			if (cursor_position > 0)
			{
				enhanced_memory_move(&cmd_buffer[cursor_position - 1], &cmd_buffer[cursor_position],
									 line_content_length - cursor_position + 1);
				cursor_position--;
				line_content_length--;
				refresh_command_line_display(command_prompt, cmd_buffer,
											 line_content_length, cursor_position);
			}
			break;

		case 27: // ESC sequences
		{
			char seq_chars[3];
			if (read(0, &seq_chars[0], 1) != 1)
				continue;
			if (read(0, &seq_chars[1], 1) != 1)
				continue;

			if (seq_chars[0] == '[')
			{
				if (history_browse_index == -1)
				{
					strcpy(preserved_input, cmd_buffer);
				}

				switch (seq_chars[1])
				{
				case 'A': // Up arrow
					printf("\x1b[B");

					if (stored_commands_count == 0)
						continue;

					int oldest_cmd_index = (next_history_slot - stored_commands_count +
											HISTORY_BUFFER_SIZE) %
										   HISTORY_BUFFER_SIZE;

					if (history_browse_index == -1)
					{
						history_browse_index = (next_history_slot + HISTORY_BUFFER_SIZE - 1) %
											   HISTORY_BUFFER_SIZE;
					}
					else if (history_browse_index != oldest_cmd_index)
					{
						history_browse_index = (history_browse_index + HISTORY_BUFFER_SIZE - 1) %
											   HISTORY_BUFFER_SIZE;
					}

					strcpy(cmd_buffer, command_history_buffer[history_browse_index]);
					line_content_length = strlen(cmd_buffer);
					cursor_position = line_content_length;
					refresh_command_line_display(command_prompt, cmd_buffer,
												 line_content_length, cursor_position);
					break;

				case 'B': // Down arrow
					if (history_browse_index == -1)
						continue;

					if (history_browse_index == (next_history_slot + HISTORY_BUFFER_SIZE - 1) %
													HISTORY_BUFFER_SIZE)
					{
						history_browse_index = -1;
						strcpy(cmd_buffer, preserved_input);
					}
					else
					{
						history_browse_index = (history_browse_index + 1) % HISTORY_BUFFER_SIZE;

						while (command_history_buffer[history_browse_index][0] == '\0' &&
							   history_browse_index != (next_history_slot + HISTORY_BUFFER_SIZE - 1) %
														   HISTORY_BUFFER_SIZE)
						{
							history_browse_index = (history_browse_index + 1) % HISTORY_BUFFER_SIZE;
						}

						strcpy(cmd_buffer, command_history_buffer[history_browse_index]);
					}

					line_content_length = strlen(cmd_buffer);
					cursor_position = line_content_length;
					refresh_command_line_display(command_prompt, cmd_buffer,
												 line_content_length, cursor_position);
					break;

				case 'C': // Right arrow
					if (cursor_position < line_content_length)
					{
						cursor_position++;
					}
					else
					{
						printf("\x1b[D");
					}
					break;

				case 'D': // Left arrow
					if (cursor_position > 0)
					{
						cursor_position--;
					}
					else
					{
						printf("\x1b[C");
					}
					break;
				}
			}
		}
		break;

		default: // Printable characters
			if (input_character >= 32 && input_character < 127)
			{
				if (line_content_length < max_buffer_size - 1)
				{
					enhanced_memory_move(&cmd_buffer[cursor_position + 1], &cmd_buffer[cursor_position],
										 line_content_length - cursor_position + 1);
					cmd_buffer[cursor_position] = input_character;
					line_content_length++;
					cursor_position++;
					refresh_command_line_display(command_prompt, cmd_buffer,
												 line_content_length, cursor_position);
				}
			}
			break;
		}
	}
}

// Main command buffer
char primary_command_buffer[LARGE_BUFFER_SIZE];

/*
 * Display shell usage information
 */
void show_shell_usage(void)
{
	printf("usage: sh [-ix] [script-file]\n");
	exit(1);
}

/*
 * Check if command starts with specified prefix
 */
int command_starts_with(const char *command_str, const char *prefix_str)
{
	while (*prefix_str)
	{
		if (*command_str != *prefix_str)
			return 0;
		command_str++;
		prefix_str++;
	}
	return (*command_str == '\0' || strchr(WHITESPACE_CHARS, *command_str) != 0);
}

/*
 * Main shell program entry point
 */
int main(int argc, char **argv)
{
	int operation_result;
	int interactive_session = iscons(0);
	int echo_command_mode = 0;

	printf("\n\033[38;2;255;100;100m:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\033[0m\n");
	printf("\033[38;2;255;100;100m::                                                           ::\033[0m\n");
	printf("\033[38;2;255;100;100m::                                                           ::\033[0m\n");
	printf("\033[38;2;255;100;100m::     \033[1m\033[3m\033[38;2;255;50;50m███╗   ███╗ \033[38;2;255;100;50m██████╗ \033[38;2;255;150;50m███████╗\033[0m      \033[38;2;255;100;100m::\033[0m\n");
	printf("\033[38;2;255;100;100m::     \033[1m\033[3m\033[38;2;255;100;50m████╗ ████║\033[38;2;255;150;50m██╔═══██╗\033[38;2;255;200;50m██╔════╝\033[0m      \033[38;2;255;100;100m::\033[0m\n");
	printf("\033[38;2;255;100;100m::     \033[1m\033[3m\033[38;2;255;150;50m██╔████╔██║\033[38;2;255;200;50m██║   ██║\033[38;2;255;255;50m███████╗\033[0m      \033[38;2;255;100;100m::\033[0m\n");
	printf("\033[38;2;255;100;100m::     \033[1m\033[3m\033[38;2;255;200;50m██║╚██╔╝██║\033[38;2;200;255;50m██║   ██║\033[38;2;150;255;50m╚════██║\033[0m      \033[38;2;255;100;100m::\033[0m\n");
	printf("\033[38;2;255;100;100m::     \033[1m\033[3m\033[38;2;150;255;50m██║ ╚═╝ ██║\033[38;2;100;255;50m╚██████╔╝\033[38;2;50;255;50m███████║\033[0m      \033[38;2;255;100;100m::\033[0m\n");
	printf("\033[38;2;255;100;100m::     \033[1m\033[3m\033[38;2;100;255;50m╚═╝     ╚═╝ \033[38;2;50;255;100m╚═════╝ \033[38;2;50;255;150m╚══════╝\033[0m      \033[38;2;255;100;100m::\033[0m\n");
	printf("\033[38;2;255;100;100m::                                                           ::\033[0m\n");
	printf("\033[38;2;0;100;100m::   \033[1m\033[38;2;0;150;150m███████╗   \033[38;2;0;170;170m██████╗  \033[38;2;0;190;190m  ███████╗   \033[38;2;0;210;210m ███████╗  \033[38;2;0;100;100m::\033[0m\n");
	printf("\033[38;2;0;100;100m::   \033[1m\033[38;2;0;160;160m╚══██╔═╝  \033[38;2;0;180;180m██╔═══██╗\033[38;2;0;200;200m  ╚════██╝   \033[38;2;0;220;220m  ██╔════╝   \033[38;2;0;100;100m::\033[0m\n");
	printf("\033[38;2;0;100;100m::     \033[1m\033[38;2;0;170;170m███╔╝    \033[38;2;0;190;190m██║   ██║\033[38;2;0;210;210m    ████     \033[38;2;0;230;230m  ███████╗  \033[38;2;0;100;100m::\033[0m\n");
	printf("\033[38;2;0;100;100m::    \033[1m\033[38;2;0;180;180m███╔╝     \033[38;2;0;200;200m██║   ██║\033[38;2;0;220;220m  ███╔      \033[38;2;0;240;240m  ╚════██║   \033[38;2;0;100;100m::\033[0m\n");
	printf("\033[38;2;0;100;100m::   \033[1m\033[38;2;0;190;190m███████╗   \033[38;2;0;210;210m╚██████╔╝\033[38;2;0;230;230m  ███████╗   \033[38;2;0;250;250m████████║   \033[0m\n");
	printf("\033[38;2;0;100;100m::   \033[1m\033[38;2;0;200;200m╚══════╝   \033[38;2;0;220;220m╚═════╝  \033[38;2;0;240;240m  ╚══════╝   \033[38;2;0;255;255m╚══════ ╝   \033[0m\n");
	printf("\033[38;2;0;100;100m::                                                           ::\033[0m\n");
	printf("\033[38;2;0;100;100m:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\033[0m\n");

	// Process command line arguments
	ARGBEGIN
	{
	case 'i':
		interactive_session = 1;
		break;
	case 'x':
		echo_command_mode = 1;
		break;
	default:
		show_shell_usage();
	}
	ARGEND

	if (argc > 1)
	{
		show_shell_usage();
	}

	if (argc == 1)
	{
		close(0);
		if ((operation_result = open(argv[0], O_RDONLY)) < 0)
		{
			user_panic("open %s: %d", argv[0], operation_result);
		}
		if ((operation_result = chdir("/")) < 0)
		{
			printf("created root path failed: %d\n", operation_result);
		}
		user_assert(operation_result == 0);
	}

	// Initialize shell with unique identifier
	shell_instance_id = syscall_alloc_shell_id();

	// Main interactive shell loop
	for (;;)
	{
		if (interactive_session)
		{
			printf("\n\033[32m@pzh shell\033[0m\n\033[34m>  \033[0m");
			printf("\n\033[32m@pzh shell\033[0m\n\033[34m>  \033[0m");
		}

		read_interactive_command_line(primary_command_buffer, sizeof primary_command_buffer);
		store_command_in_history(primary_command_buffer);

		// Handle comments and empty lines
		if (primary_command_buffer[0] == '#')
		{
			continue;
		}

		if (echo_command_mode)
		{
			printf("# %s\n", primary_command_buffer);
		}

		// Handle exit command
		if (command_starts_with(primary_command_buffer, "exit"))
		{
			exit(0);
			break;
		}

		// Handle built-in commands that need direct execution
		if (command_starts_with(primary_command_buffer, "cd") ||
			command_starts_with(primary_command_buffer, "pwd") ||
			command_starts_with(primary_command_buffer, "declare") ||
			command_starts_with(primary_command_buffer, "unset") ||
			command_starts_with(primary_command_buffer, "history"))
		{
			execute_shell_command(primary_command_buffer);
			continue;
		}

		// Fork and execute external commands
		if ((operation_result = fork()) < 0)
		{
			user_panic("fork: %d", operation_result);
		}

		if (operation_result == 0)
		{
			execute_shell_command(primary_command_buffer);
		}
		else
		{
			wait(operation_result);
		}
	}

	return 0;
}