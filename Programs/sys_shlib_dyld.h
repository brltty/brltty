#include <mach-o/dyld.h>

static void
logDyldError (const char *action) {
  NSLinkEditErrors errors;
  int number;
  const char *file;
  const char *message;
  NSLinkEditError(&errors, &number, &file, &message);
  LogPrint(LOG_ERR, "%.*s", strlen(message)-1, message);
}

void *
loadSharedObject (const char *path) {
  NSObjectFileImage image;
  switch (NSCreateObjectFileImageFromFile(path, image)) {
    case NSObjectFileImageSuccess: {
      NSModule module = NSLinkModule(image, path, NSLINKMODULE_OPTION_RETURN_ON_ERROR);
      if (module) return module;
      logDyldError("link module");
      LogPrint(LOG_ERR, "shared object not linked: %s", path);
      break;
    }

    case NSObjectFileImageInappropriateFile:
      LogPrint(LOG_ERR, "inappropriate object type: %s", path);
      break;

    case NSObjectFileImageArch:
      LogPrint(LOG_ERR, "incorrect object architecture: %s", path);
      break;

    case NSObjectFileImageFormat:
      LogPrint(LOG_ERR, "invalid object format: %s", path);
      break;

    case NSObjectFileImageAccess:
      LogPrint(LOG_ERR, "inaccessible object: %s", path);
      break;

    case NSObjectFileImageFailure:
    default:
      LogPrint(LOG_ERR, "shared object not loaded: %s", path);
      break;
  }
  return NULL;
}

void 
unloadSharedObject (void *object) {
  NSModule module = object;
  NSUnLinkModule(module, NSUNLINKMODULE_OPTION_NONE);
}

int 
findSharedSymbol (void *object, const char *symbol, void *pointerAddress) {
  NSModule module = object;
  NSSymbol sym = NSLookupSymbolInModule(module, symbol);
  if (sym) {
    void **address = pointerAddress;
    *address = NSAddressOfSymbol(sym);
    return 1;
  }

  return 0;
}
