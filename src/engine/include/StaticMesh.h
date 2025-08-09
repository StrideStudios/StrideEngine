#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Common.h"
#include "EngineLoader.h"
#include "Hashing.h"
#include "Material.h"
#include "SceneObject.h"

struct SStaticMesh {

	struct Surface {

		std::string name;

		std::shared_ptr<CMaterial> material;

		uint32 startIndex;
		uint32 count;
	};

	std::string name;

	SBounds bounds;
	std::vector<Surface> surfaces;
	SMeshBuffers meshBuffers;

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
class CStaticMeshObject : public CRenderableObject {

	REGISTER_CLASS(CStaticMeshObject)

public:

	CStaticMeshObject() = default;

	CStaticMeshObject(const std::shared_ptr<SStaticMesh>& inMesh): mesh(inMesh) {}

	no_discard const std::shared_ptr<SStaticMesh>& getMesh() const override {
		return mesh;
	}

	std::shared_ptr<SStaticMesh> mesh;

	CArchive& save(CArchive& inArchive) override {
		CRenderableObject::save(inArchive);
		// Save the mesh's name
		inArchive << mesh->name;
		return inArchive;
	}

	CArchive& load(CArchive& inArchive) override {
		CRenderableObject::load(inArchive);
		// Load the mesh's name and ask mesh loader for it
		std::string name;
		inArchive >> name;
		mesh = CEngineLoader::getMeshes()[name];
		return inArchive;
	}
};