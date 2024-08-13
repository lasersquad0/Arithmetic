// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

#define PLUGIN_NAME "ARPacker"

#define LOGGER_NAME "TCPluginLogger"
#define LOGGER_FILE_NAME _T("ARPackerX.log") // we need _T() here 

#define CONFIG_SECTION _T("ARPacker")
#define CONFIG_PARAM_BLOCKSIZE _T("BlockSize")
#define CONFIG_PARAM_MODEL _T("Model")
#define CONFIG_PARAM_CODER _T("Coder")
#define CONFIG_PARAM_THREADS _T("Threads")
#define CONFIG_PARAM_STREAMMODE _T("StreamMode")
#define CONFIG_PARAM_LOGFILENAME _T("LogFileName")

