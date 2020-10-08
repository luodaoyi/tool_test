#include "OnceCall.h"

void OnceCall::Invoke(std::function<void(void)> fn)
{
	std::call_once(flag_, fn);
}
