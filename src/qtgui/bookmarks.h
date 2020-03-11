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
#ifndef BOOKMARKS_H
#define BOOKMARKS_H

#include <QColor>
#include <QMap>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QUuid>

/**
 * @brief The TagInfo struct for storing data of tags
 */
struct TagInfo
{
    static const QColor DEFAULT_COLOR;
    static const QString UNTAGGED;

    bool    checked; // temporary sync usage for updating BookmarkInfo
    QColor  color;
    QUuid   id;
    bool    modified;
    QString name;
    bool    show;  // filter

    /**
     * @brief TagInfo
     * @param modified detects need for save
     */
    TagInfo(bool modified = true);

    /**
     * @brief TagInfo
     * @param id
     * @param modified detects need for save
     */
    TagInfo(const QUuid &id, bool modified = true);

    /**
     * @brief TagInfo
     * @param name
     * @param modified detects need for save
     */
    TagInfo(const QString &name, bool modified = true);

    /**
     * @brief compares member name
     * @param other
     * @return
     */
    bool operator<(const TagInfo &other) const;

    /**
     * @brief compares member id or name for equality
     * @param other
     * @return
     */
    bool operator==(const TagInfo &other) const;

    /**
     * @brief compares member id or name for inequality
     * @param other
     * @return
     */
    bool operator!=(const TagInfo &other) const;
};

/**
 * @brief The BookmarkInfo struct for storing data of bookmark
 */
struct BookmarkInfo
{
    static bool compareTagInfoPtr(const TagInfo *a, const TagInfo *b);
    static QString tagsToString(const QList<TagInfo *> &tagList);

    qint64  bandwidth;
    QUuid   id;
    QString info;
    qint64  frequency;
    QString modulation;
    bool    modified;
    QString name;
    QList<TagInfo *> tags;
    QString tagsStr;

    /**
     * @brief BookmarkInfo
     * @param modified detects need for save
     */
    BookmarkInfo(bool modified = true);

    /**
     * @brief add TagInfo by tagInfo pointer
     * @param tagInfo
     * @param modified detects need for save
     */
    void addTagInfo(TagInfo * const tagInfo, bool modified = true);

    /**
     * @brief get tags without TagInfo::UNTAGGED
     * @return QList<TagInfo *>
     */
    QList<TagInfo *> getFilteredTags() const;

    /**
     * @brief get tags without TagInfo::UNTAGGED
     * @param const QList<TagInfo *>&
     * @return QList<TagInfo *>
     */
    QList<TagInfo *> getFilteredTags(const QList<TagInfo *> &tagList) const;

    /**
     * @brief get tags without TagInfo::UNTAGGED
     * @return QString
     */
    QString getTags() const;

    /**
     * @brief remove TagInfo by pointer
     * @param tagInfo
     * @return
     */
    bool removeTagInfo(TagInfo * const tagInfo);

    /**
     * @brief set tags list of tagInfo pointer
     * @param QList tagInfo
     */
    void setTags(const QList<TagInfo *> &tagInfo);

    /**
     * @brief compares frequency
     * @param other
     * @return
     */
    bool operator<(const BookmarkInfo &other) const;

    /**
     * @brief compares for equal id(s) or frequency
     * @param other
     * @return
     */
    bool operator==(const BookmarkInfo &other) const;

    /**
     * @brief compares for not equal id(s) or frequency
     * @param other
     * @return
     */
    bool operator!=(const BookmarkInfo &other) const;

    /**
     * @brief get Color of first active tag
     * @return
     */
    const QColor &getColor() const;
};

/**
 * @brief The Bookmarks class holding all bookmarks
 */
class Bookmarks : public QObject
{
    Q_OBJECT

public:
    static const QChar CSV_QUOTE;
    static const QString CSV_SEPARATOR;
    static const QString CSV_SEPARATOR2;
    static const int ID_ROLE;
    static const QString TAG_SEPARATOR;
    static const QString TAG_SEPARATOR2;

    /**
     * @brief lazy loaded singleton instance of Bookmarks
     * @return
     */
    static Bookmarks &instance();

    /**
     * @brief add bookmark (optional adding UNTAGGED)
     * @param info
     * @param markModified
     */
    void add(BookmarkInfo &info, bool markModified = true);

