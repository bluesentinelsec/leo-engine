#include "leo/macos_path_helper.h"
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <limits.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <direct.h>
#else
#include <unistd.h>
#endif

char *leo_GetResourceBasePath(void)
{
#ifdef __APPLE__
    CFBundleRef bundle = CFBundleGetMainBundle();
    if (bundle)
    {
        CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(bundle);
        if (resourcesURL)
        {
            char path[PATH_MAX];
            if (CFURLGetFileSystemRepresentation(resourcesURL, true, (UInt8 *)path, PATH_MAX))
            {
                CFRelease(resourcesURL);
                return strdup(path);
            }
            CFRelease(resourcesURL);
        }
    }

    // Fallback to current directory
    char *cwd = getcwd(NULL, 0);
    return cwd;
#elif defined(_WIN32)
    // Windows: return current directory
    return _getcwd(NULL, 0);
#else
    // Other platforms: return current directory
    return getcwd(NULL, 0);
#endif
}
