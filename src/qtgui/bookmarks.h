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
#include <QUuid>

/**
 * @brief The TagInfo struct for storing data of tags
 */
struct TagInfo
{
    static const QColor DEFAULT_COLOR;
    static const QString UNTAGGED;

    bool    active;
    QColor  color;
    QUuid   id;
    QString name;

    TagInfo();
    TagInfo(const QString &name);

    /**
     * @brief compares member name
     * @param other
     * @return
     */
    bool operator<(const TagInfo &other) const;

    /**
     * @brief compares member id for equality
     * @param other
     * @return
     */
    bool operator==(const TagInfo &other) const;

    /**
     * @brief compares member id for inequality
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
    qint64  bandwidth;
    QUuid   id;
    QString info;
    qint64  frequency;
    QString name;
    QString modulation;
    QList<TagInfo*> tags;

    BookmarkInfo();

    /**
     * @brief compares frequency
     * @param other
     * @return
     */
    bool operator<(const BookmarkInfo &other) const;

    /**
     * @brief compares for equal id(s)
     * @param other
     * @return
     */
    bool operator==(const BookmarkInfo &other) const;

    /**
     * @brief compares for not equal id(s)
     * @param other
     * @return
     */
    bool operator!=(const BookmarkInfo &other) const;

    const QColor &getColor() const;
    bool isActive() const;
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
    static const QString TAG_SEPARATOR;

    /**
     * @brief lazy loaded singleton instance of Bookmarks
     * @return
     */
    static Bookmarks &instance();

    void add(const BookmarkInfo &info);

    /**
     * @brief find or add a tag with internal trimmed tagName
     * @param tagName (need not to be trimmed)
     * @return  TagInfo&
     */
    TagInfo &findOrAddTag(const QString &tagName);

    BookmarkInfo &getBookmark(int i) { return m_bookmarkList[i]; }
    QList<BookmarkInfo> getBookmarksInRange(qint64 low, qint64 high);

    /**
     * @brief get index for internal trimmed tagName
     * @param tagName (need not to be trimmed)
     * @return int index
     */
    int getTagIndex(const QString &tagName);

    QList<TagInfo> getTagList() { return  QList<TagInfo>(m_tagList); }

    /**
     * @brief get pointer to TagInfo by tagName
     * @param tagInfo to retrieve
     * @param tagName (need not to be trimmed)
     * @return
     */
    void getTagInfo(const TagInfo *tagInfo, const QString &tagName) const;

    /**
     * @brief Loads special formatted data with semikolon and comma separator
     * @details should enable all chars in the data, only combination "; gets replaced with "_
     * @see csvsplit()
     * @return true on success
     */
    bool load();


    void remove(int index);

    /**
     * @brief remove tag for internal trimmed tagName
     * @param tagName (need not to be trimmed)
     * @return true on success
     */
    bool removeTag(const QString &tagName);

    /**
     * @brief Safe save with temporary file, backup and replacement
     * @details does quoting of separator chars for some text fields
     * @see csvquote()
     * @return true on success
     */
    bool save();

    /**
     * @brief setConfigDir to know where to save()
     * @param cfg_dir
     */
    void setConfigDir(const QString &cfg_dir);

    /**
     * @brief set active state of tag
     * @param tagName
     * @param is_active
     * @return true if tag found and set, else false
     */
    bool setTagActive(const QString &tagName, bool is_active);

    int size() { return m_bookmarkList.size(); }

signals:
    void bookmarksChanged(void);
    void tagListChanged(void);

private:
    bool                 m_bmModified;
    QList<BookmarkInfo>  m_bookmarkList;
    QString              m_bookmarksFile;
    QList<TagInfo>       m_tagList;

    /**
     * @brief Bookmarks is lazy initialized static singleton instance
     */
    Bookmarks();

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
