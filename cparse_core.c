#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#define _XCONFIG_H
#include "cparse_core.h"

static char glb_err_buf[MAX_ERRBUF];

// ==================== Utility Functions ====================

/**
 * Safe string duplication with error checking
 */
static char *dynamic_strdup(const char *str)
{
	if (!str) return NULL;
	
	char *new_str = malloc(strlen(str) + 1);
	if (!new_str) {
		return NULL;
	}
	strcpy(new_str, str);
	return new_str;
}

/**
 * Safe string duplication with length limit
 */
static char *dynamic_strndup(const char *str, size_t len)
{
	if (!str) return NULL;
	
	size_t actual_len = strlen(str);
	if (actual_len > len) {
		actual_len = len;
	}
	
	char *new_str = malloc(actual_len + 1);
	if (!new_str) {
		return NULL;
	}
	
	strncpy(new_str, str, actual_len);
	new_str[actual_len] = '\0';
	return new_str;
}

/**
 * Dynamic string concatenation with automatic buffer growth
 */
static int dynamic_strcat(char **dest, size_t *dest_size, const char *src)
{
	if (!dest || !src) return 0;
	
	size_t src_len = strlen(src);
	size_t dest_len = *dest ? strlen(*dest) : 0;
	size_t needed_size = dest_len + src_len + 1;
	
	if (needed_size > *dest_size) {
		size_t new_size = needed_size * BUFFER_GROWTH_FACTOR;
		char *new_dest = realloc(*dest, new_size);
		if (!new_dest) {
			return 0;
		}
		*dest = new_dest;
		*dest_size = new_size;
	}
	
	strcat(*dest + dest_len, src);
	return 1;
}

// ==================== Configuration Management ====================

/**
 * Initialize configuration structure
 */
static void config_init(Config *config)
{
	if (!config) return;
	
	memset(config, 0, sizeof(Config));
}

/**
 * Add a new section to configuration
 */
ConfigSection *config_add_section(Config *config, const char *name)
{
	if (!config || !name) {
		return NULL;
	}
	
	ConfigSection *section = malloc(sizeof(ConfigSection));
	if (!section) {
		return NULL;
	}
	
	memset(section, 0, sizeof(ConfigSection));
	section->name = dynamic_strdup(name);
	if (!section->name) {
		free(section);
		return NULL;
	}
	
	/* Add to linked list */
	if (!config->sections) {
		config->sections = section;
	} else {
		ConfigSection *last = config->sections;
		while (last->next) {
			last = last->next;
		}
		last->next = section;
	}
	
	config->current_section = section;
	config->section_count++;
	
	return section;
}

/**
 * Add key-value pair to current section
 */
int config_add_entry(Config *config, const char *key, const char *value)
{
	if (!config || !key || !value || !config->current_section) {
		return 0;
	}
	
	ConfigEntry *entry = malloc(sizeof(ConfigEntry));
	if (!entry) {
		return 0;
	}
	
	memset(entry, 0, sizeof(ConfigEntry));
	
	entry->key = dynamic_strdup(key);
	entry->value = dynamic_strdup(value);
	
	if (!entry->key || !entry->value) {
		if (entry->key) free(entry->key);
		if (entry->value) free(entry->value);
		free(entry);
		return 0;
	}
	
	/* Add to linked list */
	if (!config->current_section->entries) {
		config->current_section->entries = entry;
	} else {
		ConfigEntry *last = config->current_section->entries;
		while (last->next) {
			last = last->next;
		}
		last->next = entry;
	}
	
	config->entry_count++;
	return 1;
}

/**
 * Free configuration memory
 */
static void config_free(Config *config)
{
	if (!config) return;
	
	ConfigSection *section = config->sections;
	while (section) {
		ConfigEntry *entry = section->entries;
		while (entry) {
			ConfigEntry *next_entry = entry->next;
			if (entry->key) free(entry->key);
			if (entry->value) free(entry->value);
			free(entry);
			entry = next_entry;
		}
		
		ConfigSection *next_section = section->next;
		if (section->name) free(section->name);
		free(section);
		section = next_section;
	}
	
	memset(config, 0, sizeof(Config));
}

