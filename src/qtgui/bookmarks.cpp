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

#include <algorithm>

#ifndef QT_NO_DEBUG_OUTPUT
#include <QDebug>
#endif
#include <QSet>
#include <QTemporaryFile>
#include <QTextStream>

#include "bookmarks.h"

#define FIELD_WIDTH_BW   10
#define FIELD_WIDTH_FREQ 12
#define FIELD_WIDTH_INFO 30
#define FIELD_WIDTH_NAME 27
#define FIELD_WIDTH_MOD  20
#define FIELD_WIDTH_TAG  22

#define SAVE_TIMER_INTERVAL_MS 300000

const QColor TagInfo::DEFAULT_COLOR(Qt::lightGray);
const QString TagInfo::UNTAGGED("Untagged");

/**
 * @brief TagInfo::TagInfo
 */
TagInfo::TagInfo(bool modified) :
    checked(false),
    color(DEFAULT_COLOR),
    id(QUuid::createUuid()),
    modified(modified),
    show(true)
{
}

TagInfo::TagInfo(const QUuid &id, bool modified) :
    checked(false),
    color(DEFAULT_COLOR),
    id(id),
    modified(modified),
    show(true)
{
}

TagInfo::TagInfo(const QString &name, bool modified) :
    checked(false),
    color(DEFAULT_COLOR),
    id(QUuid::createUuid()),
    modified(modified),
    name(name.trimmed()),
    show(true)
{
}

bool TagInfo::operator<(const TagInfo &other) const
{
    return name < other.name;
}

bool TagInfo::operator==(const TagInfo &other) const
{
    return id == other.id || name == other.name;
}

bool TagInfo::operator!=(const TagInfo &other) const
{
    return !(*this == other);
}

// BookmarkInfo
bool BookmarkInfo::compareTagInfoPtr(const TagInfo *a, const TagInfo *b)
{
    return a->name.toLower() < b->name.toLower();
}

QString BookmarkInfo::tagsToString(const QList<TagInfo *> &tagList)
{
    QString s;
    auto sortedList = tagList;
    std::stable_sort(sortedList.begin(), sortedList.end(), compareTagInfoPtr);
    for (auto it = sortedList.cbegin(), it_begin = it, it_end = sortedList.cend(); it != it_end; it++)
    {
        if (it != it_begin)
            s += Bookmarks::TAG_SEPARATOR2;
        s += (*it)->name;
    }
    return s;
}

/**
 * @brief BookmarkInfo::BookmarkInfo
 */
BookmarkInfo::BookmarkInfo(bool modified) :
    bandwidth(0),
    id(QUuid::createUuid()),
    frequency(0),
    modified(modified)
{
}

void BookmarkInfo::addTagInfo(TagInfo * const tagInfo, bool modified)
{
    tags.append(tagInfo);
    tagsStr = tagsToString(getFilteredTags());
    this->modified = modified;
}

QList<TagInfo *> BookmarkInfo::getFilteredTags() const
{
    return getFilteredTags(tags);
}

QList<TagInfo *> BookmarkInfo::getFilteredTags(const QList<TagInfo *> &tagList) const
{
    QList<TagInfo *> list;
    for (auto it = tagList.cbegin(), itend = tagList.cend(); it != itend; it++)
    {
        if ((*it)->name == TagInfo::UNTAGGED)
            continue;

        list.append(*it);
    }
    return list;
}

bool BookmarkInfo::removeTagInfo(TagInfo * const tagInfo)
{
    const auto ret = tags.removeOne(tagInfo);
    tagsStr = tagsToString(getFilteredTags());
    modified = true;
    return ret;
}

void BookmarkInfo::setTags(const QList<TagInfo *> &tagInfo)
{
    const auto oldTagsStr = tagsStr;
    tagsStr = tagsToString(getFilteredTags(tagInfo));
    if (tagsStr != oldTagsStr)
    {
        tags = tagInfo;
        modified = true;
    }
    else
    {
        tagsStr = oldTagsStr;
    }
}

bool BookmarkInfo::operator<(const BookmarkInfo &other) const
{
    return frequency < other.frequency;
}

bool BookmarkInfo::operator==(const BookmarkInfo &other) const
{
    return (!id.isNull() && id == other.id) || frequency == other.frequency;
}

bool BookmarkInfo::operator!=(const BookmarkInfo &other) const
{
    return !(*this == other);
}

