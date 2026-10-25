// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

#define PLUGIN_NAME "ARPacker"

constexpr auto LOGGER_NAME = "TCPluginLogger"; //TODO we have Global::LOGGER_NAME may be use it instead of constexpr

#define LOGGER_FILE_NAME U("ARPackerX.log") // we need _T() here 

#define CONFIG_SECTION U("ARPacker")
#define CONFIG_PARAM_BLOCKSIZE U("BlockSize")
#define CONFIG_PARAM_MODEL U("Model")
#define CONFIG_PARAM_CODER U("Coder")
#define CONFIG_PARAM_THREADS U("Threads")
#define CONFIG_PARAM_STREAMMODE U("StreamMode")
#define CONFIG_PARAM_LOGFILENAME U("LogFileName")