// ==================== Error Handling ====================

/**
 * Set error message
 */
void cparse_set_error(CPState *st, const char *str, ...)
{
	if (!str) return;
	(void)st;
	
	va_list args;
	va_start(args, str);
	vsnprintf(glb_err_buf, MAX_ERRBUF, str, args);
	va_end(args);
}

// ==================== Parser Initialization ====================

/**
 * Initialize parser state
 */
void cparse_init(CPState *st, int from, int fd, const char *str)
{
	if (!st) return;
	
	memset(glb_err_buf, 0, MAX_ERRBUF);
	
	if (from == P_FD) {
		if (fd < 0) {
			cparse_set_error(st, "Invalid file descriptor: %d", fd);
			return;
		}
		st->fd = fd;
	} else if (from == P_STR) {
		if (!str) {
			cparse_set_error(st, "Invalid string: NULL");
			return;
		}
		st->str = dynamic_strdup(str);
		if (!st->str) {
			cparse_set_error(st, "Failed to allocate memory");
			return;
		}
	} else {
		cparse_set_error(st, "Invalid source: %d", from);
		return;
	}
	
	st->type = from;
	st->off = 0;
}

// ==================== Character Input ====================

/**
 * Get next character from input
 */
static int cparse_getc(CPState *st)
{
	if (!st) return EOF;
	
	int ch = EOF;
	
	if (st->type == P_FD) {
		char buf[1];
		if (read(st->fd, buf, 1) <= 0) {
			return EOF;
		}
		st->off++;
		ch = buf[0];
	} else if (st->type == P_STR) {
		if (st->off >= strlen(st->str)) {
			return EOF;
		}
		ch = st->str[st->off++];
	} else {
		cparse_set_error(st, "Invalid type: %d", st->type);
		ch = -2;
	}
	
	return ch;
}

/**
 * Peek at next character without consuming it
 */
static int cparse_peek(CPState *st)
{
	if (!st) return EOF;
	
	off_t saved_off = st->off;
	int ch = cparse_getc(st);
	st->off = saved_off;
	return ch;
}

/**
 * Clean up parser resources
 */
void cparse_cleanup(CPState *st)
{
	if (st && st->type == P_STR && st->str) {
		free(st->str);
		st->str = NULL;
	}
}

// ==================== Parser Utilities ====================

/**
 * Skip whitespace characters
 */
static void skip_whitespace(CPState *st)
{
	if (!st) return;
	
	int ch;
	while ((ch = cparse_peek(st)) != EOF) {
		if (isspace(ch) && ch != '\n') {
			cparse_getc(st); /* Consume whitespace */
		} else {
			break;
		}
	}
}

/**
 * Skip comment lines
 */
static int skip_comments(CPState *st)
{
	if (!st) return 0;
	
	skip_whitespace(st);
	
	int ch = cparse_peek(st);
	if (ch == '\n') {
		cparse_getc(st); /* Skip empty line */
		return 1;
	}
	
	if (ch == '#' || ch == ';') { /* Comment line */
		while ((ch = cparse_getc(st)) != EOF && ch != '\n') {
			/* Skip entire line */
		}
		return 1;
	}
	
	return 0;
}

// ==================== Value Reading ====================

/**
 * Read simple value (without quotes)
 */
