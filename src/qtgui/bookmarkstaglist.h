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
#ifndef BOOKMARKSTAGLIST_H
#define BOOKMARKSTAGLIST_H

#include <QTableWidget>

#include "bookmarks.h"

/**
 * @brief A QWidget containing the List of Bookmark-Tags
 */
class BookmarksTagList : public QTableWidget
{
    Q_OBJECT

public:
    explicit BookmarksTagList(QWidget *parent = 0, bool bShowUntagged = true);
    QString getSelectedTagsAsString();
    void setSelectedTagsAsString(const QString &strTags);
    void setSelectedTags(QList<TagInfo*> tags);

private:
    bool m_bShowUntagged;
    bool m_bUpdating; // TODO currently not used

public slots:
    void updateTags();
    void on_cellClicked(int row, int column);
    void changeColor(int row);
    void toggleCheckedState(int row, int column);
    void showContextMenu(const QPoint &pos);
    bool renameSelectedTag();
    void addNewTag();
    void addTag(QString name, Qt::CheckState checkstate = Qt::Checked, QColor color = TagInfo::DEFAULT_COLOR);
    void deleteSelectedTag();
    void deleteTag(const QString &name);
    void selectAll();
    void deselectAll();
};

#endif // BOOKMARKSTAGLIST_H
