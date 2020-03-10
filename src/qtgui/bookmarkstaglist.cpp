/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2014 Stefano Leucci, Christian Lindner DL2VCL.
 * Copyright 2020 Markus Kolb
 *
 * Gqrx is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * Gqrx is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Gqrx; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <QColorDialog>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>

#include "bookmarks.h"
#include "bookmarkstaglist.h"

BookmarksTagList::BookmarksTagList(QWidget *parent, bool bShowUntagged, Variant variant)
    : QTableWidget(parent),
      m_bookmarks(&Bookmarks::instance()),
      m_bShowUntagged(bShowUntagged),
      m_variant(variant)
{
    connect(this, SIGNAL(cellClicked(int, int)), this, SLOT(on_cellClicked(int, int)));
    connect(this, SIGNAL(itemChanged(QTableWidgetItem *)), this, SLOT(on_itemChanged(QTableWidgetItem *)));

    // right click menu
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(showContextMenu(const QPoint&)));

    //horizontalHeader()->setVisible(false);
    horizontalHeader()->setStretchLastSection(true);

    verticalHeader()->setVisible(false);

    setColumnCount(2);
    setColumnWidth(0, 20);

    setHorizontalHeaderItem(0, new QTableWidgetItem(""));
    setHorizontalHeaderItem(1, new QTableWidgetItem("Tags"));

    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    //setSortingEnabled(true);
    // TODO remove commented
}

QList<TagInfo *> BookmarksTagList::getCheckedTags()
{
    QList<TagInfo *> tags;

    int iRows = rowCount();
    for(int i = 0; i < iRows; ++i)
    {
        QTableWidgetItem *pItem = item(i, 1);
        const QUuid id = pItem->data(Bookmarks::ID_ROLE).value<QUuid>();
        auto &tagInfo = m_bookmarks->getTagInfo(id);
        if (m_variant == Variant::Filter)
        {
            tagInfo.show = (pItem->checkState() == Qt::Checked);
            tags.append(&tagInfo);
        }
        else if (m_variant == Variant::Selection)
        {
            if (tagInfo.checked != (pItem->checkState() == Qt::Checked))
            {
                tagInfo.checked = (pItem->checkState() == Qt::Checked);
                tagInfo.modified = true;
            }
            if (tagInfo.checked)
                tags.append(&tagInfo);
        }
    }

    if (m_variant == Variant::Selection && tags.count() == 0)
    {
        tags.append(&m_bookmarks->findOrAddTag(TagInfo::UNTAGGED));
    }

    return tags;
}

void BookmarksTagList::setTagsCheckState(const QList<TagInfo*> &tags)
{
    const int iRows = rowCount();
    for(int i=0; i < iRows; ++i)
    {
        bool checked = false;
        QTableWidgetItem *pItem = item(i, 1);
        const QUuid id = pItem->data(Bookmarks::ID_ROLE).value<QUuid>();
        for(auto it = tags.begin(), itend = tags.end(); it != itend; ++it)
        {
            if ((*it)->id == id)
            {
                if (m_variant == Variant::Filter)
                {
                    checked = (*it)->show;
                    break;
                }
                else if (m_variant == Variant::Selection)
                {
                    // we get here only the BookmarkInfo tags and can set to true
                    checked = true;
                    (*it)->checked = true;
                    break;
                }
                Q_ASSERT(false);
            }
        }
        pItem->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    }
    setSortingEnabled(true); // TODO sorting
}

void BookmarksTagList::addTag(const QUuid &id, const QString &name, Qt::CheckState checkstate, const QColor &color)
{
    int i = rowCount();
    setRowCount(i + 1);

    // Column 1
    QTableWidgetItem *item = new QTableWidgetItem(name);
    item->setData(Bookmarks::ID_ROLE, id);
    item->setCheckState(checkstate);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
    setItem(i, 1, item);

    // Column 0
    item = new QTableWidgetItem();
    item->setData(Bookmarks::ID_ROLE, id);
    item->setFlags(Qt::ItemIsEnabled);
    item->setBackgroundColor(color);
    setItem(i, 0, item);
}

void BookmarksTagList::addNewTag()
{
    TagInfo tagInfo;
    m_bookmarks->addTagInfo(tagInfo);

    addTag(tagInfo.id, "*enter tag name*", tagInfo.checked ? Qt::Checked : Qt::Unchecked);
    scrollToBottom();

    const int rowcount = rowCount();
    const auto pItem = item(rowcount - 1, 1);
    editItem(pItem);
}

void BookmarksTagList::changeColor(int row)
{
    auto &tagInfo = getTagInfo(item(row, 1));
    const QColor color = QColorDialog::getColor(tagInfo.color, this);

    m_bookmarks->setTagColor(tagInfo, color);
}

