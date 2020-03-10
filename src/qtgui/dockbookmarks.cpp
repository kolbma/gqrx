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

#include <QComboBox>
#include <QDialogButtonBox>
#include <QMenu>
#include <QMessageBox>

#include "bookmarkstaglist.h"
#include "dockbookmarks.h"
#include "dockrxopt.h"
#include "qtcolorpicker.h"
#include "ui_dockbookmarks.h"

/**
 * @brief ComboBoxDelegateModulation::ComboBoxDelegateModulation
 * @param parent
 */
ComboBoxDelegateModulation::ComboBoxDelegateModulation(QObject *parent)
    :QItemDelegate(parent)
{
}

QWidget *ComboBoxDelegateModulation::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */,
                                                  const QModelIndex &index) const
{
    QComboBox* comboBox = new QComboBox(parent); // handled by Qt with deleteLater
    for (int i = 0; i < DockRxOpt::ModulationStrings.size(); ++i)
    {
        comboBox->addItem(DockRxOpt::ModulationStrings[i]);
    }
    setEditorData(comboBox, index);
    return comboBox;
}

void ComboBoxDelegateModulation::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *comboBox = static_cast<QComboBox*>(editor);
    QString value = index.model()->data(index, Qt::EditRole).toString();
    int iModulation = DockRxOpt::GetEnumForModulationString(value);
    comboBox->setCurrentIndex(iModulation);
}

void ComboBoxDelegateModulation::setModelData(QWidget *editor, QAbstractItemModel *model,
                                              const QModelIndex &index) const
{
    const QComboBox *comboBox = static_cast<QComboBox*>(editor);
    model->setData(index, comboBox->currentText(), Qt::EditRole);
}

/**
 * @brief DockBookmarks::DockBookmarks
 * @param parent
 */
DockBookmarks::DockBookmarks(QWidget *parent) :
    QDockWidget(parent),
    m_bookmarks(&Bookmarks::instance()),
    ui(new Ui::DockBookmarks)
{
    ui->setupUi(this);

    bookmarksTableModel = new BookmarksTableModel();

    // Frequency List
    ui->tableViewFrequencyList->setModel(bookmarksTableModel);
    ui->tableViewFrequencyList->setColumnWidth(BookmarksTableModel::COL_NAME,
        ui->tableViewFrequencyList->columnWidth(BookmarksTableModel::COL_NAME) * 2);
    ui->tableViewFrequencyList->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableViewFrequencyList->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableViewFrequencyList->installEventFilter(this);

    // Demod Selection in Frequency List Table.
    delegateModulation = new ComboBoxDelegateModulation(this);
    ui->tableViewFrequencyList->setItemDelegateForColumn(2, delegateModulation);

    // Bookmarks Context menu
    contextmenu = new QMenu(this);
    // MenuItem Edit
    {
        QAction* action = new QAction("Edit Field", this);
        contextmenu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(editSelectedField()));
    }
    // MenuItem Delete
    {
        QAction* action = new QAction("Delete Bookmark", this);
        contextmenu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(deleteSelectedBookmark()));
    }
    // MenuItem Add
    {
        QAction* action = new QAction("Add Bookmark", this);
        contextmenu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(addBookmark()));
    }
    ui->tableViewFrequencyList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableViewFrequencyList, SIGNAL(customContextMenuRequested(const QPoint&)),
        this, SLOT(showContextMenu(const QPoint&)));

    // Update GUI // TODO do this on first show
    m_bookmarks->load();
    bookmarksTableModel->update();

    m_currentFrequency = 0;

    // TagList
    ui->tableWidgetTagList->updateTags();

    connect(ui->tableViewFrequencyList, SIGNAL(activated(const QModelIndex &)),
            this, SLOT(activated(const QModelIndex &)));
    connect(ui->tableViewFrequencyList, SIGNAL(doubleClicked(const QModelIndex &)),
            this, SLOT(tagsClicked(const QModelIndex &)));
    connect(bookmarksTableModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
            this, SLOT(onDataChanged(const QModelIndex &, const QModelIndex &)));
    connect(m_bookmarks, SIGNAL(tagListChanged()), ui->tableWidgetTagList, SLOT(updateTags()));
    connect(m_bookmarks, SIGNAL(tagListFilter()), ui->tableWidgetTagList, SLOT(filterTags()));
    connect(m_bookmarks, SIGNAL(bookmarksChanged()), bookmarksTableModel, SLOT(update()));
}