    /**
     * @brief add TagInfo
     * @param tagInfo
     * @return
     */
    bool addTagInfo(const TagInfo &tagInfo);

    /**
     * @brief how many bookmarks there are
     * @return
     */
    int count() { return m_bookmarkList.count(); }

    /**
     * @brief find or add a tag with internal trimmed tagName
     * @param tagName (need not to be trimmed)
     * @param markModified - marks modified for save
     * @return TagInfo&
     */
    TagInfo &findOrAddTag(const QString &tagName, bool markModified = true);

    BookmarkInfo &getBookmark(int i) { return m_bookmarkList[i]; }

    /**
     * @brief getBookmark by QUuid id, is backed by QMap<QUuid, *BookmarkInfo>
     * @param id
     * @return
     */
    BookmarkInfo &getBookmark(const QUuid &id);

    QList<const BookmarkInfo *> getBookmarksInRange(qint64 low, qint64 high) const;

    const QList<TagInfo> &getTagList() const { return m_tagList; }

    /**
     * @brief getTagInfo by UUid
     * @param id
     * @return const TagInfo&
     */
    const TagInfo &getTagInfo(const QUuid &id) const;

    /**
     * @brief getTagInfo by UUid
     * @param id
     * @return TagInfo&
     */
    TagInfo &getTagInfo(const QUuid &id);

    /**
     * @brief Loads special formatted data with semikolon and comma separator
     * @details should enable all chars in the data, only combination "; gets replaced with "_
     * @see csvsplit()
     * @return true on success
     */
    bool load();

    /**
     * @brief remove Bookmark by QUuid
     * @param id
     */
    void remove(const QUuid &id);

    /**
     * @brief remove TagInfo by QUuid
     * @param &tagInfo
     * @return
     */
    bool removeTagInfo(TagInfo &tagInfo);

    /**
     * @brief setConfigDir to know where to save()
     * @param cfg_dir
     */
    void setConfigDir(const QString &cfg_dir);

    /**
     * @brief setModified
     * @param modified
     */
    void setModified(bool modified = true);

    /**
     * @brief set checked state of tag
     * @param id
     * @param checked
     */
    void setTagChecked(const QUuid &id, bool checked);

    /**
     * @brief set checked state of tag
     * @param tagInfo
     * @param checked
     */
    void setTagChecked(TagInfo &tagInfo, bool checked);

    /**
     * @brief setTagColor
     * @param tagInfo
     * @param color
     */
    void setTagColor(TagInfo &tagInfo, const QColor &color);

    /**
     * @brief set show state of tag
     * @param id
     * @param show
     */
    void setTagShow(const QUuid &id, bool show);

    /**
     * @brief set show state of tag
     * @param tagInfo
     * @param show
     */
    void setTagShow(TagInfo &tagInfo, bool show);

public slots:
    /**
     * @brief Safe save with temporary file, backup and replacement
     * @details does quoting of separator chars for some text fields
     * @see csvquote()
     * @return true on success
     */
    bool save();

signals:
    void bookmarksChanged();
    void tagListChanged();
    void tagListFilter();

private:
    bool                 m_bmModified;
    QMap<QUuid, BookmarkInfo *> m_bookmarkIdMap;
    QList<BookmarkInfo>  m_bookmarkList;
    QString              m_bookmarksFile;
    QTimer               *m_saveTimer;
    QList<TagInfo>       m_tagList;

    /**
     * @brief Bookmarks is lazy initialized static singleton instance
     */
    Bookmarks();
    ~Bookmarks();

    /**
     * @brief Decides if quoting is needed and returns a safe string
     * @param unquoted
     * @param minlength for leftJustify
     * @return a safe and perhaps quoted string
     */
    static QString csvquote(const QString &unquoted, int minlength = 0);

    /**
     * @brief Splits up text at separator and checks for fieldCount
     * @details Handles quoted fields, but field must be quoted till the end and the next char is the separator
     * @param text
     * @param fieldCount check if > 0
     * @param separator a seperator char like , or ;
     * @return empty list if fieldCount does not match, filled list on success
     */
    static QStringList csvsplit(const QString &text, int fieldCount, const QString &separator = CSV_SEPARATOR);

};

#endif // BOOKMARKS_H
