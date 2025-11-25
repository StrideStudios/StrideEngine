#pragma once

#include <stack>

#include "basic/core/Common.h"

struct SRenderStack {

	/*
	 * 3D Matrix Modifications
	 */

	SRenderStack& translate(const Vector3f inPosition) {
		m_MatrixStack.top() = glm::translate(get(), inPosition);
		return *this;
	}

	SRenderStack& rotate(const Vector3f inRotation) {
		m_MatrixStack.top() *= glm::mat4_cast(glm::qua(inRotation / 180.f * static_cast<float>(PI)));
		return *this;
	}

	SRenderStack& scale(const Vector3f inScale) {
		m_MatrixStack.top() = glm::scale(get(), inScale);
		return *this;
	}

	/*
	 * 2D Matrix Modifications
	 */

	SRenderStack& translate(const Vector2f inPosition, const Vector2f inOrigin = Vector2f(0.f), const Vector2f inScale = Vector2f(1.f)) {
		m_MatrixStack.top() = glm::translate(get(), Vector3f(inPosition - inOrigin * inScale, 0.f));
		return *this;
	}

	SRenderStack& rotate(const float inRotation) {
		m_MatrixStack.top() *= glm::mat4_cast(glm::qua(Vector3f(0.f, 0.f, inRotation / 180.f * static_cast<float>(PI))));
		return *this;
	}

	SRenderStack& scale(const Vector2f inScale) {
		m_MatrixStack.top() = glm::scale(get(), Vector3f(inScale, 1.f));
		return *this;
	}

	/*
	 * Stack Utils
	 */

	const Matrix4f& get() const { return m_MatrixStack.top(); }

	void push() {
		m_MatrixStack.push(Matrix4f{1.f});
	}

	void pop() {
		m_MatrixStack.pop();
	}

	operator Matrix4f() { return get(); }

	operator Matrix4f() const { return get(); }

	friend CArchive& operator<<(CArchive& inArchive, const SRenderStack& inRenderStack) {
		inArchive << inRenderStack.m_MatrixStack;
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, SRenderStack& inRenderStack) {
		inArchive >> inRenderStack.m_MatrixStack;
		return inArchive;
	}

private:

	std::stack<Matrix4f> m_MatrixStack;

};