DockBookmarks::~DockBookmarks()
{
    delete bookmarksTableModel;
    delete delegateModulation;
    delete contextmenu;
}

void DockBookmarks::setNewFrequency(qint64 rx_freq)
{
    ui->tableViewFrequencyList->clearSelection();
    const int iRowCount = bookmarksTableModel->rowCount();
    for (int row = 0; row < iRowCount; ++row)
    {
        BookmarkInfo& info = *(bookmarksTableModel->getBookmarkAtRow(row));
        if (std::abs(rx_freq - info.frequency) <= ((info.bandwidth / 2 ) + 1))
        {
            ui->tableViewFrequencyList->selectRow(row);
            ui->tableViewFrequencyList->scrollTo(ui->tableViewFrequencyList->currentIndex(),
                                                 QAbstractItemView::EnsureVisible);
            break;
        }
    }
    m_currentFrequency = rx_freq;
}

bool DockBookmarks::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* pKeyEvent = static_cast<QKeyEvent *>(event);
        if (pKeyEvent->key() == Qt::Key_Delete && ui->tableViewFrequencyList->hasFocus())
        {
            return deleteSelectedBookmark();
        }
    }
    return QWidget::eventFilter(object, event);
}

void DockBookmarks::showTagsSelector(int row, int /*column*/)
{
    const int idx = bookmarksTableModel->getBookmarksIndexForRow(row);
    BookmarkInfo& bmi = m_bookmarks->getBookmark(idx);

    // Create and show the Dialog for a new Bookmark.
    QDialog dialog(this);
    dialog.setWindowTitle("Change Bookmark Tags");

    BookmarksTagList *taglist = new BookmarksTagList(&dialog, false, BookmarksTagList::Variant::Selection);
    taglist->updateTags();
    taglist->setTagsCheckState(bmi.tags);
    connect(m_bookmarks, SIGNAL(tagListChanged()), taglist, SLOT(updateTags()));

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                          | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    mainLayout->addWidget(taglist);
    mainLayout->addWidget(buttonBox);

    if (dialog.exec())
    {
        bmi.tags = taglist->getCheckedTags();
        emit m_bookmarks->bookmarksChanged();
    }
}

void DockBookmarks::activated(const QModelIndex & index)
{
    const BookmarkInfo *info = bookmarksTableModel->getBookmarkAtRow(index.row());
    emit newBookmarkActivated(info->frequency, info->modulation, info->bandwidth);
}

void DockBookmarks::addBookmark()
{
    emit newBookmarkAdd();
}

bool DockBookmarks::deleteSelectedBookmark()
{
    QModelIndexList selected = ui->tableViewFrequencyList->selectionModel()->selectedRows();

    if (selected.empty())
    {
        return true;
    }

    if (QMessageBox::question(this, "Delete bookmark", "Really delete?",
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        int iIndex = bookmarksTableModel->getBookmarksIndexForRow(selected.first().row());
        m_bookmarks->remove(iIndex);
        bookmarksTableModel->update();
    }
    return true;
}

bool DockBookmarks::editSelectedField()
{
    QModelIndexList selected = ui->tableViewFrequencyList->selectionModel()->selectedRows();

    if (selected.empty())
    {
        return true;
    }

    auto row = selected.first().row();
    auto col = selected.first().column();
    switch (col)
    {
    case BookmarksTableModel::COL_BANDWIDTH:
        break;
    case BookmarksTableModel::COL_FREQUENCY:
        break;
    case BookmarksTableModel::COL_MODULATION:
        break;
    case BookmarksTableModel::COL_NAME:
        // ui->tableViewFrequencyList->edit(...); // TODO
        break;
    case BookmarksTableModel::COL_TAGS:
        break;
    default:
        break;
    }
}

void DockBookmarks::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    Q_ASSERT(topLeft == bottomRight);
    ui->tableWidgetTagList->updateTags();
    m_bookmarks->setModified();
    emit bookmarkModified();
}

void DockBookmarks::showContextMenu(const QPoint& pos)
{
    contextmenu->popup(ui->tableViewFrequencyList->viewport()->mapToGlobal(pos));
}

void DockBookmarks::tagsClicked(const QModelIndex & index)
{
    if(index.column() == BookmarksTableModel::COL_TAGS)
    {
        showTagsSelector(index.row(), index.column());
    }
}

