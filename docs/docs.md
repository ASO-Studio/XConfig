# Basic usage

## Parse a file

```C
#include "xconfig.h"

// ...
XConfig *xc = XConfig_ParseString("[Section]\n" "Key = \"Value\""); // If you want to parse from a file, use 'XConfig_ParseFile(path_to_your_file);'

// Error check
if (!xc || XConfig_HaveError())
{
    fprintf(stderr, "Parse failed: %s\n", XConfig_GetError());
    return 1;
}

// Read key
const char *key = XConfig_Read(xc, "Section", "Key");   // Return NULL if cannot find 'key' in section 'Section'

XConfig_Delete(xc);
// ...

```

## Create a config
```C
#include "xconfig.h"

// ...

// Create a pointer
XConfig *xc = XConfig_Create();

// Error checks
if (!xc)
{
    perror("Create failed");    // Use perror() instead
}

XConfig_AddSection(xc, "Section");  // Add section to POINTER

XConfig_AddKeyValue(xc, "Section", "Key", "Value");  // Add "'Key' = 'Value'" to 'Section'

char * str = XConfig_Print(xc); // Convert XConfig pointer to config string

// Error checks
if (!str)
{
    perror("Cannot convert pointer to string");
    XConfig_Delete(xc);
    return 1;
}

printf("%s\n", str);
free(str);

// Use XConfig_WriteFile(xc, "File name") to store this configuration to a file

XConfig_Delete(xc);

// ...
```
