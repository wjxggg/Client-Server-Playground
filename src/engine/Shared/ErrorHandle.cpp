#include "ErrorHandle.h"

#include <cstdlib>
#include <iostream>

#include "Log.h"

BE_USING_NAMESPACE

void ErrorHandle::PauseThenAbort()
{
	Log::Error("Abort due to an error");

	std::cin.get();
	std::abort();
}
