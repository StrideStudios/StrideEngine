
// This include ensures static calls are done before main (even in libraries)
// Without it, the linker might drop symbols that need to run before main (like factory registration)
#include "Sources.h"

int main() {

#ifdef BUILD_DEBUG
	msgs("Engine Initialized with profile 'Debug'");
#endif
#ifdef BUILD_RELEASE
	msgs("Engine Initialized with profile 'Release'");
#endif

	// Tell Engine to use CEditorRenderer, which has certain passes
	CEngine::run<CEditorRenderer>();

	return 0;
}
