#include "utils/include.h"

#include <appcontext.h>
#include <QtDebug>
#include <utility/fileutils.h>

using namespace napkin;

TEST_CASE("PropertyPath", "napkin-propertypath")
{
    RUN_Q_APPLICATION

	auto doc = AppContext::get().newDocument();
	auto entity = doc->addObject<nap::Entity>();
	auto resB = doc->addObject<TestResourceB>();
	auto res = doc->addObject<TestResourceB>();
	res->mID = "MyResource";

	// Add a pointer to array of pointers
	doc->arrayAddExistingObject({*res, "ResPointers"}, resB);

	SECTION("general")
	{
		PropertyPath nameProp(*entity, nap::rtti::sIDPropertyName);
		REQUIRE(nameProp.getType() == rttr::type::get<std::string>());
		REQUIRE(&nameProp.getObject() == entity);
		REQUIRE(nameProp.isValid());
		std::string newName = "NewName";
		nameProp.setValue(newName);
		REQUIRE(nameProp.getValue() == newName);
		REQUIRE(entity->mID == newName);

		PropertyPath invalidPath;
		REQUIRE(!invalidPath.isValid());
	}

	SECTION("enum")
	{
		PropertyPath path(*res, "Enum");
		REQUIRE(path.getType() == rttr::type::get<TestEnum>());
		REQUIRE(path.isValid());
		REQUIRE(path.isEnum());
		REQUIRE(!path.isArray());
		REQUIRE(!path.isPointer());
		REQUIRE(!path.isEmbeddedPointer());
		REQUIRE(!path.isNonEmbeddedPointer());
	}

	SECTION("regular pointer")
	{
		PropertyPath path(*res, "ResPointer");
		REQUIRE(path.getType() == rttr::type::get<nap::ResourcePtr<TestResource>>());
		REQUIRE(path.isValid());
		REQUIRE(path.isPointer());
		REQUIRE(path.isNonEmbeddedPointer());
		REQUIRE(!path.isEmbeddedPointer());
		REQUIRE(!path.isEnum());
		REQUIRE(!path.isArray());
	}

	SECTION("array of regular pointers")
	{
		PropertyPath path(*res, "ResPointers");
		REQUIRE(path.getType() == rttr::type::get<std::vector<nap::ResourcePtr<TestResource>>>());
		REQUIRE(path.isValid());
		REQUIRE(path.isArray());
		REQUIRE(path.isPointer());
		REQUIRE(path.isNonEmbeddedPointer());
		REQUIRE(!path.isEmbeddedPointer());
		REQUIRE(!path.isEnum());
	}

	SECTION("array element: regular pointer")
	{
		PropertyPath path(*res, "ResPointers/0");
		REQUIRE(path.getType() == rttr::type::get<nap::ResourcePtr<TestResource>>());
		REQUIRE(path.isValid());
		REQUIRE(path.isPointer());
		REQUIRE(path.isNonEmbeddedPointer());
		REQUIRE(!path.isArray());
		REQUIRE(!path.isEmbeddedPointer());
		REQUIRE(!path.isEnum());
	}

	SECTION("embedded pointer")
	{
		PropertyPath path(*res, "EmbedPointer");
		REQUIRE(path.getType() == rttr::type::get<nap::ResourcePtr<TestResource>>());
		REQUIRE(path.isValid());
		REQUIRE(path.isPointer());
		REQUIRE(path.isEmbeddedPointer());
		REQUIRE(!path.isNonEmbeddedPointer());
		REQUIRE(!path.isEnum());
		REQUIRE(!path.isArray());
	}

	SECTION("array of embedded pointers")
	{
		PropertyPath path(*res, "EmbedPointers");
		REQUIRE(path.getType() == rttr::type::get<std::vector<nap::ResourcePtr<TestResource>>>());
		REQUIRE(path.isValid());
		REQUIRE(path.isArray());
		REQUIRE(path.isPointer());
		REQUIRE(path.isEmbeddedPointer());
		REQUIRE(!path.isNonEmbeddedPointer());
		REQUIRE(!path.isEnum());
	}

	SECTION("array of structs")
	{
		TestPropertiesStruct uniform;
		res->mStructs.emplace_back(uniform);
		PropertyPath path(*res, "Structs");
		REQUIRE(path.getType() == rttr::type::get<std::vector<TestPropertiesStruct>>());
		REQUIRE(path.isValid());
		REQUIRE(path.isArray());
		REQUIRE(!path.isPointer());
		REQUIRE(!path.isEmbeddedPointer());
		REQUIRE(!path.isNonEmbeddedPointer());
		REQUIRE(!path.isEnum());
	}

}

