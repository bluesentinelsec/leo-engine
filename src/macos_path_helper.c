#include "leo/macos_path_helper.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

char* leo_GetResourceBasePath(void) {
#ifdef __APPLE__
    CFBundleRef bundle = CFBundleGetMainBundle();
    if (bundle) {
        CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(bundle);
        if (resourcesURL) {
            char path[PATH_MAX];
            if (CFURLGetFileSystemRepresentation(resourcesURL, true, (UInt8*)path, PATH_MAX)) {
                CFRelease(resourcesURL);
                return strdup(path);
            }
            CFRelease(resourcesURL);
        }
    }
#endif
    
    // Fallback to current directory
    char* cwd = getcwd(NULL, 0);
    return cwd;
}
