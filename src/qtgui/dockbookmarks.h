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
#pragma once

#include <QDockWidget>
#include <QItemDelegate>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QTableWidgetItem>

#include "qtgui/bookmarkstablemodel.h"

namespace Ui {
    class DockBookmarks;
}

/**
 * @brief The ComboBoxDelegateModulation class
 */
class ComboBoxDelegateModulation : public QItemDelegate
{
Q_OBJECT
public:
  ComboBoxDelegateModulation(QObject *parent = 0);
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
  void setEditorData(QWidget *editor, const QModelIndex &index) const;
  void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
};

/**
 * @brief The DockBookmarks class
 */
class DockBookmarks : public QDockWidget
{
    Q_OBJECT

public:
    explicit DockBookmarks(QWidget *parent = 0);
    ~DockBookmarks();
    void readSettings(QSettings *settings);
    void saveSettings(QSettings *settings);

signals:
    void bookmarkModified();
    void newBookmarkActivated(qint64, QString, int);
    void newBookmarkAdd();
    void tagListModified();

public slots:
    void setNewFrequency(qint64 rx_freq);

private:
    QSortFilterProxyModel      *bookmarksSortModel;
    BookmarksTableModel        *bookmarksTableModel;
    QMenu                      *contextmenu;
    ComboBoxDelegateModulation *delegateModulation;
    Bookmarks                  *m_bookmarks;
    qint64                      m_currentFrequency;
    Ui::DockBookmarks          *ui;

    bool eventFilter(QObject* object, QEvent* event);
    void showTagsSelector(const QUuid &id);

private slots:
    void activated(const QModelIndex &index);
    void addBookmark();
    bool deleteSelectedBookmark();
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void onLayoutChanged();
    void resetSorting();
    void showContextMenu(const QPoint &pos);
    void emitTagListModified();
    void tagsDblClicked(const QModelIndex &index);
};
