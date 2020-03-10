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
    static QString toString(const QList<TagInfo *> &tagList);

    enum Variant
    {
        Filter,
        Selection
    };

    explicit BookmarksTagList(QWidget *parent = 0, bool bShowUntagged = true, Variant variant = Variant::Filter);
    QList<TagInfo *> getCheckedTags();
    //QString getSelectedTagsAsString();

    /**
     * @brief set checked/unchecked based on active state
     * @param tags
     */
    void setTagsCheckState(const QList<TagInfo*> &tags);
    //void setSelectedTagsAsString(const QString &strTags);

public slots:
    void addTag(const QUuid &id, const QString &name, Qt::CheckState checkstate = Qt::Checked,
                const QColor &color = TagInfo::DEFAULT_COLOR);
    void addNewTag();
    void changeColor(int row);
    void deleteSelectedTag();
    void deselectAll();
    void filterTags();
    void on_cellClicked(int row, int column);
    void on_itemChanged(QTableWidgetItem *item);
    void renameSelectedTag();
    void selectAll();
    void showContextMenu(const QPoint &pos);
    void toggleCheckedState(int row, int column);
    void updateTags();

private:
    bool       m_blockSlot;
    Bookmarks *m_bookmarks;
    bool       m_bShowUntagged;
    Variant    m_variant;

    inline TagInfo &getTagInfo(const QTableWidgetItem *pItem);

};

#endif // BOOKMARKSTAGLIST_H