static char *read_simple_value(CPState *st)
{
	if (!st) return NULL;
	
	size_t buffer_size = INITIAL_BUFFER_SIZE;
	char *buffer = malloc(buffer_size);
	if (!buffer) return NULL;
	
	size_t pos = 0;
	int ch;
	
	while ((ch = cparse_peek(st)) != EOF && ch != '\n' && ch != '#' && ch != ';') {
		ch = cparse_getc(st);
		
		/* Resize buffer if needed */
		if (pos + 1 >= buffer_size) {
			buffer_size *= BUFFER_GROWTH_FACTOR;
			char *new_buffer = realloc(buffer, buffer_size);
			if (!new_buffer) {
				free(buffer);
				return NULL;
			}
			buffer = new_buffer;
		}
		
		buffer[pos++] = ch;
	}
	
	/* Trim trailing whitespace */
	while (pos > 0 && isspace(buffer[pos - 1])) {
		pos--;
	}
	
	buffer[pos] = '\0';
	
	/* Shrink buffer to actual size */
	if (pos + 1 < buffer_size) {
		char *shrunk_buffer = realloc(buffer, pos + 1);
		if (shrunk_buffer) {
			buffer = shrunk_buffer;
		}
	}
	
	return buffer;
}

/**
 * Read quoted value with support for multiline and escape sequences
 */
static char *read_quoted_value(CPState *st, char quote_char)
{
	if (!st) return NULL;
	
	cparse_getc(st); /* Consume opening quote */
	
	size_t buffer_size = INITIAL_BUFFER_SIZE;
	char *buffer = malloc(buffer_size);
	if (!buffer) return NULL;
	
	size_t pos = 0;
	int ch;
	int escape = 0;
	
	while ((ch = cparse_getc(st)) != EOF) {
		if (escape) {
			/* Handle escape sequences */
			switch (ch) {
			case 'n': ch = '\n'; break;
			case 't': ch = '\t'; break;
			case 'r': ch = '\r'; break;
			case '\\': ch = '\\'; break;
			case '"': ch = '"'; break;
			case '\'': ch = '\''; break;
			default: break; /* Keep other escaped characters */
			}
			escape = 0;
		} else if (ch == '\\') {
			escape = 1;
			continue;
		} else if (ch == quote_char) {
			break; /* End of quoted value */
		}
		
		/* Resize buffer if needed */
		if (pos + 1 >= buffer_size) {
			buffer_size *= BUFFER_GROWTH_FACTOR;
			char *new_buffer = realloc(buffer, buffer_size);
			if (!new_buffer) {
				free(buffer);
				return NULL;
			}
			buffer = new_buffer;
		}
		
		buffer[pos++] = ch;
		
		/* Handle multiline values */
		if (ch == '\n') {
			skip_whitespace(st);
			int next_ch = cparse_peek(st);
			if (next_ch != quote_char && next_ch != '\n' && next_ch != '#' && next_ch != ';' && next_ch != '[') {
				/* Continue multiline value */
				if (pos + 1 >= buffer_size) {
					buffer_size *= BUFFER_GROWTH_FACTOR;
					char *new_buffer = realloc(buffer, buffer_size);
					if (!new_buffer) {
						free(buffer);
						return NULL;
					}
					buffer = new_buffer;
				}
				buffer[pos++] = ' '; /* Add space between lines */
			}
		}
	}
	
	if (ch != quote_char) {
		cparse_set_error(st, "Unclosed quote");
		free(buffer);
		return NULL;
	}
	
	buffer[pos] = '\0';
	
	/* Shrink buffer to actual size */
	if (pos + 1 < buffer_size) {
		char *shrunk_buffer = realloc(buffer, pos + 1);
		if (shrunk_buffer) {
			buffer = shrunk_buffer;
		}
	}
	
	return buffer;
}

/**
 * Read configuration value
 */
static char *read_value(CPState *st)
{
	if (!st) return NULL;
	
	skip_whitespace(st);
	int ch = cparse_peek(st);
	
	if (ch == '"' || ch == '\'') {
		return read_quoted_value(st, ch);
	} else {
		return read_simple_value(st);
	}
}

// ==================== Key Reading ====================

/**
 * Read key name
 */
