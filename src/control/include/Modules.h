#pragma once

// Module for the renderer
class CRendererModule : public CModule {
public:
	virtual void render() = 0;

	virtual bool wait() = 0;
};