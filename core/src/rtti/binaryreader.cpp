#include "binaryreader.h"
#include "rttibinaryversion.h"
#include "nap/errorstate.h"
#include "nap/object.h"
#include <fstream>
#include "rttiutilities.h"

namespace nap
{
	static bool deserializeObjectRecursive(nap::Object* object, RTTI::Instance compound, MemoryStream& stream, RTTI::RTTIPath& rttiPath, UnresolvedPointerList& unresolvedPointers,
		std::vector<FileLink>& linkedFiles, ErrorState& errorState);

	/**
	 * Helper function to check if the specified stream starts with the RTTI binary version header
	 */
	bool readAndCheckRTTIBinaryVersion(MemoryStream& stream)
	{
		// Determine length of header
		int len = strlen(gRTTIBinaryVersion);
		
		// Read header from stream
		char header[64] = { 0 };
		stream.read(header, len);

		return strcmp(gRTTIBinaryVersion, header) == 0;
	}


	/**
	 * Helper function to read a basic JSON type to a C++ type
	 */
	static RTTI::Variant deserializePrimitive(const RTTI::TypeInfo& type, MemoryStream& stream)
	{
		if (type.is_arithmetic())
		{
			if (type == RTTI::TypeInfo::get<bool>())
				return stream.read<bool>();
			else if (type == RTTI::TypeInfo::get<char>())
				return stream.read<uint8_t>();
			else if (type == RTTI::TypeInfo::get<int8_t>())
				return stream.read<int8_t>();
			else if (type == RTTI::TypeInfo::get<int16_t>())
				return stream.read<int16_t>();
			else if (type == RTTI::TypeInfo::get<int32_t>())
				return stream.read<int32_t>();
			else if (type == RTTI::TypeInfo::get<int64_t>())
				return stream.read<int64_t>();
			else if (type == RTTI::TypeInfo::get<uint8_t>())
				return stream.read<uint8_t>();
			else if (type == RTTI::TypeInfo::get<uint16_t>())
				return stream.read<uint16_t>();
			else if (type == RTTI::TypeInfo::get<uint32_t>())
				return stream.read<uint32_t>();
			else if (type == RTTI::TypeInfo::get<uint64_t>())
				return stream.read<uint64_t>();
			else if (type == RTTI::TypeInfo::get<float>())
				return stream.read<float>();
			else if (type == RTTI::TypeInfo::get<double>())
				return stream.read<double>();
		}
		else if (type.is_enumeration())
		{
			return stream.read<uint64_t>();
		}
		else if (type == RTTI::TypeInfo::get<std::string>())
		{
			std::string str;
			stream.readString(str);
			return str;
		}

		// Unknown type
		assert(false);
		return RTTI::Variant();
	}