void BookmarksTagList::deleteSelectedTag()
{
    const QModelIndexList selected = selectionModel()->selectedRows();
    if(selected.empty())
        return;

    if (QMessageBox::question(this, "Delete tag", "Really delete?",
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        const int iRow = selected.first().row();
        const QTableWidgetItem *pItem = item(iRow, 1);
        auto &tagInfo = getTagInfo(pItem);
        m_bookmarks->removeTagInfo(tagInfo);
    }
}

void BookmarksTagList::deselectAll()
{
    int iRows = rowCount();
    for(int i=0; i < iRows; ++i)
    {
        QTableWidgetItem *pItem = item(i, 1);
        pItem->setCheckState(Qt::Unchecked);
        auto &tagInfo = getTagInfo(pItem);
        if (m_variant == Variant::Filter)
        {
            m_bookmarks->setTagShow(tagInfo, false);
        }
        else if (m_variant == Variant::Selection)
        {
            m_bookmarks->setTagChecked(tagInfo, false);
        }
    }
}

void BookmarksTagList::filterTags()
{
    updateTags();
    emit m_bookmarks->bookmarksChanged();
}

void BookmarksTagList::on_cellClicked(int row, int column)
{
    if(column == 0)
    {
        changeColor(row);
    }
    else if(column == 1)
    {
        toggleCheckedState(row, column);
    }
}

void BookmarksTagList::on_itemChanged(QTableWidgetItem *item)
{
    const QString text(item->text().trimmed());
    auto &tagInfo = getTagInfo(item);
    if (m_blockSlot || item->column() == 0 || tagInfo.name == text)
    {
        return;
    }
    else if (text.isEmpty() || tagInfo.name == TagInfo::UNTAGGED)
    {
        item->setText(tagInfo.name);
        return;
    }

    if (m_bookmarks->getTagList().indexOf(text) >= 0)
    {
        m_blockSlot = true;
        item->setText(tagInfo.name);
        m_blockSlot = false;
        return;
    }

    tagInfo.name = text;
    tagInfo.modified = true;
    emit m_bookmarks->tagListChanged();
    emit m_bookmarks->bookmarksChanged();
}

void BookmarksTagList::renameSelectedTag()
{
    QModelIndexList selected = selectionModel()->selectedRows();

    if(!selected.empty())
        editItem(item(selected.first().row(), 1));
}

void BookmarksTagList::selectAll()
{
    int iRows = rowCount();
    for(int i=0; i < iRows; ++i)
    {
        QTableWidgetItem *pItem = item(i, 1);
        pItem->setCheckState(Qt::Checked);
        auto &tagInfo = getTagInfo(pItem);
        if (m_variant == Variant::Filter)
        {
            m_bookmarks->setTagShow(tagInfo, true);
        }
        else if (m_variant == Variant::Selection)
        {
            m_bookmarks->setTagChecked(tagInfo, true);
        }
    }
}

void BookmarksTagList::showContextMenu(const QPoint &pos)
{
    QMenu *menu = new QMenu(this);

    // MenuItem "Rename"
    {
        QAction *action = new QAction("Rename", this);
        menu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(renameSelectedTag()));
    }

    // MenuItem "Create new Tag"
    {
        QAction *action = new QAction("Create new Tag", this);
        menu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(addNewTag()));
    }

    // Menu "Delete Tag"
    {
        QAction *action = new QAction("Delete Tag", this);
        menu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(deleteSelectedTag()));
    }

    // Menu "Select All"
    {
        QAction *action = new QAction("Select All", this);
        menu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(selectAll()));
    }

    // Menu "Deselect All"
    {
        QAction *action = new QAction("Deselect All", this);
        menu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(deselectAll()));
    }

    menu->popup(viewport()->mapToGlobal(pos));
}

void BookmarksTagList::toggleCheckedState(int row, int column)
{
    const QTableWidgetItem *pItem = item(row, column);
    auto &tagInfo = getTagInfo(pItem);
    if (m_variant == Variant::Filter)
    {
        m_bookmarks->setTagShow(tagInfo, pItem->checkState() != Qt::Checked);
    }
    else if (m_variant == Variant::Selection)
    {
        m_bookmarks->setTagChecked(tagInfo, pItem->checkState() != Qt::Checked);
    }
}

// TODO sorting
void BookmarksTagList::updateTags()
{
    m_blockSlot = true;

    // Get current List of Tags
    const QList<TagInfo> &tagList = m_bookmarks->getTagList();

    // Rebuild List in GUI
    clearContents();
    setSortingEnabled(false);
    setRowCount(0);

    for (auto it = tagList.cbegin(), it_end = tagList.cend(); it != it_end; it++)
    {
        if (it->name != TagInfo::UNTAGGED || m_bShowUntagged)
        {
            bool checked = false;
            if (m_variant == Variant::Filter)
            {
                checked = it->show;
            }
            else if (m_variant == Variant::Selection)
            {
                checked = it->checked;
            }
            addTag(it->id,
                   it->name,
                   checked ? Qt::Checked : Qt::Unchecked,
                   it->color);
        }
    }

    setSortingEnabled(true);

    m_blockSlot = false;
}

inline TagInfo &BookmarksTagList::getTagInfo(const QTableWidgetItem *pItem)
{
    const QUuid id = pItem->data(Bookmarks::ID_ROLE).value<QUuid>();
    return m_bookmarks->getTagInfo(id);
}
