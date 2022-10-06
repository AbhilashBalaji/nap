/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "inspectorpanel.h"
#include "appcontext.h"
#include "commands.h"
#include "napkinglobals.h"
#include "standarditemsproperty.h"
#include "naputils.h"
#include "napkinutils.h"
#include "napkin-resources.h"

#include <QApplication>
#include <QMimeData>
#include <QScrollBar>

#include <utility/fileutils.h>
#include <napqt/filterpopup.h>
#include <nap/group.h>
#include <fcurve.h>

using namespace nap::rtti;
using namespace napkin;

void InspectorModel::setPath(const PropertyPath& path)
{
	mPath = path;
	clearItems();
	populateItems();
}

const PropertyPath& InspectorModel::path() const
{
	return mPath;
}

void InspectorModel::clearItems()
{
	removeRows(0, rowCount());
}

InspectorModel::InspectorModel() : QStandardItemModel()
{
	setHorizontalHeaderLabels({TXT_LABEL_NAME, TXT_LABEL_VALUE, TXT_LABEL_TYPE});
}

Qt::DropActions InspectorModel::supportedDragActions() const
{
	return Qt::MoveAction;
}

Qt::DropActions InspectorModel::supportedDropActions() const
{
	return Qt::MoveAction;
}


InspectorPanel::InspectorPanel() : mTreeView(new QTreeView())
{
	setLayout(&mLayout);
	layout()->setContentsMargins(0, 0, 0, 0);

	auto font = mTitle.font();
	font.setPointSize(14);
	mTitle.setFont(font);
	mSubTitle.setAlignment(Qt::AlignRight);

	mHeaderLayout.addWidget(&mTitle);
	mHeaderLayout.addWidget(&mSubTitle);
	mLayout.addLayout(&mHeaderLayout);
	mHeaderLayout.setContentsMargins(0, 6, 0, 0);

	mLayout.addWidget(&mTreeView);
	mTreeView.setModel(&mModel);
	mTreeView.getTreeView().setColumnWidth(0, 250);
	mTreeView.getTreeView().setColumnWidth(1, 250);
	mTreeView.getTreeView().setItemDelegateForColumn(1, &mWidgetDelegate);
	mTreeView.getTreeView().setDragEnabled(true);

	mTreeView.setMenuHook(std::bind(&InspectorPanel::onItemContextMenu, this, std::placeholders::_1));

	connect(&AppContext::get(), &AppContext::propertySelectionChanged, this, &InspectorPanel::onPropertySelectionChanged);
	connect(&AppContext::get(), &AppContext::documentClosing, this, &InspectorPanel::onFileClosing);
	connect(&AppContext::get(), &AppContext::objectRenamed, this, &InspectorPanel::onObjectRenamed);
	connect(&AppContext::get(), &AppContext::serviceConfigurationClosing, this, &InspectorPanel::onFileClosing);
	connect(&mModel, &InspectorModel::childAdded, this, &InspectorPanel::onChildAdded);

	mPathLabel.setText("Path:");
	mSubHeaderLayout.addWidget(&mPathLabel);
	mPathField.setReadOnly(true);
	mSubHeaderLayout.addWidget(&mPathField);

	mLayout.addLayout(&mSubHeaderLayout);

}

