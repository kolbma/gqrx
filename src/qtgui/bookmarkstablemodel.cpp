/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2013 Christian Lindner DL2VCL, Stefano Leucci.
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

#include "bookmarks.h"
#include "bookmarkstablemodel.h"
#include "bookmarkstaglist.h"
#include "dockrxopt.h"

BookmarksTableModel::BookmarksTableModel(QObject *parent) :
    QAbstractTableModel(parent),
    m_bookmarks(&Bookmarks::instance())
{
}

int BookmarksTableModel::columnCount(const QModelIndex& /* parent */) const
{
    return EColumns::COL_INFO + 1;
}

QVariant BookmarksTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch(section)
        {
        case COL_FREQUENCY:
            return QString("Frequency");
            break;
        case COL_NAME:
            return QString("Name");
            break;
        case COL_MODULATION:
            return QString("Modulation");
            break;
        case COL_BANDWIDTH:
            return QString("Bandwidth");
            break;
        case COL_TAGS:
            return QString("Tags");
            break;
        case COL_INFO:
            return QString("Info");
            break;
        }
    }
    if(orientation == Qt::Vertical && role == Qt::DisplayRole)
    {
        return section;
    }
    return QVariant();
}

QVariant BookmarksTableModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::BackgroundColorRole)
    {
        const BookmarkInfo &info = *m_bookmarkList[index.row()];
        QColor bg(info.getColor());
        bg.setAlpha(0x60);
        return bg;
    }
    else if(role == Qt::DisplayRole || role == Qt::EditRole)
    {
        const BookmarkInfo &info = *m_bookmarkList[index.row()];
        switch(index.column())
        {
        case COL_FREQUENCY:
            return info.frequency;
        case COL_NAME:
            return (role == Qt::EditRole) ? QString(info.name) : info.name;
        case COL_MODULATION:
            return info.modulation;
        case COL_BANDWIDTH:
            return (info.bandwidth == 0) ? QVariant("") : QVariant(info.bandwidth);
        case COL_TAGS:
            return info.tagsStr;
        case COL_INFO:
            return info.info;
        }
    }
    else if (role == Bookmarks::ID_ROLE)
    {
        const BookmarkInfo &info = *m_bookmarkList[index.row()];
        return info.id;
    }
    return QVariant();
}

Qt::ItemFlags BookmarksTableModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = 0;

    switch(index.column())
    {
    case COL_FREQUENCY:
        flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
        break;
    case COL_NAME:
        flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
        break;
    case COL_BANDWIDTH:
        flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
        break;
    case COL_MODULATION:
        flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
        break;
    case COL_TAGS:
        flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        break;
    case COL_INFO:
        flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
        break;
    }
    return flags;
}

const BookmarkInfo *BookmarksTableModel::getBookmark(int index) const
{
    return m_bookmarkList[index];
}

int BookmarksTableModel::rowCount(const QModelIndex& /* parent */) const
{
    return m_bookmarkList.count();
}

bool BookmarksTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(role == Qt::EditRole)
    {
        BookmarkInfo &info = *m_bookmarkList[index.row()];
        switch(index.column())
        {
        case COL_FREQUENCY:
            {
                const auto val = value.toLongLong();
                if (info.frequency != val)
                {
                    info.frequency = val;
                    emit dataChanged(index, index);
                }
            }
            break;
        case COL_NAME:
            {
                const auto val = value.toString();
                if (info.name != val)
                {
                    info.name = val;
                    emit dataChanged(index, index);
                }
            }
            break;
        case COL_MODULATION:
            {
                // may not contain a comma because no csvsplit()
                Q_ASSERT(!value.toString().contains(Bookmarks::CSV_SEPARATOR));
                const auto val = value.toString();
                if (info.modulation != val && DockRxOpt::IsModulationValid(val))
                {
                    info.modulation = val;
                    emit dataChanged(index, index);
                }
            }
            break;
        case COL_BANDWIDTH:
            {
                const auto val = value.toInt();
                if (info.bandwidth != val)
                {
                    info.bandwidth = val;
                    emit dataChanged(index, index);
                }
            }
            break;
        case COL_TAGS:
            {
                // info.tags should be set by dialog
                if (info.modified)
                {
                    emit dataChanged(index, index);
                }
            }
            break;
        case COL_INFO:
            {
                const auto val = value.toString();
                if (info.info != val)
                {
                    info.info = val;
                    emit dataChanged(index, index);
                }
            }
            break;
        }
        return true; // return true means success
    }
    return false;
}

void BookmarksTableModel::update()
{
    m_bookmarkList.clear();
    for(int i = 0, count = m_bookmarks->count(); i < count; i++)
    {
        auto &info = m_bookmarks->getBookmark(i);

        for (auto tag : info.tags)
        {
            if(tag->show)
            {
                info.setTags(info.tags); // just update tags
                m_bookmarkList.append(&info);
                break;
            }
        }
    }

    emit layoutChanged();
}
