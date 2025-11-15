#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "xconfig.h"
#include "cparse_core.h"

/* Read file content */
XC_STATIC(char *) ReadFileContent(const char *file)
{
	/* Get file status */
	struct stat st;
	if (stat(file, &st) < 0) {
		return NULL;
	}

	/* Open file */
	int fd = open(file, O_RDONLY);
	if (fd < 0)
	{
		return NULL;
	}

	/* Allocate memory for buffer */
	char *content = malloc(st.st_size);
	if (!content)
	{
		close(fd);
		return NULL;
	}

	/* Read content */
	read(fd, content, st.st_size);
	close(fd);

	TRACE("Content: \n%s\n", content);

	return content;
}

/* Parse config file. */
XC_EXPORT(XConfig *) XConfig_ParseFile(const char *file)
{
	/* Allocate memory for new pointer. */
	XConfig *xc = malloc(sizeof(XConfig));
	if (!xc)
		return NULL;

	/* Read file content. */
	char *content = ReadFileContent(file);
	if (!content)
	{
		free(xc);
		return NULL;
	}

	/* Load configuration. */
	cparse_init(&(xc->parser), P_STR, -1, content);
	xc->config = cparse_load(&(xc->parser));

	/* Now, CONTENT is not neccessary, just free it */
	free(content);

	if (!xc->config)
	{
		cparse_cleanup(&(xc->parser));
		return NULL;
	}

	return xc;
}

/* Parse config string */
XC_EXPORT(XConfig *) XConfig_ParseString(const char *string)
{
	XConfig *xc = malloc(sizeof(XConfig));

	/* Load configuration. */
	cparse_init(&(xc->parser), P_STR, -1, string);
	xc->config = cparse_load(&(xc->parser));

	if (!xc->config)
	{
		cparse_cleanup(&(xc->parser));
		return NULL;
	}

	return xc;
}

/* Free memory. */
XC_EXPORT(void) XConfig_Delete(XConfig *xc)
{
	if (!xc)
		return;

	/* Clean up */
	cparse_free(xc->config);
	cparse_cleanup(&xc->parser);

	free(xc);
}

/* Read config data. */
XC_EXPORT(const char *) XConfig_Read(XConfig *xc, const char *section, const char *key)
{
	return cparse_read(xc->config, section, key);
}

/* Get error string */
XC_EXPORT(const char *) XConfig_GetError(void)
{
	return cparse_get_error();
}

/* Convert XConfig pointer to string. */
#define INITIAL_BUF (1024)
#define CHANGE_SZ() if (used_size > size) \
		{ \
			size *= 2; \
			ret_buf = realloc(ret_buf, size); \
			if (!ret_buf) \
				return NULL; \
		}

XC_EXPORT(char *) XConfig_Print(XConfig *xc)
{
	size_t size = INITIAL_BUF;
	size_t used_size = 0;
	char *ret_buf = malloc(size);
	memset(ret_buf, 0, size);

	ConfigSection *cs = xc->config->sections;
	if (!cs)
		return ret_buf;

	while (cs)
	{
		/* Catenet section name */
		CHANGE_SZ();
		if (strlen(cs->name) <= 0) {
			cs = cs->next;
			continue;
		}
		strcat(ret_buf, "[");
		strcat(ret_buf, cs->name);
		strcat(ret_buf, "]\n");
		used_size += strlen(cs->name) + 2;

		CHANGE_SZ();

		/* Catenet Keys */
		ConfigEntry *ce = cs->entries;
		while (ce)
		{
			CHANGE_SZ();
			strcat(ret_buf, ce->key);
			strcat(ret_buf, " = \"");
			strcat(ret_buf, ce->value);
			strcat(ret_buf, "\"\n");
			used_size += strlen(ce->value) + strlen(ce->key) + 6;
			ce = ce->next;
		}
		strcat(ret_buf, "\n");
		used_size++;
		cs = cs->next;
	}

	/* Remove last '\n' */
	ret_buf[strlen(ret_buf) - 1] = '\0';

	return ret_buf;
}

#undef CHANGE_SZ
#undef INITIAL_BUF

/* Have error */
XC_EXPORT(bool) XConfig_HaveError(void)
{
	return (strlen(XConfig_GetError()) > 0);
}

/* Write config string to file */
XC_EXPORT(bool) XConfig_WriteFile(XConfig *xc, const char *file)
{
	char *cstr = NULL;
	int fd = -1;

	/* Convert config pointer to string */
	if ((cstr = XConfig_Print(xc)) == NULL)
	{
		return false;
	}

	/* Create and open file with W */
	if ((fd = open(file, O_WRONLY | O_CREAT, 0666)) < 0)
	{
		free(cstr);
		return false;
	}

	/* Write to FD */
	write(fd, cstr, strlen(cstr));

	/* Clean up */
	free(cstr);
	close(fd);

	return true;
}

/* Create a XConfig pointer */
XC_EXPORT(XConfig *) XConfig_Create(void)
{
	XConfig *s = (XConfig*)malloc(sizeof(XConfig));
	return s;
}

/* Add a section */
XC_EXPORT(bool) XConfig_AddSection(XConfig *xc, const char *name)
{
	bool succ = config_add_section(xc->config, name);
	return succ;
}

/* Check if a key had added */
XC_STATIC(bool) XConfig_IsKeyAdded(ConfigSection *css, const char *key)
{
	bool found = false;
	if (!css)
	{
		return false;
	}

	if (!(css->entries) || !(css->entries->key))
	{
		return false;
	}

	ConfigEntry *ce = css->entries;
	while (ce)
	{
		if (ce->key && strcmp(key, ce->key) != 0) {
			ce = ce->next;
			continue;
		}

		found = true;
		break;
	}

	return found;
}

/* Add key-value pair to configuration */
XC_EXPORT(bool) XConfig_AddKeyValue(XConfig *xc, const char *section,
					const char *key, const char *name)
{
	ConfigSection *current_section = xc->config->sections;
	bool found = false;

	/* Search section */
	while (current_section)
	{
		if (section && strcmp(current_section->name, section) != 0)
		{
			current_section = current_section->next;
			continue;
		}

		found = true;
		if (XConfig_IsKeyAdded(current_section, "key"))
		{
			cparse_set_error(&xc->parser, "The key had already added");
			return false;
		}
		TRACE("Founded section : '%s'\n", section);
		xc->config->current_section = current_section;
		config_add_entry(xc->config, key, name);
		break;
	}

	if (!found)
	{
		cparse_set_error(&xc->parser, "Section not found");
	}

	return found;
}
