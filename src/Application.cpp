
// This include ensures static calls are done before main (even in libraries)
// Without it, the linker might drop symbols that need to run before main (like factory registration)
#include "Sources.h"

int main() {

	// Tell Engine to use CEditorRenderer, which has certain passes
	CEngine::run<CEditorRenderer>();

	return 0;
}
