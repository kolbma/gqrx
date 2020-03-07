/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2013 Christian Lindner DL2VCL, Stefano Leucci.
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
#include "dockrxopt.h"

BookmarksTableModel::BookmarksTableModel(QObject *parent) :
    QAbstractTableModel(parent)
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
    BookmarkInfo &info = *m_bookmarks[index.row()];

    if (role == Qt::BackgroundColorRole)
    {
        QColor bg(info.getColor());
        bg.setAlpha(0x60);
        return bg;
    }
    else if(role == Qt::DisplayRole || role == Qt::EditRole)
    {
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
            {
                QString strTags;
                for(int iTag = 0; iTag < info.tags.size(); ++iTag)
                {
                    if(iTag != 0)
                    {
                        strTags.append(",");
                    }
                    TagInfo &tag = *info.tags[iTag];
                    strTags.append(tag.name);
                }
                return strTags;
            }
        case COL_INFO:
            return info.info;
        }
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

BookmarkInfo *BookmarksTableModel::getBookmarkAtRow(int row)
{
    return m_bookmarks[row];
}

int BookmarksTableModel::getBookmarksIndexForRow(int iRow)
{
  return m_mapRowToBookmarksIndex[iRow];
}

int BookmarksTableModel::rowCount(const QModelIndex& /* parent */) const
{
    return m_bookmarks.size();
}

bool BookmarksTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(role == Qt::EditRole)
    {
        BookmarkInfo &info = *m_bookmarks[index.row()];
        switch(index.column())
        {
        case COL_FREQUENCY:
            {
                info.frequency = value.toLongLong();
                emit dataChanged(index, index);
            }
            break;
        case COL_NAME:
            {
                info.name = value.toString();
                emit dataChanged(index, index);
                return true;
            }
            break;
        case COL_MODULATION:
            {
                Q_ASSERT(!value.toString().contains(Bookmarks::CSV_SEPARATOR)); // may not contain a comma because tablemodel is saved as comma-separated file (csv).
                if(DockRxOpt::IsModulationValid(value.toString()))
                {
                    info.modulation = value.toString();
                    emit dataChanged(index, index);
                }
            }
            break;
        case COL_BANDWIDTH:
            {
                info.bandwidth = value.toInt();
                emit dataChanged(index, index);
            }
            break;
        case COL_TAGS:
            {
                info.tags.clear();
                QString strValue = value.toString();
                QStringList strList = strValue.split(Bookmarks::TAG_SEPARATOR); // TODO handle this
                for(int i = 0; i < strList.size(); ++i)
                {
                    QString strTag = strList[i].trimmed();
                    info.tags.append(&Bookmarks::instance().findOrAddTag(strTag));
                }
                emit dataChanged(index, index);
                return true;
            }
            break;
        case COL_INFO:
            {
                info.info = value.toString();
                emit dataChanged(index, index);
                return true;
            }
        break;
        }
        return true; // return true means success
    }
    return false;
}

void BookmarksTableModel::update()
{
    m_bookmarks.clear();
    for(int iBookmark = 0, iRow = 0; iBookmark < Bookmarks::instance().size(); iBookmark++)
    {
        BookmarkInfo &info = Bookmarks::instance().getBookmark(iBookmark);

        bool bActive = false;
        for(int iTag = 0; iTag < info.tags.size(); ++iTag)
        {
            TagInfo &tag = *info.tags[iTag];
            if(tag.active)
            {
                bActive = true;
                break;
            }
        }
        if(bActive)
        {
            m_mapRowToBookmarksIndex[iRow] = iBookmark;
            m_bookmarks.append(&info);
            ++iRow;
        }
    }

    emit layoutChanged();
}