TEST_CASE("InstanceProperties", "[napkinpropertypath]")
{
	RUN_Q_APPLICATION

	auto& ctx = AppContext::get();
	auto doc = ctx.newDocument();
	auto entity = doc->addObject<nap::Entity>();
	auto comp = doc->addComponent<TestComponent>(*entity);
	auto scene = doc->addObject<nap::Scene>();
	doc->addEntityToScene(*scene, *entity);

	auto rootEntity = doc->getRootEntity(*scene, *entity);
	REQUIRE(rootEntity != nullptr);

	PropertyPath regularPath(*comp, "Float");
	REQUIRE(!regularPath.isInstance());
	REQUIRE(regularPath.isValid());

	PropertyPath instancePath(*rootEntity, *comp, "Float");
	REQUIRE(instancePath.isInstance());
	REQUIRE(instancePath.isValid());

	float val1 = 123.456;
	regularPath.setValue(val1);
	REQUIRE(regularPath.getValue() == val1);
	REQUIRE(instancePath.getValue() == val1);

	float val2 = 678.90;
	instancePath.setValue(val2);
	REQUIRE(instancePath.getValue() == val2);
	REQUIRE(regularPath.getValue() != val2);

	PropertyPath instancePath2(*rootEntity, *comp, "Float");
	REQUIRE(instancePath2.getValue() == val2);


	QString tempFilename = "__TEMP_NAPKIN_PROP_PATH_TEST.json";
	doc->setFilename(tempFilename);
	nap::Logger::info(nap::utility::getAbsolutePath(doc->getCurrentFilename().toStdString()));
	REQUIRE(ctx.saveDocument());

}

TEST_CASE("PropertyIteration", "[napkinpropertypath]")
{
	RUN_Q_APPLICATION

	TestResourceB res;
	res.mID = "TestResource";

	{
		auto props = PropertyPath(res).getProperties();
		REQUIRE(props.size() == 16);
	}

	{
		auto props = PropertyPath(res).getProperties(IterFlag::Resursive);
		REQUIRE(props.size() == 26);
	}

	{
		TestResource subRes;
		subRes.mID = "SubRes";
		res.mResPointer = &subRes;
		REQUIRE(res.mResPointer != nullptr);

		TestResourceB embedRes;
		embedRes.mID = "EmbedRes";
		res.mEmbedPointer = &embedRes;
		REQUIRE(res.mEmbedPointer != nullptr);

		PropertyPath p(res, "ResPointer");
		REQUIRE(p.isValid());
		REQUIRE(p.isPointer());


		auto props1 = PropertyPath(res).getProperties(IterFlag::Resursive | IterFlag::FollowPointers);
		for (auto p : props1)
		{
			// The embedded pointee cannot be in this result, only regular pointees
			REQUIRE(&p.getObject() != &embedRes);
		}
		REQUIRE(props1.size() == 38);

		auto props2 = PropertyPath(res).getProperties(IterFlag::Resursive | IterFlag::FollowEmbeddedPointers);
		for (auto p : props2)
		{
			// The regular pointee subRes cannot be in this result, only embedded pointees
			REQUIRE(&p.getObject() != &subRes);
		}
		REQUIRE(props2.size() == 42);

		res.mResPointer = nullptr;

	}
}