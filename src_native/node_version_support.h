#pragma once

#include <nan.h>

// Do not attempt to build for older versions of NodeJS. We still want the module
// to install successfully though.
#if NODE_MODULE_VERSION >= NODE_4_0_MODULE_VERSION
#define IS_SUPPORTED_NODE_VERSION
#endif