	/**
	 * Helper function to recursively read an array (can be an array of basic types, nested compound, or any other type) from JSON
	 */
	static bool deserializeArrayRecursively(nap::Object* rootObject, RTTI::VariantArray& array, MemoryStream& stream, RTTI::RTTIPath& rttiPath, 
		UnresolvedPointerList& unresolvedPointers, std::vector<FileLink>& linkedFiles, ErrorState& errorState)
	{
		uint32_t length;
		stream.read(length);

		// Pre-size the array to avoid too many dynamic allocs
		array.set_size(length);

		// Determine the rank of the array (i.e. how many dimensions it has)
		const RTTI::TypeInfo array_type = array.get_rank_type(array.get_rank());

		// Read values from JSON array
		for (std::size_t index = 0; index < length; ++index)
		{
			// Add array element to rtti path
			rttiPath.pushArrayElement(index);

			if (array_type.is_array())
			{
				// Array-of-arrays; read array recursively
				RTTI::VariantArray sub_array = array.get_value_as_ref(index).create_array_view();
				if (!deserializeArrayRecursively(rootObject, sub_array, stream, rttiPath, unresolvedPointers, linkedFiles, errorState))
					return false;
			}
			else if (array_type.is_associative_container())
			{
				// Maps not supported (yet)
				errorState.fail("Encountered currently unsupported associative property");
				return false;
			}
			else if (array_type.is_pointer())
			{
				// Pointer types must point to objects derived from nap::Object
				if (!errorState.check(array_type.get_raw_type().is_derived_from<nap::Object>(), "Encountered pointer to non-Object. This is not supported"))
					return false;

				// Determine the target of the pointer
				std::string target;
				stream.readString(target);

				// Add to list of unresolved pointers
				if (!target.empty())
					unresolvedPointers.push_back(UnresolvedPointer(rootObject, rttiPath, target));
			}
			else if (RTTI::isPrimitive(array_type))
			{
				// Array of basic types; read basic type
				RTTI::Variant extracted_value = deserializePrimitive(array_type, stream);
				if (extracted_value.convert(array_type))
					array.set_value(index, extracted_value);
			}
			else
			{
				// Array-of-compounds; read object recursively
				RTTI::Variant var_tmp = array.get_value_as_ref(index);
				RTTI::Variant wrapped_var = var_tmp.extract_wrapped_value();
				if (!deserializeObjectRecursive(rootObject, wrapped_var, stream, rttiPath, unresolvedPointers, linkedFiles, errorState))
					return false;
			
				array.set_value(index, wrapped_var);
			}			

			// Remove array element from rtti path again
			rttiPath.popBack();
		}

		return true;
	}


	/**
	 * Helper function to recursively read an object (can be a nap::Object, nested compound or any other type) from JSON
	 */
	static bool deserializeObjectRecursive(nap::Object* object, RTTI::Instance compound, MemoryStream& stream, RTTI::RTTIPath& rttiPath, UnresolvedPointerList& unresolvedPointers, 
		std::vector<FileLink>& linkedFiles, ErrorState& errorState)
	{
		// Determine the object type. Note that we want to *most derived type* of the object.
		RTTI::TypeInfo object_type = compound.get_derived_type();

		// Go through all properties of the object
		for (const RTTI::Property& property : object_type.get_properties())
		{
			// Push attribute on path
			rttiPath.pushAttribute(property.get_name().data());

			// Determine meta-data for the property
			bool is_required = RTTI::hasFlag(property, RTTI::EPropertyMetaData::Required);
			bool is_file_link = RTTI::hasFlag(property, RTTI::EPropertyMetaData::FileLink);

			const RTTI::TypeInfo value_type = property.get_type();

			// If this is a file link, make sure it's of the expected type (string)
			if (!errorState.check((is_file_link && value_type.get_raw_type().is_derived_from<std::string>()) || !is_file_link, "Encountered a non-string file link. This is not supported"))
				return false;

			// If this is an array, read its elements recursively again (in case of nested compounds)
			if (value_type.is_array())
			{				
				// Get instance of the current value (this is a copy) and create an array view on it so we can fill it
				RTTI::Variant value = property.get_value(compound);
				RTTI::VariantArray array_view = value.create_array_view();

				// Now read the array recursively into array view
				if (!deserializeArrayRecursively(object, array_view, stream, rttiPath, unresolvedPointers, linkedFiles, errorState))
					return false;

				// Now copy the read array back into the target object
				property.set_value(compound, value);
			}
			else if (value_type.is_associative_container())
			{
				// Maps not supported (yet)
				errorState.fail("Encountered currently unsupported associative property %s", property.get_type().get_name().data());
				return false;
			}
			else if (value_type.is_pointer())
			{
				// Pointer types must point to objects derived from nap::Object
				if (!errorState.check(value_type.get_raw_type().is_derived_from<nap::Object>(), "Encountered pointer to non-Object. This is not supported"))
					return false;

				// Determine the target of the pointer
				std::string target;
				stream.readString(target);

				// If the target is empty (i.e. null pointer), but the property is required, throw an error
				if (!errorState.check((is_required && !target.empty()) || (!is_required), "Required property %s not found in object of type %s", property.get_name().data(), object_type.get_name().data()))
					return false;

				// Add to list of unresolved pointers
				if (!target.empty())
					unresolvedPointers.push_back(UnresolvedPointer(object, rttiPath, target));
			}
			else if (RTTI::isPrimitive(value_type))
			{
				// Basic JSON type, read value and copy to target
				RTTI::Variant extracted_value = deserializePrimitive(value_type, stream);
				if (extracted_value.convert(value_type))
					property.set_value(compound, extracted_value);
			}
			else
			{
				// If the property is a nested compound, read it recursively
				RTTI::Variant var = property.get_value(compound);
				if (!deserializeObjectRecursive(object, var, stream, rttiPath, unresolvedPointers, linkedFiles, errorState))
					return false;

				// Copy read object back into the target object
				property.set_value(compound, var);
			}

			// If this property is a file link, add it to the list of file links
			if (is_file_link)
			{
				FileLink file_link;
				file_link.mSourceObjectID	= compound.try_convert<Object>()->mID;
				file_link.mTargetFile		= property.get_value(compound).get_value<std::string>();;
				linkedFiles.push_back(file_link);
			}

			rttiPath.popBack();
		}

		return true;
	}