const QColor &BookmarkInfo::getColor() const
{
    for(int i = 0; i < tags.size(); ++i)
    {
        if(tags[i]->show)
            return tags[i]->color;
    }
    return TagInfo::DEFAULT_COLOR;
}

const QChar Bookmarks::CSV_QUOTE('"');
const QString Bookmarks::CSV_SEPARATOR(";");
const QString Bookmarks::CSV_SEPARATOR2("; ");
const int Bookmarks::ID_ROLE(Qt::UserRole + 0x0001);
const QString Bookmarks::TAG_SEPARATOR(",");
const QString Bookmarks::TAG_SEPARATOR2(", ");

Bookmarks &Bookmarks::instance()
{
    static Bookmarks singleton;
    return singleton;
}

// TODO don't do this sort
void Bookmarks::add(BookmarkInfo &info)
{
    m_bookmarkList.append(info);
    std::stable_sort(m_bookmarkList.begin(), m_bookmarkList.end());
    emit bookmarksChanged();
    m_bmModified = true;
}

bool Bookmarks::addTagInfo(const TagInfo &tagInfo)
{
    if (m_tagList.indexOf(tagInfo.name) < 0)
    {
        m_tagList.append(tagInfo);
        return true;
    }
    return false;
}

TagInfo &Bookmarks::findOrAddTag(const QString &tagName, bool markModified)
{
    auto trimName = tagName.trimmed();
    if (!trimName.isEmpty())
    {
        const int idx = m_tagList.indexOf(trimName);
        if (idx >= 0)
            return m_tagList[idx];
    }
    else
    {
        const int idx = m_tagList.indexOf(TagInfo::UNTAGGED);
        if (idx >= 0)
            return m_tagList[idx];
        trimName = TagInfo::UNTAGGED;
    }

    const TagInfo info(tagName, markModified);
    m_tagList.append(info);
    const int idx = m_tagList.indexOf(trimName);
    if (idx >= 0)
    {
        emit tagListChanged();
        if (markModified && trimName != TagInfo::UNTAGGED)
            m_bmModified = true; // only set for non-UNTAGGED
        return m_tagList[idx];
    }
    Q_ASSERT(false);
}

QList<const BookmarkInfo *> Bookmarks::getBookmarksInRange(qint64 low, qint64 high) const
{
    BookmarkInfo infoBound;
    infoBound.frequency=low;
    auto lb = std::lower_bound(m_bookmarkList.cbegin(), m_bookmarkList.cend(), infoBound);
    infoBound.frequency=high;
    auto ub = std::upper_bound(m_bookmarkList.cbegin(), m_bookmarkList.cend(), infoBound);

    QList<const BookmarkInfo *> found;

    while (lb != ub)
    {
        const BookmarkInfo *info = &(*lb);
        found.append(info);
        lb++;
    }

    return found;
}

const TagInfo &Bookmarks::getTagInfo(const QUuid &id) const
{
    const int idx = m_tagList.indexOf(id);
    if (idx < 0)
    {
        throw "Failed to find id";
    }
    return m_tagList.at(idx);
}

TagInfo &Bookmarks::getTagInfo(const QUuid &id)
{
    const int idx = m_tagList.indexOf(id);
    if (idx < 0)
    {
        qWarning() << "Failed to find id" << id;
        throw "Failed to find id";
    }
    return m_tagList[idx];
}

