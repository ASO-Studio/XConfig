#ifndef _CPARSE_CORE_H
#define _CPARSE_CORE_H

#if !defined( _XCONFIG_H )
# error Never use "cparse_core.h", use "xconfig.h" instead
#endif // _XCONFIG_H

#if !defined(_STDIO_H)
# include <stdio.h>
#endif // _STDIO_H

#include <sys/types.h> // For off_t

#if !defined(NO_TRACE)
# define TRACE(...) do { \
			fprintf(stderr, "\033[32;1m" "Trace" "\033[0m" ": '%s', '%s': ", __FILE__, __func__); \
			fprintf(stderr, __VA_ARGS__); fflush(stderr); \
		} while(0)
#else // NO_TRACE
# define TRACE(...)
#endif // NO_TRACE

#define MAX_ERRBUF 512
#define INITIAL_BUFFER_SIZE 64
#define BUFFER_GROWTH_FACTOR 2

#if !defined(__CPState_defined)
typedef struct
{
	enum
	{
		P_FD,
		P_STR
	} type;

	char *str;
	int fd;
	off_t off;
} CPState;
#define __CPState_defined
#endif /* __CPState_defined */

typedef struct ConfigEntry ConfigEntry;
typedef struct ConfigSection ConfigSection;
typedef struct Config Config;

struct ConfigEntry
{
	char *key;
	char *value;
	ConfigEntry *next;
};

struct ConfigSection
{
	char *name;
	ConfigEntry *entries;
	ConfigSection *next;
};

struct Config
{
	ConfigSection *sections;
	ConfigSection *current_section;
	size_t entry_count;
	size_t section_count;
};

/* Initialize parser state */
void cparse_init(CPState *state, int from, int fd, const char *str);

/* Add a new section to configuration*/
ConfigSection *config_add_section(Config *config, const char *name);

/* Add key-value pair to current section */
int config_add_entry(Config *config, const char *key, const char *value);

/* Main configuration parsing function */
Config *cparse_load(CPState *state);

/* Read value from specified section and key. Returns NULL if not found */
const char *cparse_read(const Config *config, const char *section, const char *key);

/* Clean up parser resources */
void cparse_cleanup(CPState *st);

/* Free configuration memory */
void cparse_free(Config *config);

/* Get error message */
const char *cparse_get_error(void);

/* Set error message */
void cparse_set_error(CPState *st, const char *msg, ...);

#endif