void InspectorPanel::onItemContextMenu(QMenu& menu)
{
	// Get property path item
	auto path_item = qitem_cast<PropertyPathItem*>(mTreeView.getSelectedItem());
	if (path_item == nullptr)
		return;
		
	// In Array?
	auto parent_item = path_item->parentItem();
	auto parent_array_item = qobject_cast<ArrayPropertyItem*>(parent_item);
	if (parent_array_item != nullptr)
	{
		// Remove
		auto parent_array_item = static_cast<ArrayPropertyItem*>(parent_item);
		PropertyPath parent_property = parent_array_item->getPath();
		long element_index = path_item->row();

		// Construct label based on array type
		QString label("Remove ");
		if (path_item->getPath().isPointer())
		{
			auto pointee = path_item->getPath().getPointee();
			assert(pointee != nullptr);
			label += pointee->mID.c_str();
		}
		else
		{
			auto array_type = parent_array_item->getPath().getArrayElementType();
			label += array_type.get_name().data();
		}

		menu.addAction(AppContext::get().getResourceFactory().getIcon(QRC_ICONS_REMOVE), label, [parent_property, element_index]()
			{
				AppContext::get().executeCommand(new ArrayRemoveElementCommand(parent_property, element_index));
			});
	}

	// File link?
	auto& path = path_item->getPath();
	const auto& type = path.getType();
	const auto& prop = path.getProperty();
	if (type.is_derived_from<std::string>() && nap::rtti::hasFlag(prop, nap::rtti::EPropertyMetaData::FileLink))
	{
		bool ok;
		std::string filename = path_item->getPath().getValue().to_string(&ok);
		if (nap::utility::fileExists(filename))
		{
			menu.addAction("Show file in " + nap::qt::fileBrowserName(), [filename]()
			{
				nap::qt::revealInFileBrowser(QString::fromStdString(filename));
			});
			menu.addAction("Open in external editor", [filename]()
			{
				nap::qt::openInExternalEditor(QString::fromStdString(filename));
			});
		}

	}

	// Instance property?
	if (path.isInstanceProperty() && path.isOverridden())
	{
		menu.addAction(AppContext::get().getResourceFactory().getIcon(QRC_ICONS_REMOVE), 
			"Remove override", [path]()
			{
				PropertyPath p = path;
				p.removeOverride();
			});
	}

	// Pointer?
	if (qobject_cast<PointerItem*>(path_item) != nullptr)
	{
		nap::rtti::Object* pointee = path_item->getPath().getPointee();
		QAction* action = menu.addAction(AppContext::get().getResourceFactory().getIcon(*pointee),
			"Select Resource", [pointee]
		{
			QList<nap::rtti::Object*> objects = {pointee};
			AppContext::get().selectionChanged(objects);
		});
		action->setEnabled(pointee != nullptr);
	}

	// Embedded pointer?
	if (qobject_cast<EmbeddedPointerItem*>(path_item) != nullptr)
	{
		nap::rtti::Object* pointee = path_item->getPath().getPointee();
		auto path = path_item->getPath();
		auto type = path.getWrappedType();

		if (pointee != nullptr)
		{
			QString label = QString("Replace %1").arg(pointee->mID.c_str());
			menu.addAction(AppContext::get().getResourceFactory().getIcon(QRC_ICONS_CHANGE), label, [this, path, type]
				{
					TypePredicate predicate = [&type](auto t) { return t.is_derived_from(type); };
					rttr::type chosenType = showTypeSelector(this, predicate);
					if (!chosenType.is_valid())
						return;

					path.getDocument()->executeCommand(new ReplaceEmbeddedPointerCommand(path, chosenType));
				});

			// Only add option to delete if not in array.
			if (parent_array_item == nullptr)
			{
				label = QString("Delete %1").arg(pointee->mID.c_str());
				menu.addAction(AppContext::get().getResourceFactory().getIcon(QRC_ICONS_DELETE), label, [path_item, pointee]
					{
						// TODO: Make this a command
						auto doc = path_item->getPath().getDocument();
						auto pointeePath = PropertyPath(*pointee, *doc);
						auto ownerPath = doc->getEmbeddedObjectOwnerPath(*pointeePath.getObject());
						doc->removeObject(*pointeePath.getObject());
						if (ownerPath.isValid())
						{
							doc->propertyValueChanged(ownerPath);
						}
					});
			}
		}
		else
		{
			QString label = QString("Create %1...").arg(type.get_raw_type().get_name().data());
			menu.addAction(AppContext::get().getResourceFactory().getIcon(type), label, [this, path, type]
				{
					// TODO: Make this a command
					TypePredicate predicate = [&type](auto t) { return t.is_derived_from(type); };
					rttr::type chosenType = showTypeSelector(this, predicate);
					if (!chosenType.is_valid())
						return;
					path.getDocument()->executeCommand(new ReplaceEmbeddedPointerCommand(path, chosenType));
				});
		}
	}

	// Array?
	auto* array_item = qobject_cast<ArrayPropertyItem*>(path_item);
	if (array_item != nullptr && array_item->getPath().getArrayEditable())
	{
		PropertyPath array_path = path_item->getPath();
		auto array_type = array_path.getArrayElementType();
		if (array_path.isNonEmbeddedPointer())
		{
			// Build 'Add Existing' menu, populated with all existing objects matching the array type
			QString label = QString("Add %1...").arg(QString::fromUtf8(array_type.get_raw_type().get_name().data()));
			menu.addAction(AppContext::get().getResourceFactory().getIcon(QRC_ICONS_ADD), label, [this, array_path, array_type]()
			{
				auto objects = AppContext::get().getDocument()->getObjects(array_type);
				nap::rtti::Object* selected_object = showObjectSelector(this, objects);
				if (selected_object != nullptr)
					AppContext::get().executeCommand(new ArrayAddExistingObjectCommand(array_path, *selected_object));
			});

		}
		else if (array_path.isEmbeddedPointer())
		{
			QString label = QString("Add %1...").arg(QString::fromUtf8(array_type.get_raw_type().get_name().data()));
			menu.addAction(AppContext::get().getResourceFactory().getIcon(QRC_ICONS_ADD) , label, [this, array_path, array_type]()
			{
				TypePredicate predicate = [array_type](auto t) { return t.is_derived_from(array_type); };
				rttr::type elementType = showTypeSelector(this, predicate);
				if (elementType.is_valid())
					AppContext::get().executeCommand(new ArrayAddNewObjectCommand(array_path, elementType));
			});
		}
		else
		{
			QString label = QString("Add %1").arg(QString::fromUtf8(array_type.get_raw_type().get_name().data()));
			menu.addAction(AppContext::get().getResourceFactory().getIcon(QRC_ICONS_ADD), label, [array_path]()
			{
				AppContext::get().executeCommand(new ArrayAddValueCommand(array_path));
			});

		}
	}
}


