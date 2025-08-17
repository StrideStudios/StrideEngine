#pragma once

#include <memory>
#include <string>
#include <vector>

#include "EngineLoader.h"
#include "Material.h"
#include "SceneObject.h"

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
class EXPORT CStaticMeshObject : public CSceneObject3D {

	REGISTER_CLASS(CStaticMeshObject)

public:

	CStaticMeshObject() = default;

	CStaticMeshObject(const std::shared_ptr<SStaticMesh>& inMesh): mesh(inMesh) {}

	no_discard virtual const std::shared_ptr<SStaticMesh>& getMesh() const {
		return mesh;
	}

	std::shared_ptr<SStaticMesh> mesh;

	virtual CArchive& save(CArchive& inArchive) override {
		CSceneObject3D::save(inArchive);
		// Save the mesh's name
		inArchive << static_cast<bool>(mesh);
		if (mesh) {
			inArchive << mesh->name;
		}
		return inArchive;
	}

	virtual CArchive& load(CArchive& inArchive) override {
		CSceneObject3D::load(inArchive);
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
class EXPORT CInstancedStaticMeshObject : public CStaticMeshObject {

public:

	/*CInstancedStaticMeshObject() {
		m_Instances.clear();
		setDirty();
	}

	virtual uint32 addInstance(const Transform3f& inPosition) {
		return m_Instancer.push(SInstance{inPosition.toMatrix()});
	}

	virtual void setInstance(const int32 inInstanceIndex, const Transform3f& inPosition) {
		SInstance& instance = m_Instancer.instances[inInstanceIndex];
		instance.Transform = inPosition.toMatrix();
		m_Instancer.setDirty();
	}

	virtual void removeInstance(const uint32 instance) {
		m_Instancer.remove(instance);
	}

	virtual void onPositionChanged() override {
		m_Instancer.setDirty();
	}

	virtual SInstancer& getInstancer() override {
		return m_Instancer;
	}

	virtual CArchive& save(CArchive& inArchive) override {
		CStaticMeshObject::save(inArchive);
		inArchive << m_Instancer;
		return inArchive;
	}

	virtual CArchive& load(CArchive& inArchive) override {
		CStaticMeshObject::load(inArchive);
		inArchive >> m_Instancer;
		return inArchive;
	}*/
};