#pragma once

#include "rendercore/StaticMesh.h"
#include "scene/base/RenderableObject.h"
#include "renderer/EngineLoader.h"

// A scene object that has the capability to render static meshes
class CStaticMeshObject : public CRenderableWorldObject {

	REGISTER_CLASS(CStaticMeshObject, CRenderableWorldObject)

public:

	CStaticMeshObject() = default;

	CStaticMeshObject(const std::shared_ptr<SStaticMesh>& inMesh): mesh(inMesh) {}

	no_discard virtual const std::shared_ptr<SStaticMesh>& getMesh() const {
		return mesh;
	}

	std::shared_ptr<SStaticMesh> mesh;

	virtual CArchive& save(CArchive& inArchive) override {
		CWorldObject::save(inArchive);
		const bool hasMesh = mesh != nullptr;
		// Save the mesh's name
		inArchive << hasMesh;
		if (hasMesh) {
			inArchive << mesh->name;
		}
		return inArchive;
	}

	virtual CArchive& load(CArchive& inArchive) override {
		CWorldObject::load(inArchive);
		// Load the mesh's name and ask mesh loader for it
		bool hasMesh;
		inArchive >> hasMesh;
		if (hasMesh) {
			std::string name;
			inArchive >> name;
			mesh = CEngineLoader::getMeshes()[name];
		}

		return inArchive;
	}
};

//TODO: mostly the same as SInstancer
class CInstancedStaticMeshObject : public CStaticMeshObject {

public:

	SInstancer instancer;

	CInstancedStaticMeshObject() {}

	virtual IInstancer& getInstancer() override {
		return instancer;
	}

	virtual uint32 addInstance(const Transform3f& inPosition) {
		return instancer.push(SInstance{inPosition.toMatrix()});
	}

	virtual void setInstance(const int32 inInstanceIndex, const Transform3f& inPosition) {
		SInstance& instance = instancer.instances[inInstanceIndex];
		instance.Transform = inPosition.toMatrix();
		instancer.setDirty();
	}

	virtual void removeInstance(const uint32 instance) {
		instancer.remove(instance);
	}

	virtual CArchive& save(CArchive& inArchive) override {
		CStaticMeshObject::save(inArchive);
		inArchive << instancer;
		return inArchive;
	}

	virtual CArchive& load(CArchive& inArchive) override {
		CStaticMeshObject::load(inArchive);
		inArchive >> instancer;
		return inArchive;
	}
};