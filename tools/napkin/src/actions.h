/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "appcontext.h"
#include "napkinglobals.h"
#include "napkin-resources.h"

#include <QAction>
#include <QFileDialog>
#include <QKeySequence>
#include <QSet>
#include <QStandardItem>
#include <QString>
#include <QUndoCommand>
#include <entity.h>
#include <nap/logger.h>
#include <nap/group.h>

namespace napkin
{
	class EntityItem;

	/**
	 * Base class for actions. Each subclass must implement the perform() method in which the actual work will be done.
	 * In many cases perform() will create an instance of an appropriate command and execute it.
	 */
	class Action : public QAction
	{
	public:
		/**
		 * @param text action description
		 * @param iconName name of icon to load, nullptr if there is no icon
		 */
		Action(const char* text, const char* iconName);
	protected:
		/**
		 * This method will be called if the action is triggered
		 */
		virtual void perform() = 0;

	private:
		void loadIcon();
		void onThemeChanged(const Theme* theme)		{ loadIcon(); }
		QString mIconName;
	};


	/**
	 * Create a new file.
	 */
	class NewFileAction : public Action
	{
	public:
		NewFileAction();
	private:
		void perform() override;
	};


	/**
	 * Display a file open dialog and open the file if confirmed.
	 */
	class OpenProjectAction : public Action
	{
	public:
		OpenProjectAction();
	private:
		void perform() override;
	};


	/**
	 * Reload current data file
	 */
	class ReloadFileAction : public Action
	{
	public:
		ReloadFileAction();
	private:
		void perform() override;
	};


	/**
	 * Save the currently open file, show a save file dialog if the file wasn't saved before.
	 */
	class SaveFileAction : public Action
	{
	public:
		SaveFileAction();
	private:
		void perform() override;
	};


	/**
	 * Present a save file dialog and store the file if confirmed.
	 */
	class SaveFileAsAction : public Action
	{
	public:
		SaveFileAsAction();
	private:
		void perform() override;
	};


	/**
	 * Presents a load file dialog, to load a different data file
	 */
	class OpenFileAction : public Action
	{
	public:
		OpenFileAction();
	private:
		void perform() override;
	};


	/**
	 * Updates project data path to point to current loaded document
	 */
	class UpdateDefaultFileAction : public Action
	{
	public:
		UpdateDefaultFileAction();
	private:
		void perform() override;
	};


	/**
	 * Creates a service configuration
	 */
	class NewServiceConfigAction : public Action
	{
	public:
		NewServiceConfigAction();
	private:
		void perform();
	};


	/**
	 * Saves a service configuration to disk
	 */
	class SaveServiceConfigAction : public Action
	{
	public: 
		SaveServiceConfigAction();
	private:
		void perform();
	};


	/**
	 * Saves a service configuration to disk
	 */
	class SaveServiceConfigurationAs : public Action
	{
	public:
		SaveServiceConfigurationAs();
	private:
		void perform();
	};


	/**
	 * Open a service configuration from disk
	 */
	class OpenServiceConfigAction : public Action
	{
	public:
		OpenServiceConfigAction();
	private:
		void perform();
	};


	/**
	 * Sets current service configuration to be project default
	 */
	class SetAsDefaultServiceConfigAction : public Action
	{
	public:
		SetAsDefaultServiceConfigAction();
	private:
		void perform();
	};


	/**
	 * Remove service configuration from project
	 */
	class ClearServiceConfigAction : public Action
	{
	public:
		ClearServiceConfigAction();
	private:
		void perform();
	};


	/**
	 * Create a Resource
	 */
	class CreateResourceAction : public Action
	{
	public:
		explicit CreateResourceAction();
	private:
		void perform() override;
	};


	/**
	 * Create a Resource Group 
	 */
	class CreateGroupAction : public Action
	{
	public:
		explicit CreateGroupAction();
	private:
		void perform() override;
	};


	/**
	 * Add a new resource to a group
	 */
	class CreateResourceGroupAction : public Action
	{
	public:
		explicit CreateResourceGroupAction(nap::IGroup& group);
	private:
		void perform() override;
		nap::IGroup* mGroup = nullptr;
	};


	/**
	 * Parents a resource under a to be selected group
	 * @param resource resource to parent under a group
	 * @param parentGroup current parent group, nullptr if there is no parent
	 */
	class MoveResourceToGroupAction : public Action
	{
	public:
		explicit MoveResourceToGroupAction(nap::Resource& resource, nap::IGroup* parentGroup);
	private:
		void perform() override;
		nap::IGroup* mParentGroup = nullptr;
		nap::Resource* mResource = nullptr;
	};


	/**
	 * Add an existing resource to a group.
	 * If the resource is not specified, a dialog to select a resource is presented.
	 * If the group is not specified, a dialog to select the group is presented
	 */
	class AddResourceToGroupAction : public Action
	{
	public:
		explicit AddResourceToGroupAction(nap::IGroup& group);
	private:
		void perform() override;
		nap::IGroup* mGroup = nullptr;
	};


	/**
	 * Removes a resource from a group, moving it to the root of the document
	 */
	class RemoveResourceFromGroupAction : public Action
	{
	public:
		explicit RemoveResourceFromGroupAction(nap::IGroup& group, nap::Resource& resource);
		void perform() override;
	private:
		nap::IGroup* mGroup = nullptr;
		nap::Resource* mResource = nullptr;

	};


	/**
	 * Create an Entity
	 */
	class CreateEntityAction : public Action
	{
	public:
		explicit CreateEntityAction();
	private:
		void perform() override;
	};


	/**
	 * Add an Entity as child of another Entity
	 */
	class AddChildEntityAction : public Action
	{
	public:
		explicit AddChildEntityAction(nap::Entity& entity);
	private:
		void perform() override;
		nap::Entity* mEntity;
	};


	/**
	 * Add a Component to an Entity
	 */
	class AddComponentAction : public Action
	{
	public:
		explicit AddComponentAction(nap::Entity& entity);
	private:
		void perform() override;
		nap::Entity* mEntity;
	};

	/**
	 * Delete a single object.
	 * If another object points to the object to delete, ask the user for confirmation.
	 */
	class DeleteObjectAction : public Action
	{
	public:
		explicit DeleteObjectAction(nap::rtti::Object& object);
	private:
		void perform() override;
		nap::rtti::Object& mObject;
	};


	/**
	 * Delete a group, including all children in the group.
	 * If another object points to any of the children in the group, ask the user for confirmation.
	 */
	class DeleteGroupAction : public Action
	{
	public:
		explicit DeleteGroupAction(nap::IGroup& group);
	private:
		void perform() override;
		nap::IGroup& mGroup;
	};


	/**
	 * Remove a child Entity from its parent
	 */
	class RemoveChildEntityAction : public Action
	{
	public:
		explicit RemoveChildEntityAction(EntityItem& entityItem);
	private:
		void perform() override;
		EntityItem* mEntityItem;
	};


	/**
	 * Remove something defined by the propertypath
	 */
	class RemovePathAction : public Action
	{
	public:
		explicit RemovePathAction(const PropertyPath& path);
	private:
		void perform() override;
		PropertyPath mPath;
	};


	/**
	 * Change the current theme. The name must match a theme name defined in the ThemeManager
	 */
	class SetThemeAction : public Action
	{
	public:
        explicit SetThemeAction(const QString& themeName);
	private:
		void perform() override;
		QString mTheme;	// The theme to set
	};
}
