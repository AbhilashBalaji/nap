#include "jsonserializer.h"


#include <nap.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/prettywriter.h>

using namespace rapidjson;

#define J_NAME "name"
#define J_VALUE "value"
#define J_TYPE "type"
#define J_EDITABLE "editable"
#define J_CHILDREN "children"
#define J_PTR "ptr"
#define J_ATTRIBUTES "attributes"
#define J_VALUE_TYPE "vType"

namespace nap
{

	std::string toString(AttributeBase& attrib)
	{
		const Entity* root = dynamic_cast<const Entity*>(attrib.getRootObject());
		if (!root)
			return "";

		const TypeConverterBase* converter = root->getCore().getModuleManager().getTypeConverter(
			attrib.getValueType(), RTTI::TypeInfo::get<std::string>());
		if (!converter)
			return "";

		Attribute<std::string> stringAttr;
		if (converter->convert(&attrib, &stringAttr))
			return stringAttr.getValue();
		return "";
	}

	template <typename T>
	std::vector<AttributeBase*> realAttributes(T& attribObj)
	{
		std::vector<AttributeBase*> attribs;
		for (AttributeBase* a : attribObj.getAttributes())
			if (!a->getTypeInfo().isKindOf<CompoundAttribute>())
				attribs.emplace_back(a);
		return attribs;
	}

	template <typename T>
    void writeAttribute(T& writer, AttributeBase& attrib, bool writePointers)
	{
		writer.StartObject();
		{
			writer.String(J_NAME);
			writer.String(attrib.getName().c_str());
			writer.String(J_VALUE_TYPE);
			writer.String(attrib.getValueType().getName().c_str());
            writer.String(J_EDITABLE);
            writer.Bool(attrib.checkFlag(Editable));

            if (writePointers) {
                writer.String(J_PTR);
                writer.Int64(toPtr(attrib));
            }

			std::string value = toString(attrib);
			if (!value.empty()) {
				writer.String(J_VALUE);
				writer.String(value.c_str());
			}
		}
		writer.EndObject();
	}

	bool isAttribute(Object& obj) { return obj.getTypeInfo().isKindOf<AttributeBase>(); }

	template <typename T>
    void writeTheObject(T& writer, Object& obj, bool writePointers)
	{
		writer.StartObject();
		{
			writer.String(J_NAME);
			writer.String(obj.getName().c_str());
			writer.String(J_TYPE);
			writer.String(obj.getTypeInfo().getName().c_str());
            writer.String(J_EDITABLE);
            writer.Bool(obj.checkFlag(Editable));

            if (writePointers) {
                writer.String(J_PTR);
                writer.Int64(toPtr(obj));
            }

			if (obj.getTypeInfo().isKindOf<CompoundAttribute>()) {
				CompoundAttribute* attribObj = static_cast<CompoundAttribute*>(&obj);

				auto attribs = realAttributes(*attribObj);
				if (attribs.size() > 0) {
					writer.String(J_ATTRIBUTES);
					writer.StartArray();
					for (AttributeBase* attrib : attribs) {
                        writeAttribute(writer, *attrib, writePointers);
					}
					writer.EndArray();
				}
			} else if (obj.getTypeInfo().isKindOf<AttributeObject>()) {
				AttributeObject* attribObj = static_cast<AttributeObject*>(&obj);
				auto attribs = realAttributes(*attribObj);
				if (attribs.size() > 0) {
					writer.String(J_ATTRIBUTES);
					writer.StartArray();
					for (AttributeBase* attrib : attribs) {
                        writeAttribute(writer, *attrib, writePointers);
					}
					writer.EndArray();
				}
			}



			auto children = obj.getChildren();
			std::vector<Object*> filteredObjects;
			for (auto child : children) {
				if (!isAttribute(*child))
					filteredObjects.emplace_back(child);
			}

			if (filteredObjects.size() > 0) {
				writer.String(J_CHILDREN);
				writer.StartArray();
				for (Object* child : filteredObjects)
                    writeTheObject(writer, *child, writePointers);
				writer.EndArray();
			}
		}
		writer.EndObject();
	}

    TypeList getInstantiableSubTypes(RTTI::TypeInfo parentType) {
        TypeList types;
        for (const auto& type : RTTI::TypeInfo::getRawTypes()) {
            if (!type.isKindOf(parentType))
                continue;
            if (type == parentType)
                continue;
            if (!type.canCreateInstance())
                continue;
            types.push_back(type);
        }
        return types;
    }

    TypeList getAttributeTypes() {
        TypeList types;
        for (const auto& type: RTTI::TypeInfo::getRawTypes()) {
            if (type.isKindOf<AttributeBase>() && type.canCreateInstance()) {
                AttributeBase* attrib = static_cast<AttributeBase*>(type.createInstance());
                types.push_back(attrib->getValueType());
            }
        }
        return types;
    }

    void writeTypes(PrettyWriter<StringBuffer>& w, TypeList types) {
        w.StartArray();
        for (const auto& type : types) {
            w.String(type.getName().c_str());
        }
        w.EndArray();
    }