void InspectorPanel::setPath(const PropertyPath& path)
{
	// Clear everything
	clear();

	// Bail if path isn't valid
	mPath = path;
	if (!path.isValid())
		return;

	// Update title and subtitle
	mPathField.setText(QString::fromStdString(path.toString()));
	mTitle.setText(path.getName().c_str());
	mSubTitle.setText(path.getType().get_name().data());

	// These types are excluded from property editing
	static const std::vector<nap::rtti::TypeInfo> typeExceptions
	{
		RTTI_OF(nap::IGroup),
		RTTI_OF(nap::math::FloatFCurve)
	};

	// Check if path is an exception
	auto path_obj = path.getObject();
	assert(path_obj != nullptr); auto obj_type = path_obj->get_type();
	auto it = std::find_if(typeExceptions.begin(), typeExceptions.end(), [&obj_type](const nap::rtti::TypeInfo& typeException)
		{
			return obj_type.is_derived_from(typeException);
		});

	// Add if not an exception
	if (it == typeExceptions.end())
	{
		mModel.setPath(path);
		expandTree(QModelIndex());
	}
}


void InspectorPanel::clear()
{
	mModel.clearItems();
	mPathField.setText("");
	mTitle.setText("");
	mSubTitle.setText("");
}


void napkin::InspectorPanel::expandTree(const QModelIndex& parent)
{
	// Get parent item
	QStandardItem* parent_item = parent.isValid() ? mModel.itemFromIndex(parent) : nullptr;
	for (int r = 0; r < mModel.rowCount(parent); r++)
	{
		// Get child item
		QModelIndex child_index = mModel.index(r, 0, parent);
		QStandardItem* child_item =  mModel.itemFromIndex(child_index);
		assert(child_item != nullptr);

		// Don't expand when item in array
		if(qitem_cast<ArrayPropertyItem*>(parent_item) != nullptr)
			continue;

		// Don't expand embedded resources
		if(qitem_cast<EmbeddedPointerItem*>(child_item) != nullptr)
			continue;

		// Don't expand color
		CompoundPropertyItem* compound = qitem_cast<CompoundPropertyItem*>(child_item);
		if (compound != nullptr && compound->getPath().isColor())
			continue;

		// Do expand the rest
		auto mapped_idx = mTreeView.getProxyModel().mapFromSource(child_index);
		mTreeView.getTreeView().expand(mapped_idx);

		// Repeat
		expandTree(child_index);
	}
}


void napkin::InspectorPanel::onChildAdded(QList<QStandardItem*> items)
{
	assert(items.size() > 0);
	auto parent = items.first()->parent();
	if (qitem_cast<const ArrayPropertyItem*>(parent) != nullptr)
	{
		mTreeView.select(items[0], false);
	}
}


void napkin::InspectorPanel::onFileClosing(const QString& filename)
{
	mModel.clearPath();
	clear();
}


void napkin::InspectorPanel::onObjectRenamed(nap::rtti::Object& object, const std::string& oldName, const std::string& newName)
{
	// Update path if object that was renamed is currently referenced
	if (mPath.referencesObject(oldName))
	{
		mPath.updateObjectName(oldName, newName);
		mPathField.setText(QString::fromStdString(mPath.toString()));
		mTitle.setText(mPath.getName().c_str());
	}
}


