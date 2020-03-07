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

#include "bookmarks.h"
#include "bookmarkstaglist.h"

BookmarksTagList::BookmarksTagList(QWidget *parent, bool bShowUntagged)
    : QTableWidget(parent),
      m_bShowUntagged(bShowUntagged),
      m_bUpdating(false)
{
    connect(this, SIGNAL(cellClicked(int, int)),
            this, SLOT(on_cellClicked(int, int)));

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

void BookmarksTagList::on_cellClicked(int row, int column)
{
    if(column == 0)
    {
        changeColor(row);
    }
    if(column == 1)
    {
        toggleCheckedState(row, column);
    }
}

void BookmarksTagList::changeColor(int row)
{
    TagInfo &info = Bookmarks::instance().findOrAddTag(item(row, 1)->text());
    QColor color = QColorDialog::getColor(info.color, this);

    if(!color.isValid())
        return;

    info.color = color;
    updateTags();
    Bookmarks::instance().save();
}

void BookmarksTagList::toggleCheckedState(int row, int column)
{
    QTableWidgetItem *p = item(row, column);
    if(p->checkState() == Qt::Unchecked)
        p->setCheckState(Qt::Checked);
    else
        p->setCheckState(Qt::Unchecked);
}

void BookmarksTagList::updateTags()
{
    m_bUpdating = true;

    // Remember which items were unchecked.
    QStringList unchecked;
    for(int i=0; i < rowCount(); i++)
    {
        if(item(i,1)->checkState() == Qt::Unchecked)
            unchecked.append(item(i, 1)->text());
    }

    // Get current List of Tags.
    QList<TagInfo> newTags = Bookmarks::instance().getTagList();
    if(!m_bShowUntagged)
    {
        for(int i=0; i < newTags.size(); ++i)
        {
            TagInfo &taginfo = newTags[i];
            if(taginfo.name.compare(TagInfo::UNTAGGED) == 0)
            {
                newTags.removeAt(i);
                break;
            }
        }
    }

    // Rebuild List in GUI.
    clearContents();
    setSortingEnabled(false);
    setRowCount(0);
    for(int i=0; i < newTags.count(); i++)
    {
        addTag(newTags[i].name,
               unchecked.contains(newTags[i].name) ? Qt::Unchecked : Qt::Checked,
               newTags[i].color);
    }
    setSortingEnabled(true);

    m_bUpdating = false;
}

void BookmarksTagList::setSelectedTagsAsString(const QString &strTags)
{
    QStringList list = strTags.split(",");
    int iRows = rowCount();
    for(int i=0; i < iRows; ++i)
    {
        QTableWidgetItem *pItem = item(i, 1);
        QString name = pItem->text();
        bool bChecked = list.contains(name);
        pItem->setCheckState(bChecked ? Qt::Checked : Qt::Unchecked);
    }
    setSortingEnabled(true);
}

void BookmarksTagList::setSelectedTags(QList<TagInfo*> tags)
{
    int iRows = rowCount();
    for(int i=0; i < iRows; ++i)
    {
        QTableWidgetItem *pItem = item(i,1);
        QString name = pItem->text();
        bool bChecked = false;
        for(QList<TagInfo*>::const_iterator it=tags.begin(), itend=tags.end(); it != itend; ++it)
        {
            TagInfo *pTag = *it;
            if(pTag->name == name)
                bChecked = true;
        }
        pItem->setCheckState(bChecked ? Qt::Checked : Qt::Unchecked);
    }
    setSortingEnabled(true);
}

QString BookmarksTagList::getSelectedTagsAsString()
{
    QString strResult;

    int iRows = rowCount();
    bool bFirst = true;
    for(int i=0; i < iRows; ++i)
    {
        QTableWidgetItem *pItem = item(i, 1);
        if(pItem->checkState() == Qt::Checked)
        {
            if(!bFirst)
                strResult += ", ";
            strResult += pItem->text();
            bFirst = false;
        }
    }
    return strResult;
}

void BookmarksTagList::showContextMenu(const QPoint &pos)
{
    QMenu *menu = new QMenu(this);  // TODO: check if there is a delete!!!

    // Rename currently does not work.
    // The problem is that after the tag name is changed in GUI
    // you can not find the right TagInfo because you dont know
    // the old tag name.
    // MenuItem "Rename"
    {
        QAction *actionRename = new QAction("Rename", this);
        menu->addAction(actionRename);
        connect(actionRename, SIGNAL(triggered()), this, SLOT(renameSelectedTag()));
    }

    // MenuItem "Create new Tag"
    {
        QAction *actionNewTag = new QAction("Create new Tag", this);
        menu->addAction(actionNewTag);
        connect(actionNewTag, SIGNAL(triggered()), this, SLOT(addNewTag()));
    }

    // Menu "Delete Tag"
    {
        QAction *actionDeleteTag = new QAction("Delete Tag", this);
        menu->addAction(actionDeleteTag);
        connect(actionDeleteTag, SIGNAL(triggered()), this, SLOT(deleteSelectedTag()));
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

bool BookmarksTagList::renameSelectedTag()
{
    QModelIndexList selected = selectionModel()->selectedRows();

    if(selected.empty())
        return true;

    int iRow = selected.first().row();
    QTableWidgetItem *pItem = item(iRow, 1);
    editItem(pItem);
    //Bookmarks::get().save();

    return true;
}

void BookmarksTagList::addNewTag()
{
    addTag("*enter tag name*");
    scrollToBottom();
    editItem(item(rowCount() - 1, 1));
}

void BookmarksTagList::addTag(QString name, Qt::CheckState checkstate, QColor color)
{
    int i = rowCount();
    setRowCount(i + 1);

    // Column 1
    QTableWidgetItem *item = new QTableWidgetItem(name);
    item->setCheckState(checkstate);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
    setItem(i, 1, item);

    // Column 0
    item = new QTableWidgetItem();
    item->setFlags(Qt::ItemIsEnabled);
    item->setBackgroundColor(color);
    setItem(i, 0, item);
}

void BookmarksTagList::deleteSelectedTag()
{
    QModelIndexList selected = selectionModel()->selectedRows();
    if(selected.empty())
        return;

    int iRow = selected.first().row();
    QTableWidgetItem *pItem = item(iRow, 1);
    QString strTagName = pItem->text();
    deleteTag(strTagName);
    return;
}

void BookmarksTagList::deleteTag(const QString &name)
{
    Bookmarks::instance().removeTag(name);
    updateTags();
}

void BookmarksTagList::selectAll()
{
    int iRows = rowCount();
    for(int i=0; i < iRows; ++i)
    {
        QTableWidgetItem *pItem = item(i, 1);
        QString name = pItem->text();
        pItem->setCheckState(Qt::Checked);
    }
}

void BookmarksTagList::deselectAll()
{
    int iRows = rowCount();
    for(int i=0; i < iRows; ++i)
    {
        QTableWidgetItem *pItem = item(i, 1);
        QString name = pItem->text();
        pItem->setCheckState(Qt::Unchecked);
    }
}