bool Bookmarks::load()
{
    QFile file(m_bookmarksFile);
    if (file.open(QFile::ReadOnly | QFile::Text))
    {
        QTextStream stream(&file);

        m_bmModified = false; // no save during loading
        m_bookmarkList.clear();
        m_tagList.clear();

        // always create the UNTAGGED entry for enable filter on not tagged bookmarks
        auto untaggedTag = findOrAddTag(TagInfo::UNTAGGED, false);

        // Read Tags, until first empty line.
        while (!stream.atEnd())
        {
            auto line = stream.readLine().trimmed();

            if(line.isEmpty()) // jump to next block
                break;

            if(line.startsWith("#")) // trimmed line
                continue;

            QStringList list = csvsplit(line, 2);

            if(list.count() == 2)
            {
                auto info = findOrAddTag(list[0], false); // emits tagListChanged()
                info.modified = false;  // freshly read
                info.color = QColor(list[1]);
            }
#ifndef QT_NO_DEBUG_OUTPUT
            else
            {
                qDebug() << "Bookmarks: Ignoring Line:" << line;
            }
#endif
        }
        // std::sort(m_tagList.begin(), m_tagList.end()); // TODO remove

        // Read Bookmarks, after first empty line.
        while (!stream.atEnd())
        {
            auto line = stream.readLine().trimmed();

            // Hint: if there are more changes in field count, you could read the field count
            //       from this comment line
            if(line.isEmpty() || line.startsWith("#"))
                continue;

            int listcount = 6;
            QStringList list = csvsplit(line, listcount);
            if (list.isEmpty())
                list = csvsplit(line, --listcount);

            if(list.count() == listcount)
            {
                BookmarkInfo info(false);
                info.frequency  = list[0].toLongLong();
                info.name       = list[1];
                info.modulation = list[2];
                info.bandwidth  = list[3].toInt();

                // Multiple Tags may be separated by comma.
                QStringList tagList = csvsplit(list[4] + TAG_SEPARATOR, 0, TAG_SEPARATOR);
                const int tag_count = tagList.size();
                for(int i = 0; i < tag_count; ++i)
                {
                    info.addTagInfo(&findOrAddTag(tagList[i], false), false); // here ok without modify
                }
                if (tag_count == 0)
                    info.addTagInfo(&untaggedTag, false); // here ok without modify

                if (listcount == 6)
                    info.info = list[5];

                m_bookmarkList.append(info);
            }
#ifndef QT_NO_DEBUG_OUTPUT
            else
            {
                qDebug() << "Bookmarks: Ignoring Line:" << line;
            }
#endif
        }
        file.close();
        //std::stable_sort(m_bookmarkList.begin(), m_bookmarkList.end()); // TODO remove

#ifndef QT_NO_DEBUG_OUTPUT
        for (int i = 0; i < m_tagList.count(); i++)
        {
            qDebug() << m_tagList[i].name << m_tagList[i].color;
        }
        for (int i = 0; i < m_bookmarkList.count(); i++)
        {
            qDebug() << m_bookmarkList[i].name << m_bookmarkList[i].frequency << m_bookmarkList[i].modulation << m_bookmarkList[i].bandwidth;
            for (int t = 0; t < m_bookmarkList[i].tags.count(); t++)
            {
                qDebug() << m_bookmarkList[i].tags[t]->name;
            }
        }
#endif

        emit bookmarksChanged();
        return true;
    }

    return false;
}

void Bookmarks::remove(int index)
{
    m_bookmarkList.removeAt(index);
    m_bmModified = true;
    emit bookmarksChanged();
}

bool Bookmarks::removeTagInfo(TagInfo &tagInfo)
{
    if (tagInfo.name == TagInfo::UNTAGGED)
        return false;

    // Delete Tag from all Bookmarks that use it.
    const auto bmlist_end = m_bookmarkList.cend();
    for (auto it = m_bookmarkList.begin(); it != bmlist_end; it++)
    {
        it->removeTagInfo(&tagInfo);
        if (it->tags.count() == 0)
        {
            it->addTagInfo(&findOrAddTag(TagInfo::UNTAGGED));
        }
    }

    // Delete Tag.
    m_tagList.removeOne(tagInfo);

    emit bookmarksChanged();
    emit tagListChanged();

    return (m_bmModified = true);
}

void Bookmarks::setConfigDir(const QString &cfg_dir)
{
    m_bookmarksFile = cfg_dir + "/bookmarks.csv";
#ifndef QT_NO_DEBUG_OUTPUT
    qDebug() << "BookmarksFile is " << m_bookmarksFile;
#endif
}

void Bookmarks::setModified(bool modified)
{
    m_bmModified = modified;
}

void Bookmarks::setTagChecked(const QUuid &id, bool checked)
{
    setTagChecked(getTagInfo(id), checked);
}

void Bookmarks::setTagChecked(TagInfo &tagInfo, bool checked)
{
    if (tagInfo.checked != checked)
    {
        tagInfo.checked = checked;
        emit tagListChanged();
    }
}

void Bookmarks::setTagColor(TagInfo &tagInfo, const QColor &color)
{
    if(!color.isValid())
        return;

    if (tagInfo.color != color)
    {
        tagInfo.color = color;
        tagInfo.modified = true;
        emit tagListChanged();
    }
}

void Bookmarks::setTagShow(const QUuid &id, bool show)
{
    setTagShow(getTagInfo(id), show);
}