static char *read_key(CPState *st)
{
	if (!st) return NULL;
	
	skip_whitespace(st);
	
	size_t buffer_size = INITIAL_BUFFER_SIZE;
	char *buffer = malloc(buffer_size);
	if (!buffer) return NULL;
	
	size_t pos = 0;
	int ch;
	int in_quotes = 0;
	char quote_char = 0;
	
	while ((ch = cparse_peek(st)) != EOF) {
		if (ch == '=' && !in_quotes) break;
		if (isspace(ch) && !in_quotes) break;
		if (ch == '\n') break;
		
		ch = cparse_getc(st);
		
		if ((ch == '"' || ch == '\'') && !in_quotes) {
			in_quotes = 1;
			quote_char = ch;
			continue;
		}
		
		if (ch == quote_char && in_quotes) {
			in_quotes = 0;
			continue;
		}
		
		/* Resize buffer if needed */
		if (pos + 1 >= buffer_size) {
			buffer_size *= BUFFER_GROWTH_FACTOR;
			char *new_buffer = realloc(buffer, buffer_size);
			if (!new_buffer) {
				free(buffer);
				return NULL;
			}
			buffer = new_buffer;
		}
		
		buffer[pos++] = ch;
	}
	
	if (in_quotes) {
		cparse_set_error(st, "Unclosed quotes in key");
		free(buffer);
		return NULL;
	}
	
	buffer[pos] = '\0';
	
	/* Trim whitespace from key */
	while (pos > 0 && isspace(buffer[pos - 1])) {
		buffer[--pos] = '\0';
	}
	
	/* Shrink buffer to actual size */
	if (pos + 1 < buffer_size) {
		char *shrunk_buffer = realloc(buffer, pos + 1);
		if (shrunk_buffer) {
			buffer = shrunk_buffer;
		}
	}
	
	return buffer;
}

// ==================== Section Reading ====================

/**
 * Read section name [section]
 */
static char *read_section(CPState *st)
{
	if (!st) return NULL;
	
	skip_whitespace(st);
	int ch = cparse_getc(st);
	
	if (ch != '[') {
		st->off--; /* Put character back */
		return NULL;
	}
	
	size_t buffer_size = INITIAL_BUFFER_SIZE;
	char *buffer = malloc(buffer_size);
	if (!buffer) return NULL;
	
	size_t pos = 0;
	
	while ((ch = cparse_getc(st)) != EOF && ch != ']' && ch != '\n') {
		/* Resize buffer if needed */
		if (pos + 1 >= buffer_size) {
			buffer_size *= BUFFER_GROWTH_FACTOR;
			char *new_buffer = realloc(buffer, buffer_size);
			if (!new_buffer) {
				free(buffer);
				return NULL;
			}
			buffer = new_buffer;
		}
		
		buffer[pos++] = ch;
	}
	
	if (ch != ']') {
		cparse_set_error(st, "Missing ']' in section header");
		free(buffer);
		return NULL;
	}
	
	buffer[pos] = '\0';
	
	/* Skip to end of line */
	while ((ch = cparse_getc(st)) != EOF && ch != '\n') {
		/* Skip remaining characters */
	}
	
	/* Trim whitespace from section name */
	while (pos > 0 && isspace(buffer[pos - 1])) {
		buffer[--pos] = '\0';
	}
	
	/* Shrink buffer to actual size */
	if (pos + 1 < buffer_size) {
		char *shrunk_buffer = realloc(buffer, pos + 1);
		if (shrunk_buffer) {
			buffer = shrunk_buffer;
		}
	}
	
	return buffer;
}

// ==================== Entry Parsing ====================

/**
 * Parse single configuration entry
 */
