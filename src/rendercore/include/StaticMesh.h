#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Material.h"
#include "base/RenderableObject.h"
#include "VulkanResources.h"

struct SBounds {
	Vector3f origin;
	float sphereRadius;
	Vector3f extents;

	friend CArchive& operator<<(CArchive& inArchive, const SBounds& inBounds) {
		inArchive << inBounds.origin;
		inArchive << inBounds.sphereRadius;
		inArchive << inBounds.extents;
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, SBounds& inBounds) {
		inArchive >> inBounds.origin;
		inArchive >> inBounds.sphereRadius;
		inArchive >> inBounds.extents;
		return inArchive;
	}
};

struct SStaticMesh {

	struct Surface {

		std::string name;

		std::shared_ptr<CMaterial> material;

		uint32 startIndex;
		uint32 count;
	};

	std::string name{"None"};

	SBounds bounds;
	std::vector<Surface> surfaces;
	SMeshBuffers_T meshBuffers;

	friend uint32 getHash(const SStaticMesh& inMesh) {
		return getHash(inMesh.name);
	}

	friend CArchive& operator<<(CArchive& inArchive, const SStaticMesh& inMesh) {
		inArchive << inMesh.name;
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, SStaticMesh& inMesh) {
		inArchive >> inMesh.name;
		return inArchive;
	}
};

// A scene object that has the capability to render static meshes
// This is also where static meshes are loaded
class CStaticMeshObject : public CRenderableWorldObject {

	REGISTER_SCENE_OBJECT(CStaticMeshObject)

public:

	CStaticMeshObject() = default;

	CStaticMeshObject(const std::shared_ptr<SStaticMesh>& inMesh): mesh(inMesh) {}

	no_discard virtual const std::shared_ptr<SStaticMesh>& getMesh() const {
		return mesh;
	}

	std::shared_ptr<SStaticMesh> mesh;

	virtual CArchive& save(CArchive& inArchive) override {
		CWorldObject::save(inArchive);
		// Save the mesh's name
		inArchive << static_cast<bool>(mesh);
		if (mesh) {
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
			//mesh = CEngineLoader::getMeshes()[name]; //TODO: mesh loading
		}

		return inArchive;
	}
};

//TODO: mostly the same as SInstancer
class CInstancedStaticMeshObject : public CStaticMeshObject {

public:

	CInstancedStaticMeshObject() {
		//m_Instances.clear();
		CRenderableWorldObject::setDirty();
	}

	virtual uint32 addInstance(const Transform3f& inPosition) {
		return getInstancer().push(SInstance{inPosition.toMatrix()});
	}

	virtual void setInstance(const int32 inInstanceIndex, const Transform3f& inPosition) {
		SInstance& instance = getInstancer().instances[inInstanceIndex];
		instance.Transform = inPosition.toMatrix();
		getInstancer().setDirty();
	}

	virtual void removeInstance(const uint32 instance) {
		getInstancer().remove(instance);
	}

	virtual CArchive& save(CArchive& inArchive) override {
		CStaticMeshObject::save(inArchive);
		inArchive << getInstancer();
		return inArchive;
	}

	virtual CArchive& load(CArchive& inArchive) override {
		CStaticMeshObject::load(inArchive);
		inArchive >> getInstancer();
		return inArchive;
	}
};