	void JSONSerializer::writeModuleInfo(std::ostream& ostream, ModuleManager& moduleManager) const
	{
		StringBuffer buf;
		PrettyWriter<StringBuffer> w(buf);

		w.StartObject();
		{
            w.String("modules");
            w.StartArray();
            for (const Module* mod : moduleManager.getModules())
            {
                w.StartObject();

                w.String("name");
                w.String(mod->getName().c_str());
                w.String("filename");
                w.String(mod->getFilename().c_str());

                TypeList types;

                mod->getDataTypes(types);
                if (!types.empty()) {
                    w.String("dataTypes");
                    writeTypes(w, types);
                }

                types.clear();

                mod->getComponentTypes(types);
                if (!types.empty()) {
                    w.String("componentTypes");
                    writeTypes(w, types);
                }

                types.clear();

                mod->getOperatorTypes(types);
                if (!types.empty()) {
                    w.String("operatorTypes");
                    writeTypes(w, types);
                }

                w.EndObject();
            }
            w.EndArray();

            w.String("dataTypes");
            writeTypes(w, getAttributeTypes());
            w.String("componentTypes");
            writeTypes(w, getInstantiableSubTypes(RTTI_OF(Component)));
            w.String("operatorTypes");
            writeTypes(w, getInstantiableSubTypes(RTTI_OF(Operator)));
		}
		w.EndObject();

		ostream << buf.GetString();
	}


	void JSONSerializer::writeObject(std::ostream& ostream, Object& object, bool writePointers) const
	{

		StringBuffer buf;
		PrettyWriter<StringBuffer> writer(buf);

        writeTheObject(writer, object, writePointers);

		ostream << buf.GetString();
	}

	AttributeBase* jsonToAttribute(Value& value, Core& core, Object* parent)
	{
		const char* attrName = value.FindMember(J_NAME)->value.GetString();
		const char* attrValueTypeName = value.FindMember(J_VALUE_TYPE)->value.GetString();

		assert(parent->getTypeInfo().isKindOf<AttributeObject>() ||
			   parent->getTypeInfo().isKindOf<CompoundAttribute>());
		AttributeObject* attributeObject = static_cast<AttributeObject*>(parent);

		AttributeBase* attrib = attributeObject->getAttribute(attrName);
		if (!attrib) {
			RTTI::TypeInfo attrType = RTTI::TypeInfo::getByName(dirtyHack(attrValueTypeName));
			attrib = &attributeObject->addAttribute(attrName, attrType);
		}
		assert(!strcmp(attrValueTypeName, attrib->getValueType().getName().c_str()));


		const TypeConverterBase* converter =
			core.getModuleManager().getTypeConverter(RTTI_OF(std::string), attrib->getValueType());
		if (!converter) {
			nap::Logger::fatal("Cannot convert to %s", attrib->getValueType().getName().c_str());
			return attrib;
		}

		Attribute<std::string> strAttr;
		std::string valueStr = value.FindMember(J_VALUE)->value.GetString();
		strAttr.setValue(valueStr);
		if (!converter->convert(&strAttr, attrib)) {
			nap::Logger::fatal("Conversion failed from '%s' to '%s'", converter->inType().getName().c_str(),
							   converter->outType().getName().c_str());
		}

		return attrib;
	};

	Object* jsonToObject(Value& value, Core& core, Object* parent)
	{
		const char* objectName = value.FindMember(J_NAME)->value.GetString();
		const char* objectTypename = value.FindMember(J_TYPE)->value.GetString();

		Object* obj = nullptr;

		if (!parent) {
			// Reading root object
			core.getRoot().setName(objectName);
			obj = &core.getRoot();
		} else {
			RTTI::TypeInfo objectType = RTTI::TypeInfo::getByName(objectTypename);

			// Handle Entity
			if (objectType.isKindOf<Entity>()) {
				assert(parent->getTypeInfo().isKindOf<Entity>());
				Entity* parentEntity = static_cast<Entity*>(parent);
				obj = &parentEntity->addEntity(objectName);

			} else {
				// Handle other types
				if (parent->hasChild(objectName) && parent->getChild(objectName)->getTypeInfo().isKindOf(objectType)) {
					// Child alread exists
					obj = parent->getChild(objectName);
				} else {
					obj = &parent->addChild(objectName, objectType);
				}
			}
		}

		if (value.HasMember(J_ATTRIBUTES)) {
			Value& attribs = value[J_ATTRIBUTES];
			assert(attribs.IsArray());
			for (auto it = attribs.Begin(); it != attribs.End(); ++it)
				jsonToAttribute(*it, core, obj);
		}

		if (value.HasMember(J_CHILDREN)) {
			Value& children = value[J_CHILDREN];
			assert(children.IsArray());
			for (auto it = children.Begin(); it != children.End(); ++it)
				jsonToObject(*it, core, obj);
		}

		return obj;
	};

	Object* JSONDeserializer::readObject(std::istream& istream, Core& core, Object* parent) const
	{
		IStreamWrapper is(istream);

		Document doc;
		doc.ParseStream(is);
		assert(doc.IsObject()); // JSON may only contain one root object

		return jsonToObject(doc, core, parent);
	}
}