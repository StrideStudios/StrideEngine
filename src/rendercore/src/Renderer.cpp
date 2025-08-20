#include "Renderer.h"

CRenderer* gRenderer;

CRenderer* CRenderer::get() {
	return gRenderer;
}

void CRenderer::set(CRenderer* inRenderer) {
	astsOnce(CRenderer); //Ensure Renderer is set only once
	gRenderer = inRenderer;
}