static int parse_config_entry(CPState *st, Config *config, int *line_num)
{
	if (!st || !config) return 0;
	
	/* Skip comments and blank lines */
	while (skip_comments(st)) {
		(*line_num)++;
	}
	
	skip_whitespace(st);
	int ch = cparse_peek(st);
	
	if (ch == EOF) return 0;
	if (ch == '[') return 0; /* Section line */
	
	/* Read key */
	char *key = read_key(st);
	if (!key) return 0;
	
	/* Find equals sign */
	skip_whitespace(st);
	ch = cparse_getc(st);
	if (ch != '=') {
		cparse_set_error(st, "Expected '=' after key");
		free(key);
		return 0;
	}
	
	/* Read value */
	char *value = read_value(st);
	if (!value) {
		free(key);
		return 0;
	}
	
	/* Add to configuration */
	if (!config_add_entry(config, key, value)) {
		cparse_set_error(st, "Failed to add configuration entry");
		free(key);
		free(value);
		return 0;
	}

	free(key);
	free(value);

	/* Skip to end of line */
	while ((ch = cparse_peek(st)) != EOF && ch != '\n') {
		cparse_getc(st);
	}
	if (ch == '\n') {
		cparse_getc(st);
		(*line_num)++;
	}

	return 1;
}

// ==================== Main Parser ====================

/**
 * Main configuration parsing function
 */
Config *cparse_load(CPState *st)
{
	if (!st) return NULL;

	Config *config = malloc(sizeof(Config));
	if (!config) {
		cparse_set_error(st, "Failed to allocate configuration memory");
		return NULL;
	}

	config_init(config);

	/* Create default section for entries before any section header */
	if (!config_add_section(config, "")) {
		cparse_set_error(st, "Failed to create default section");
		free(config);
		return NULL;
	}

	int line_num = 1;

	while (1) {
		/* Skip comments and whitespace */
		while (skip_comments(st)) {
			line_num++;
		}

		/* Check for section */
		char *section_name = read_section(st);
		if (section_name) {
			if (!config_add_section(config, section_name)) {
				cparse_set_error(st, "Failed to add section: %s", section_name);
				free(section_name);
				config_free(config);
				free(config);
				return NULL;
			}
			free(section_name);
			continue;
		}

		/* Parse key-value pair */
		if (parse_config_entry(st, config, &line_num)) {
			/* Entry successfully added */
		} else {
			/* Check for end of input */
			int ch = cparse_peek(st);
			if (ch == EOF) break;

			/* Report errors and skip line */
			if (strlen(glb_err_buf) > 0) {
				fprintf(stderr, "Error at line %d: %s\n", line_num, glb_err_buf);
				memset(glb_err_buf, 0, MAX_ERRBUF);
				
				/* Skip erroneous line */
				while ((ch = cparse_getc(st)) != EOF && ch != '\n') {
					/* Skip characters */
				}
				if (ch == '\n') line_num++;
			}
		}
	}

	return config;
}

/**
 * Free configuration memory
 */
void cparse_free(Config *config)
{
	if (config) {
		config_free(config);
		free(config);
	}
}

// ==================== Configuration Query ====================

/**
 * Read value from specified section and key. Returns NULL if not found
 */
const char *cparse_read(const Config *config, const char *section, const char *key)
{
	if (!config || !key) return NULL;

	/* If section is NULL, search in all sections */
	const ConfigSection *current_section = config->sections;

	while (current_section) {
		/* If specific section is requested, skip non-matching sections */
		if (section && (!current_section->name || strcmp(current_section->name, section) != 0)) {
			current_section = current_section->next;
			continue;
		}

		/* Search for key in current section */
		const ConfigEntry *entry = current_section->entries;
		while (entry) {
			if (entry->key && strcmp(entry->key, key) == 0) {
				return entry->value;
			}
			entry = entry->next;
		}

		/* If specific section was requested, we're done after checking it */
		if (section) break;

		current_section = current_section->next;
	}
	
	return NULL; /* Key not found */
}

/**
 * Get error message
 */
const char *cparse_get_error(void)
{
	return glb_err_buf;
}
