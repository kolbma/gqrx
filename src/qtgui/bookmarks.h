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

struct TagInfo
{
    static const QColor DEFAULT_COLOR;
    static const QString UNTAGGED;

    QString name;
    QColor color;
    bool active;

    TagInfo()
    {
        active = true;
        this->color = DEFAULT_COLOR;
    }

    TagInfo(const QString &name)
    {
        active = true;
        this->color = DEFAULT_COLOR;
        this->name = name;
    }

    bool operator<(const TagInfo &other) const
    {
        return name < other.name;
    }

    bool operator==(const TagInfo &other) const
    {
        return name == other.name;
    }
};

struct BookmarkInfo
{
    qint64  frequency;
    QString name;
    QString modulation;
    qint64  bandwidth;
    QList<TagInfo*> tags;

    BookmarkInfo()
    {
        this->frequency = 0;
        this->bandwidth = 0;
    }

/*    BookmarkInfo( qint64 frequency, QString name, qint64 bandwidth, QString modulation )
    {
        this->frequency = frequency;
        this->name = name;
        this->modulation = modulation;
        this->bandwidth = bandwidth;
    }
*/
    bool operator<(const BookmarkInfo &other) const
    {
        return frequency < other.frequency;
    }
/*
    void setTags(QString tagString);
    QString getTagString();
    bool hasTags(QString _tags);
    bool hasTags(QStringList _tags);
 */

    const QColor &getColor() const;
    bool isActive() const;
};

class Bookmarks : public QObject
{
    Q_OBJECT
public:
    static const QChar CSV_QUOTE;
    static const QString CSV_SEPARATOR;
    static const QString CSV_SEPARATOR2;
    static const QString TAG_SEPARATOR;

    // This is a Singleton Class now because you can not send qt-signals from static functions.
    static void create();
    static Bookmarks &get();

    void add(const BookmarkInfo &info);
    void remove(int index);

    /**
     * @brief Loads special formatted data with semikolon and comma separator
     * @details should enable all chars in the data, only combination "; gets replaced with "_
     * @see csvsplit()
     * @return true on success
     */
    bool load();

    /**
     * @brief Safe save with temporary file, backup and replacement
     * @details does quoting of separator chars for some text fields
     * @see csvquote()
     * @return true on success
     */
    bool save();

    int size() { return m_bookmarkList.size(); }
    BookmarkInfo &getBookmark(int i) { return m_bookmarkList[i]; }
    QList<BookmarkInfo> getBookmarksInRange(qint64 low, qint64 high);
    //int lowerBound(qint64 low);
    //int upperBound(qint64 high);

    QList<TagInfo> getTagList() { return  QList<TagInfo>(m_tagList); }

    /**
     * @brief find or add a tag with internal trimmed tagName
     * @param tagName (need not to be trimmed)
     * @return  TagInfo&
     */
    TagInfo &findOrAddTag(const QString &tagName);

    /**
     * @brief get pointer to TagInfo by tagName
     * @param tagInfo to retrieve
     * @param tagName (need not to be trimmed)
     * @return
     */
    void getTagInfo(const TagInfo *tagInfo, const QString &tagName) const;

    /**
     * @brief get index for internal trimmed tagName
     * @param tagName (need not to be trimmed)
     * @return int index
     */
    int getTagIndex(const QString &tagName);

    /**
     * @brief remove tag for internal trimmed tagName
     * @param tagName (need not to be trimmed)
     * @return true on success
     */
    bool removeTag(const QString &tagName);

    /**
     * @brief set active state of tag
     * @param tagName
     * @param is_active
     * @return true if tag found and set, else false
     */
    bool setTagActive(const QString &tagName, bool is_active);

    void setConfigDir(const QString &cfg_dir);

private:
    static Bookmarks    *m_pThis;

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

    QList<BookmarkInfo>  m_bookmarkList;
    QList<TagInfo>       m_tagList;
    QString              m_bookmarksFile;

    Bookmarks(); // Singleton Constructor is private.

signals:
    void bookmarksChanged(void);
    void tagListChanged(void);
};

#endif // BOOKMARKS_H