void InspectorPanel::onPropertySelectionChanged(const PropertyPath& prop)
{
	QList<nap::rtti::Object*> objects = {prop.getObject()};
	AppContext::get().selectionChanged(objects);
	auto pathItem = nap::qt::findItemInModel(mModel, [prop](QStandardItem* item)
	{
		auto pitem = qitem_cast<PropertyPathItem*>(item);
		return pitem != nullptr ? pitem->getPath() == prop : false;
	});
	mTreeView.select(pathItem, true);
}


void InspectorPanel::onObjectRemoved(Object* obj)
{
	// If the currently edited object is being removed, clear the view
	if (obj == mModel.path().getObject())
		setPath({});
}


void napkin::InspectorModel::clearPath()
{
	mPath = PropertyPath();
	clearItems();
}


bool InspectorModel::isPropertyIgnored(const PropertyPath& prop) const
{
	return prop.getName() == nap::rtti::sIDPropertyName;
}


void napkin::InspectorModel::onChildAdded(const QList<QStandardItem*> items)
{
	childAdded(items);
}


void InspectorModel::populateItems()
{
	// Skip entities
	if (rtti_cast<nap::Entity>(mPath.getObject()) != nullptr)
		return;

	// Create items (and child items) for every property
	for (const auto& propPath : mPath.getChildren())
	{
		if (!isPropertyIgnored(propPath))
		{
			auto row = createPropertyItemRow(propPath);
			for (const auto& item : row)
			{
				auto path_item = qitem_cast<const PropertyPathItem*>(item);
				if (path_item != nullptr)
				{
					connect(path_item, &PropertyPathItem::childAdded, this, &napkin::InspectorModel::onChildAdded);
				}
			}
			appendRow(row);
		}
	}
}


QVariant InspectorModel::data(const QModelIndex& index, int role) const
{
	switch (role)
	{
	case Qt::UserRole:
	{
		auto value_item = qitem_cast<PropertyPathItem*>(itemFromIndex(index));
		if (value_item != nullptr)
		{
			return QVariant::fromValue(value_item->getPath());
		}
		break;
	}
	case Qt::ForegroundRole:
	{
		auto value_item = qitem_cast<PropertyPathItem*>(itemFromIndex(index));
		if (value_item != nullptr)
		{
			bool correct_item = qobject_cast<PointerValueItem*>(value_item) != nullptr ||
				qobject_cast<PropertyValueItem*>(value_item) != nullptr;

			if (value_item->getPath().isInstanceProperty() && correct_item)
			{
				auto& themeManager = AppContext::get().getThemeManager();
				if (value_item->getPath().isOverridden())
				{
					return QVariant::fromValue<QColor>(themeManager.getColor(theme::color::instancePropertyOverride));
				}
			}
		}
		break;
	}
	default:
		break;
	}
	return QStandardItemModel::data(index, role);
}


bool InspectorModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	return QStandardItemModel::setData(index, value, role);
}


nap::rtti::Object* InspectorModel::getObject()
{
	return mPath.getObject();
}


Qt::ItemFlags InspectorModel::flags(const QModelIndex& index) const
{
	auto flags = QStandardItemModel::flags(index);

	// First always disable dragging & dropping
	flags &= ~Qt::ItemIsDragEnabled;
	flags &= ~Qt::ItemIsDropEnabled;

	// Not an item? Early out
	auto item = itemFromIndex(index);
	if (item == nullptr)
		return flags;

	// Is this item an array element? Enable dragging
	if (qitem_cast<ArrayPropertyItem*>(item->parent()) != nullptr)
	{
		flags |= Qt::ItemIsDragEnabled;
	}

	// Is this item an array? Allow dropping
	if (qitem_cast<ArrayPropertyItem*>(item) != nullptr)
	{
		flags |= Qt::ItemIsDropEnabled;
	}
	return flags;
}


QMimeData* InspectorModel::mimeData(const QModelIndexList& indexes) const
{
	if (indexes.empty())
		return nullptr;

	auto mime_data = new QMimeData();

	QString mime_text;

	// As soon as the first valid item is found, use that, ignore subsequent items
	// TODO: Handle dragging multiple items
	for (auto index : indexes)
	{
		auto object_item = qitem_cast<PropertyPathItem*>(itemFromIndex(index));
		if (object_item == nullptr)
			continue;

		mime_text = QString::fromStdString(object_item->getPath().toString());
		break;
	}
	mime_data->setData(sNapkinMimeData, mime_text.toLocal8Bit());

	return mime_data;
}


QStringList InspectorModel::mimeTypes() const
{
	QStringList types;
	types << sNapkinMimeData;
	return types;
}