void Bookmarks::setTagShow(TagInfo &tagInfo, bool show)
{
    if (tagInfo.show != show)
    {
        tagInfo.show = show;
        emit tagListFilter();
    }
}

bool Bookmarks::save()
{
    if (!m_bmModified)
    {
        {
            const auto it_end = m_tagList.cend();
            for (auto it = m_tagList.begin(); it != it_end; it++)
            {
                if (it->modified)
                {
                    m_bmModified = true;
                    it->modified = false;
                    // no break;
                }
            }
        }
        {
            const auto it_end = m_bookmarkList.cend();
            for (auto it = m_bookmarkList.begin(); it != it_end; it++)
            {
                if (it->modified)
                {
                    m_bmModified = true;
                    it->modified = false;
                    // no break;
                }
            }
        }

        if (!m_bmModified)
            return true;
    }

    QTemporaryFile tmp;
    if(tmp.open())
    {
        m_bmModified = false;
        QTextStream stream(&tmp);

        stream << QString("# Tag name").leftJustified(FIELD_WIDTH_TAG) << CSV_SEPARATOR2 <<
                  QString(" color") << endl;

        // TODO why don't delete tags only on request?
        bool usedInfoField = false;

        const int bookmarkSize = m_bookmarkList.size();
        QSet<TagInfo*> usedTags;
        for (int ib = 0; ib < bookmarkSize; ib++)
        {
            BookmarkInfo &info = m_bookmarkList[ib];
            const int tagssize = info.tags.size();
            for(int i = 0; i < tagssize; ++i)
            {
                usedTags.insert(info.tags[i]);
            }
            if (!(usedInfoField || info.info.isEmpty()))
                usedInfoField = true;
        }

        const auto it_end = usedTags.cend();
        for (auto it = usedTags.cbegin(); it != it_end; it++)
        {
            TagInfo &info = **it;
            stream << csvquote(info.name, FIELD_WIDTH_TAG) <<
                      CSV_SEPARATOR2 << info.color.name() << endl;
        }

        stream << endl <<
                  QString("# Frequency").leftJustified(FIELD_WIDTH_FREQ) << CSV_SEPARATOR2 <<
                  QString("Name").leftJustified(FIELD_WIDTH_NAME) << CSV_SEPARATOR2 <<
                  QString("Modulation").leftJustified(FIELD_WIDTH_MOD) << CSV_SEPARATOR2 <<
                  QString("Bandwidth").rightJustified(FIELD_WIDTH_BW) << CSV_SEPARATOR2;


        if (usedInfoField)
        {
            stream << QString("Tags").leftJustified(FIELD_WIDTH_TAG) << CSV_SEPARATOR2 << QString("Info");
        }
        else
        {
            stream << QString("Tags");
        }

        stream << endl;

        TagInfo &taggedTag = findOrAddTag(TagInfo::UNTAGGED);

        for (int ib = 0; ib < bookmarkSize; ib++)
        {
            const BookmarkInfo &info = m_bookmarkList[ib];
            stream << QString::number(info.frequency).rightJustified(FIELD_WIDTH_FREQ) <<
                      CSV_SEPARATOR2 << csvquote(info.name, FIELD_WIDTH_NAME) <<
                      CSV_SEPARATOR2 << info.modulation.leftJustified(FIELD_WIDTH_MOD) <<
                      CSV_SEPARATOR2 << QString::number(info.bandwidth).rightJustified(FIELD_WIDTH_BW) <<
                      CSV_SEPARATOR2;
            const int tagssize = info.tags.size();
            int justify = 0;
            for(int i = 0; i < tagssize; ++i)
            {
                if (info.tags[i] == &taggedTag)
                    continue;

                QString s = csvquote(info.tags[i]->name);
                stream << s;
                justify += s.length();

                if(i < tagssize - 1)
                {
                    stream << TAG_SEPARATOR;
                    justify += TAG_SEPARATOR.length();
                }
            }

            if (usedInfoField)
            {
                for (int j = FIELD_WIDTH_TAG - justify; j > 0; j--)
                {
                    stream << " ";
                }
                stream << CSV_SEPARATOR2 << csvquote(info.info, FIELD_WIDTH_INFO);
            }

            stream << endl;
        }

        tmp.close();
    }
    else
    {
        qCritical("Bookmarks: Failed to save: Could not open temporary file");
        m_bmModified = true; // reset modified state
        return false;
    }

    QString bckfile(m_bookmarksFile + ".bck");
    if (QFile::exists(bckfile) && !QFile::remove(bckfile))
    {
        qCritical("Bookmarks: Failed to save: Could not remove backup file");
        m_bmModified = true; // reset modified state
        return false;
    }
    if (QFile::exists(m_bookmarksFile) && !QFile::rename(m_bookmarksFile, bckfile))
    {
        qCritical("Bookmarks: Failed to save: Could not create backup file");
        m_bmModified = true; // reset modified state
        return false;
    }

    if (!QFile::copy(tmp.fileName(), m_bookmarksFile))
    {
        if (!QFile::copy(bckfile, m_bookmarksFile))
            qCritical("Bookmarks: Restore of backup failed");
        qCritical("Bookmarks: Failed to save: Could not copy temporary file");
        m_bmModified = true; // reset modified state
        return false;
    }

    if (QFile::exists(bckfile) && !QFile::remove(bckfile))
    {
        qInfo("Bookmarks: Failed to remove backup file");
    }

    return true;
}

