// Local Includes
#include "particleemittercomponent.h"

#include <entity.h>
#include <transformcomponent.h>
#include <rect.h>
#include <glm/gtx/transform.hpp>
#include <mathutils.h>

RTTI_BEGIN_CLASS(nap::ParticleEmitterComponent)
	RTTI_PROPERTY("SpawnRate",				&nap::ParticleEmitterComponent::mSpawnRate,					nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("LifeTime",				&nap::ParticleEmitterComponent::mLifeTime,					nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("Position",				&nap::ParticleEmitterComponent::mPosition,					nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("PositionVariation",		&nap::ParticleEmitterComponent::mPositionVariation,			nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("LifeTimeVariation",		&nap::ParticleEmitterComponent::mLifeTimeVariation,			nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("Size",					&nap::ParticleEmitterComponent::mSize,						nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("SizeVariation",			&nap::ParticleEmitterComponent::mSizeVariation,				nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("Rotation",				&nap::ParticleEmitterComponent::mRotation,					nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("RotationVariation",		&nap::ParticleEmitterComponent::mRotationVariation,			nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("RotationSpeed",			&nap::ParticleEmitterComponent::mRotationSpeed,				nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("RotationSpeedVariation", &nap::ParticleEmitterComponent::mRotationSpeedVariation,	nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("Spread",					&nap::ParticleEmitterComponent::mSpread,					nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("Velocity",				&nap::ParticleEmitterComponent::mVelocity,					nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("VelocityVariation",		&nap::ParticleEmitterComponent::mVelocityVariation,			nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("StartColor",				&nap::ParticleEmitterComponent::mStartColor,				nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("EndColor",				&nap::ParticleEmitterComponent::mEndColor,					nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::ParticleEmitterComponentInstance)
	RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component&) 
RTTI_END_CLASS

namespace nap
{
	// All the plane uvs
	static glm::vec3 plane_uvs[] =
	{
		{ 0.0f,	0.0f,	0.0f },
		{ 1.0f,	0.0f,	0.0f },
		{ 0.0f,	1.0f,	0.0f },
		{ 1.0f,	1.0f,	0.0f },
	};


	template<typename T>
	static T particleRand(T baseValue, T variation)
	{
		return math::random<T>(baseValue + variation, baseValue - variation);
	}


	//////////////////////////////////////////////////////////////////////////

	class ParticleMesh : public IMesh
	{
	public:
		bool init(utility::ErrorState& errorState)
		{
			mMeshInstance.setNumVertices(0);
			mMeshInstance.setDrawMode(opengl::EDrawMode::TRIANGLES);
			Vec3VertexAttribute& position_attribute = mMeshInstance.getOrCreateAttribute<glm::vec3>(VertexAttributeIDs::getPositionName());
			Vec3VertexAttribute& uv_attribute = mMeshInstance.getOrCreateAttribute<glm::vec3>(VertexAttributeIDs::getUVName(0));
			Vec4VertexAttribute& color_attribute = mMeshInstance.getOrCreateAttribute<glm::vec4>(VertexAttributeIDs::GetColorName(0));
			FloatVertexAttribute& id_attribute = mMeshInstance.getOrCreateAttribute<float>("pid");
			mMeshInstance.reserveVertices(1000);
			mMeshInstance.reserveIndices(1000);

			return mMeshInstance.init(errorState);
		}

		/**
		* @return MeshInstance as created during init().
		*/
		virtual MeshInstance& getMeshInstance()	override { return mMeshInstance; }

		/**
		* @return MeshInstance as created during init().
		*/
		virtual const MeshInstance& getMeshInstance() const	override { return mMeshInstance; }

	private:
		MeshInstance mMeshInstance;
	};

	//////////////////////////////////////////////////////////////////////////

	ParticleEmitterComponentInstance::ParticleEmitterComponentInstance(EntityInstance& entity, Component& resource) :
		RenderableMeshComponentInstance(entity, resource),
		mParticleMesh(std::make_unique<ParticleMesh>())
	{
	}


	ParticleEmitterComponentInstance::~ParticleEmitterComponentInstance()
	{
	}


	bool ParticleEmitterComponentInstance::init(utility::ErrorState& errorState)
	{
		if (!RenderableMeshComponentInstance::init(errorState))
			return false;

		if (!errorState.check(mParticleMesh->init(errorState), "Unable to create particle mesh"))
			return false;

		RenderableMesh renderableMesh = createRenderableMesh(*mParticleMesh, errorState);
		if (!renderableMesh.isValid())
			return false;

		setMesh(renderableMesh);

		return true;
	}


	void ParticleEmitterComponentInstance::updateParticles(double deltaTime)
	{
		ParticleEmitterComponent* component = getComponent<ParticleEmitterComponent>();
		double spawnTimeMs = (1.0f / component->mSpawnRate);
		mTimeSinceLastSpawn += deltaTime;
		if (mTimeSinceLastSpawn >= spawnTimeMs)
		{
			Particle particle(mCurrentID++);
			particle.mPosition =		particleRand(component->mPosition, component->mPositionVariation);
			particle.mRotation =		particleRand(component->mRotation, component->mRotationVariation);
			particle.mRotationAngle =	glm::normalize(particleRand<glm::vec3>({ 0.0,0.0,0.0 }, { 1.0,1.0,1.0 }));
			particle.mRotationSpeed =	particleRand(component->mRotationSpeed, component->mRotationSpeedVariation);
			particle.mSize =			particleRand(component->mSize, component->mSizeVariation);
			particle.mLifeTime =		particleRand(component->mLifeTime, component->mLifeTimeVariation);
			float spread =				particleRand(0.0f, component->mSpread);
			float velocity_x =			particleRand(component->mVelocity.x, component->mVelocityVariation);
			float velocity_y =			particleRand(component->mVelocity.y, component->mVelocityVariation);
			float velocity_z =			particleRand(component->mVelocity.z, component->mVelocityVariation);
			particle.mVelocity =		glm::vec3(spread, 1.0f, spread);
			particle.mVelocity =		glm::normalize(particle.mVelocity);
			particle.mVelocity *=		glm::vec3(velocity_x, velocity_y, velocity_z);
			particle.mTimeLeft =		particle.mLifeTime;

			mParticles.emplace_back(particle);
			mCurrentID = mCurrentID % math::max<int>();
			mTimeSinceLastSpawn = 0.0f;
		}

		for (int i = mParticles.size() - 1; i >= 0; --i)
		{
			Particle& particle = mParticles[i];
			particle.mTimeLeft -= deltaTime;
			if (particle.mTimeLeft < 0.0)
			{
				mParticles.erase(mParticles.begin() + i);
			}
			else
			{
				particle.mPosition += particle.mVelocity * (float)deltaTime;
				particle.mRotation += particle.mRotationSpeed * (float)deltaTime;

				float life_scale = 1.0f - (particle.mTimeLeft / component->mLifeTime);
				particle.mColor = component->mStartColor + (component->mEndColor - component->mStartColor) * life_scale;
				particle.mColor.a = math::bell(life_scale, 2.0f);
			}
		}
	}


	void ParticleEmitterComponentInstance::updateMesh()
	{
 		MeshInstance& mesh_instance = mParticleMesh->getMeshInstance();

		int num_vertices = mParticles.size() * 4;
		mesh_instance.setNumVertices(num_vertices);

		Vec3VertexAttribute& position_attribute = mesh_instance.getOrCreateAttribute<glm::vec3>(VertexAttributeIDs::getPositionName());
		Vec3VertexAttribute& uv_attribute		= mesh_instance.getOrCreateAttribute<glm::vec3>(VertexAttributeIDs::getUVName(0));
		Vec4VertexAttribute& color_attribute	= mesh_instance.getOrCreateAttribute<glm::vec4>(VertexAttributeIDs::GetColorName(0));
		FloatVertexAttribute& id_attribute = mesh_instance.getOrCreateAttribute<float>("pid");

		position_attribute.clear();
		uv_attribute.clear();
		color_attribute.clear();
		id_attribute.clear();
		mesh_instance.clearIndices();

		unsigned int cur_num_vertices = 0;
		for (Particle& particle : mParticles)
		{
			// Create the position vertices
			float halfSize = particle.mSize * 0.5f;
			math::Rect rect(-halfSize, -halfSize, particle.mSize, particle.mSize);

			glm::vec3 positions[] =
			{
				{rect.getMin().x,	rect.getMin().y, 0.0f },
				{rect.getMax().x,	rect.getMin().y, 0.0f },
				{rect.getMin().x,	rect.getMax().y, 0.0f },
				{rect.getMax().x,	rect.getMax().y, 0.0f },
			};

			float ids[] =
			{
				float(particle.mID),
				float(particle.mID),
				float(particle.mID),
				float(particle.mID)
			};

			glm::mat4x4 translation = glm::translate(particle.mPosition);
			glm::mat4x4 rotation = glm::rotate(particle.mRotation, particle.mRotationAngle);
			for (glm::vec3& pos : positions)
			{
				glm::vec4 world_pos = translation * rotation * glm::vec4(pos, 1.0f);
				pos = glm::vec3(world_pos.x, world_pos.y, world_pos.z);
			}

			unsigned int indices[] =
			{
				cur_num_vertices + 0,
				cur_num_vertices + 1,
				cur_num_vertices + 3,
				cur_num_vertices + 0,
				cur_num_vertices + 3,
				cur_num_vertices + 2
			};

			glm::vec4 colors[] =
			{
				particle.mColor, particle.mColor, particle.mColor, particle.mColor
			};

			position_attribute.addData(positions, 4);
			uv_attribute.addData(plane_uvs, 4);
			color_attribute.addData(colors, 4);
			id_attribute.addData(ids, 4);
			mesh_instance.addIndices(indices, 6);

			cur_num_vertices += 4;
		}

		utility::ErrorState error_state;
		mesh_instance.update(error_state);
	}

	void ParticleEmitterComponentInstance::update(double deltaTime)
	{
		updateParticles(deltaTime);
		updateMesh();

		RenderableMeshComponentInstance::update(deltaTime);
	}
}