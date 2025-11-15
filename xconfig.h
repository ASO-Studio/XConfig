#ifndef _XCONFIG_H
#define _XCONFIG_H

#include <sys/types.h>
#include <stdbool.h>

#if defined (_WIN32) || defined (_WINDOWS)
 #define XC_EXPORT(type) __declspec(dllexport) type
 #define XC_STATIC(type) static type
#else
 #define XC_EXPORT(type) type
 #define XC_STATIC(type) static type
#endif

#if !defined(NO_TRACE)
# define TRACE(...) do { \
			fprintf(stderr, "\033[32;1m" "Trace" "\033[0m" ": '%s', '%s': ", __FILE__, __func__); \
			fprintf(stderr, __VA_ARGS__); fflush(stderr); \
		} while(0)
#else // NO_TRACE
# define TRACE(...)
#endif // NO_TRACE

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

typedef struct Config Config;

typedef struct {
	CPState parser;
	Config *config;
} XConfig;

/* Parse config file */
XC_EXPORT(XConfig *) XConfig_ParseFile(const char *file);

/* Parse config string */
XC_EXPORT(XConfig *) XConfig_ParseString(const char *string);

/* Free memory */
XC_EXPORT(void) XConfig_Delete(XConfig *xc);

/* Read config data */
XC_EXPORT(const char *) XConfig_Read(XConfig *xc, const char *section, const char *key);

/* Convert XConfig pointer to string */
XC_EXPORT(char *) XConfig_Print(XConfig *xc);

/* Get error string */
XC_EXPORT(const char *) XConfig_GetError(void);

/* Have error */
XC_EXPORT(bool) XConfig_HaveError(void);

/* Write config string to file*/
XC_EXPORT(bool) XConfig_WriteFile(XConfig *xc, const char *file);

/* Create a XConfig pointer */
XC_EXPORT(XConfig *) XConfig_Create(void);

/* Add a section */
XC_EXPORT(bool) XConfig_AddSection(XConfig *xc, const char *section);

/* Add a key-value pair */
XC_EXPORT(bool) XConfig_AddKeyValue(XConfig *xc, const char *section,
					const char *key, const char *value);

#endif // _XCONFIG_H
