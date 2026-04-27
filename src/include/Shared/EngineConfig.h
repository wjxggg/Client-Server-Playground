#pragma once

#define BE_BEGIN_NAMESPACE namespace BE {
#define BE_END_NAMESPACE }

#define BE_USING_NAMESPACE using namespace BE;

#if defined(BE_HOST_CLIENT) || defined(BE_HOST_SERVER)
	#define BE_API __attribute__ ((visibility("default")))
#else
	#define BE_API
#endif

// MODULE_NAME is a predefined macro passed by ModuleCompiler
#if !defined(BE_MODULE_NAME)
	#define BE_MODULE_NAME "BlockEngine"
#endif

#define BE_MODULE_CONFIG_DIR "./config/" BE_MODULE_NAME
