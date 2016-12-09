#pragma once

#include "fileutils.h"
#include "logger.h"
#include "object.h"
#include "service.h"
#include "resource.h"

namespace nap
{
	/**
	 * An AssetManager deals with loading and caching resources.
	 * It provides a thin and easy to use interface to all AssetFactories.
	 */
	class ResourceManagerService : public Service
	{
		RTTI_ENABLE_DERIVED_FROM(Service)
	public:

		/**
		 * @param rootDir The root directory from which all assets will be loaded
		 */
		ResourceManagerService() = default;
		/**
		 * Change the current asset root dir from which each asset will be loaded.
		 * Immediately invalidate all existing cache such that the assets can be properly reloaded.
		 * @param dirname The relative or absolute directory
		 */
		void setAssetRoot(const std::string& dirname);

		/**
		 * @param path The resource path to be checked
		 * @return True if the asset can be loaded, false otherwise
		 */
		bool canLoad(const std::string& path);

		/**
		 * Get or load an asset based on the path (file path relative to the asset directory)
		 *
		 * Internally, this will forward the call to the most appropriate asset factory if the file needs to be loaded
		 * from disk.
		 *
		 * The resulting resource is managed by the ResourceManager (who thought?)
		 * @param path
		 * @return
		 */
		Resource* getResource(const std::string& path);

		template<typename T>
		T* getResource(const std::string& path) {
			return rtti_cast<T*>(getResource(path));
		}

        /**
         * Find the path of the specified resource
         * @param res
         * @return The resource path or an empty string if the resource wasn't found
         */
        std::string getResourcePath(const Resource& res) const;

		/**
		 * Purge cache and ensure all assets will be reloaded the next time they are accessed
		 */
		void invalidate();

		/**
		 * Base on the provided path, find a suitable ResourceFactory
		 * @param path The relative (or absolute) path or URI
		 * @return A suitable ResourceFactory or nullptr if none was found
		 */
		ResourceLoader* getLoaderFor(const std::string& path);

		/**
		 * @return all registered factories
		 */
		std::vector<ResourceLoader*> getLoaders();

	private:
		/**
		 * Find an appropriate AssetFactory and let it load the asset
		 */
		Resource* loadResource(const std::string& path);



		/**
		 * Retrieve an existing factory or create a new one and return that one.
		 * @param factoryType The factory type to get or create
		 * @return An AssetFactory or nullptr when the creation failed
		 */
		ResourceLoader* getOrCreateFactory(const RTTI::TypeInfo& factoryType);

		/**
		 * Retrieve a factory based on a type
		 * @param factoryType
		 * @return An AssetFactory instance or nullptr if the factory wasn't found.
		 */
		ResourceLoader* getFactory(const RTTI::TypeInfo& factoryType);

		/**
		 * Create and register an AssetFactory based on the given type
		 * @param factoryType The type to be instantiated
		 */
		ResourceLoader* createFactory(const RTTI::TypeInfo& factoryType);

	private:
		std::vector<std::unique_ptr<ResourceLoader>> mFactories;
		std::string mResourcePath = "resources";
		std::map<std::string, std::unique_ptr<Resource>> mResources;
	};

}

RTTI_DECLARE(nap::ResourceManagerService)