Bookmarks::Bookmarks() :
    m_bmModified(false)
{
    m_tagList.append(findOrAddTag(TagInfo::UNTAGGED));
    m_saveTimer = new QTimer(this);
    m_saveTimer->setInterval(SAVE_TIMER_INTERVAL_MS);
    connect(m_saveTimer, SIGNAL(timeout()), this, SLOT(save()));
    m_saveTimer->start();
}

Bookmarks::~Bookmarks()
{
    m_saveTimer->stop();
    delete m_saveTimer;
    save();
}

QString Bookmarks::csvquote(const QString &unquoted, int minlength)
{
    QString quoted(unquoted);
    int isd = quoted.indexOf(CSV_QUOTE + CSV_SEPARATOR);
    if (isd >= 0)
        quoted = quoted.replace(isd + 1, 1, "_");
    int is = quoted.indexOf(CSV_SEPARATOR);
    int ic = quoted.indexOf(TAG_SEPARATOR);
    int icd = quoted.indexOf(CSV_QUOTE + TAG_SEPARATOR);
    if (icd >= 0)
        quoted = quoted.replace(icd + 1, 1, "_");

    bool q = quoted.startsWith(CSV_QUOTE);

    bool quote = true;
    if (is < 0 && ic < 0 && !q)
        quote = false;

    QString justify = "";
    for (int i = minlength - quoted.length() - (quote ? 2 : 0); i > 0; i--)
    {
        justify += " ";
    }

    if (quote)
        return CSV_QUOTE + quoted + justify + CSV_QUOTE;

    return quoted + justify;
}

QStringList Bookmarks::csvsplit(const QString &text, int fieldCount, const QString &separator)
{
    QStringList list;
    QStringRef ref(&text);
    int sep = 0; // next separator position
    int qu = 0;  // next quote position

    for (;!ref.isEmpty();)
    {
        QStringRef field;
        sep = ref.indexOf(separator, sep);
        qu = ref.indexOf(CSV_QUOTE);
        if (sep >= 0 && qu >= 0 && qu < sep)
        {
            if (sep == ref.length() - 1 || (sep > 0 && ref[sep - 1] == CSV_QUOTE))
            {
                ref = ref.right(ref.length() - qu - 1);
                sep -= qu + 1;
                qu = sep - 1;
            }
            if (qu < sep - 1)
            {
                sep++;
                continue; // next separator
            }
            else
            {
                field = ref.left(qu).trimmed();
                list.append(field.toString());
                ref = ref.right(ref.length() - qu - 2).trimmed();
                sep = 0;
            }
        }
        else if (sep >= 0)
        {
            field = ref.left(sep).trimmed();
            list.append(field.toString());
            ref = ref.right(ref.length() - sep - 1).trimmed();
            sep = 0;
        }
        else
        {
            field = ref.trimmed();
            list.append(field.toString());
            ref = ref.right(0);
        }
    }

    if (fieldCount > 0 && list.count() != fieldCount)
    {
        qCritical() << "Bookmarks: Instead of" << fieldCount << "found" << list.count() << "field(s) with" << separator;
        return QStringList();
    }
#ifndef QT_NO_DEBUG_OUTPUT
    else
    {
        qDebug() << "Bookmarks: Found" << list.count() << "field(s) with" << separator;
    }
#endif

    return list;
}