	bool deserializeBinary(MemoryStream& stream, RTTIDeserializeResult& result, ErrorState& errorState)
	{
		if (!errorState.check(!stream.isDone(), "Can't deserialize from empty stream"))
			return false;

		if (!errorState.check(readAndCheckRTTIBinaryVersion(stream), "Can't deserialize binary; RTTIBinaryVersion mismatch"))
			return false;

		// Continue reading while there's data in the stream
		while (!stream.isDone())
		{
			// Read type of the object
			std::string object_type;
			stream.readString(object_type);

			RTTI::TypeInfo type_info = RTTI::TypeInfo::get_by_name(object_type);
			if (!errorState.check(type_info.is_valid(), "Unknown object type %s encountered.", object_type.c_str()))
				return false;

			// Check whether this is a type that can actually be instantiated
			if (!errorState.check(type_info.can_create_instance(), "Unable to instantiate object of type %s.", object_type.c_str()))
				return false;

			// We only support root-level objects that derive from nap::Object (compounds, etc can be of any type)
			if (!errorState.check(type_info.is_derived_from(RTTI_OF(nap::Object)), "Unable to instantiate object %s. Class is not derived from Object.", object_type.c_str()))
				return false;

			// Check version
			std::size_t version = stream.read<std::size_t>();
			if (!errorState.check(version == RTTI::getRTTIVersion(type_info), "Type %s found that does not match the expected version (perhaps the type has changed?). Re-export the binary to fix this issue.", object_type.c_str()))
				return false;

			// Create new instance of the object
			Object* object = type_info.create<Object>();
			result.mReadObjects.push_back(std::unique_ptr<Object>(object));

			// Recursively read properties, nested compounds, etc
			RTTI::RTTIPath path;
			if (!deserializeObjectRecursive(object, *object, stream, path, result.mUnresolvedPointers, result.mFileLinks, errorState))
				return false;
		}

		return true;
	}

	bool readBinary(const std::string& path, RTTIDeserializeResult& result, nap::ErrorState& errorState)
	{
		// Open the file
		std::ifstream in(path, std::ios::in | std::ios::binary);
		if (!errorState.check(in.good(), "Unable to open file %s", path.c_str()))
			return false;

		// Create buffer of appropriate size
		in.seekg(0, std::ios::end);
		size_t len = in.tellg();
		std::vector<uint8_t> buffer;
		buffer.resize(len);

		// Read all data
		in.seekg(0, std::ios::beg);
		in.read((char*)buffer.data(), len);
		in.close();

		MemoryStream stream(buffer.data(), buffer.size());
		if (!deserializeBinary(stream, result, errorState))
			return false;

		return true;
	}
}
