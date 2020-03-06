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

#ifndef QT_NO_DEBUG_OUTPUT
#include <QDebug>
#endif
#include <QSet>
#include <QTemporaryFile>
#include <QTextStream>

#include "bookmarks.h"

#define FIELD_WIDTH_TAG  22
#define FIELD_WIDTH_FREQ 12
#define FIELD_WIDTH_NAME 27
#define FIELD_WIDTH_MOD  20
#define FIELD_WIDTH_BW   10

const QColor TagInfo::DEFAULT_COLOR(Qt::lightGray);
const QString TagInfo::UNTAGGED("Untagged");
const QChar Bookmarks::CSV_QUOTE('"');
const QString Bookmarks::CSV_SEPARATOR(";");
const QString Bookmarks::CSV_SEPARATOR2("; ");
const QString Bookmarks::TAG_SEPARATOR(",");
Bookmarks *Bookmarks::m_pThis = 0;

Bookmarks::Bookmarks()
{
     TagInfo tag(TagInfo::UNTAGGED);
     m_tagList.append(tag);
}

void Bookmarks::create()
{
    m_pThis = new Bookmarks;
}

Bookmarks &Bookmarks::get()
{
    return *m_pThis;
}

void Bookmarks::setConfigDir(const QString &cfg_dir)
{
    m_bookmarksFile = cfg_dir + "/bookmarks.csv";
#ifndef QT_NO_DEBUG_OUTPUT
    qDebug() << "BookmarksFile is " << m_bookmarksFile;
#endif
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

// TODO don't do this sort
void Bookmarks::add(const BookmarkInfo &info)
{
    m_bookmarkList.append(info);
    std::stable_sort(m_bookmarkList.begin(), m_bookmarkList.end());
    save();
    emit bookmarksChanged();
}

void Bookmarks::remove(int index)
{
    m_bookmarkList.removeAt(index);
    save();
    emit bookmarksChanged();
}

bool Bookmarks::load()
{
    QFile file(m_bookmarksFile);
    if (file.open(QFile::ReadOnly | QFile::Text))
    {
        QTextStream stream(&file);

        m_bookmarkList.clear();
        m_tagList.clear();

        // always create the "Untagged" entry. // TODO why?
        findOrAddTag(TagInfo::UNTAGGED);

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
                TagInfo &info = findOrAddTag(list[0]); // emits tagListChanged()
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

            if(line.isEmpty() || line.startsWith("#"))
                continue;

            QStringList list = csvsplit(line, 5);
            if(list.count() == 5)
            {
                BookmarkInfo info;
                info.frequency  = list[0].toLongLong();
                info.name       = list[1];
                info.modulation = list[2];
                info.bandwidth  = list[3].toInt();

                // Multiple Tags may be separated by comma.
                QStringList tagList = csvsplit(list[4] + ',', 0, TAG_SEPARATOR);
                const int tag_size = tagList.size();
                for(int i = 0; i < tag_size; ++i)
                {
                  info.tags.append(&findOrAddTag(tagList[i]));
                }

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

bool Bookmarks::save()
{
    QTemporaryFile tmp;
    if(tmp.open())
    {
        QTextStream stream(&tmp);

        stream << QString("# Tag name").leftJustified(FIELD_WIDTH_TAG) << CSV_SEPARATOR2 <<
                  QString(" color") << endl;

        // TODO why don't delete tags only on request?
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
                  QString("Bandwidth").rightJustified(FIELD_WIDTH_BW) << CSV_SEPARATOR2 <<
                  QString("Tags") << endl;

        for (int ib = 0; ib < bookmarkSize; ib++)
        {
            const BookmarkInfo &info = m_bookmarkList[ib];
            stream << QString::number(info.frequency).rightJustified(FIELD_WIDTH_FREQ) <<
                      CSV_SEPARATOR2 << csvquote(info.name, FIELD_WIDTH_NAME) <<
                      CSV_SEPARATOR2 << info.modulation.leftJustified(FIELD_WIDTH_MOD) <<
                      CSV_SEPARATOR2 << QString::number(info.bandwidth).rightJustified(FIELD_WIDTH_BW) <<
                      CSV_SEPARATOR2;
            const int tagssize = info.tags.size();
            for(int i = 0; i < tagssize; ++i)
            {
                if(i != 0)
                    stream << TAG_SEPARATOR;
                stream << csvquote(info.tags[i]->name);
            }

            stream << endl;
        }

        tmp.close();
    }
    else
    {
        qCritical("Bookmarks: Failed to save: Could not open temporary file");
        return false;
    }

    QString bckfile(m_bookmarksFile + ".bck");
    if (QFile::exists(bckfile) && !QFile::remove(bckfile))
    {
        qCritical("Bookmarks: Failed to save: Could not remove backup file");
        return false;
    }
    if (QFile::exists(m_bookmarksFile) && !QFile::rename(m_bookmarksFile, bckfile))
    {
        qCritical("Bookmarks: Failed to save: Could not create backup file");
        return false;
    }

    if (!QFile::copy(tmp.fileName(), m_bookmarksFile))
    {
        if (!QFile::copy(bckfile, m_bookmarksFile))
            qCritical("Bookmarks: Restore of backup failed");
        qCritical("Bookmarks: Failed to save: Could not copy temporary file");
        return false;
    }

    if (QFile::exists(bckfile) && !QFile::remove(bckfile))
    {
        qInfo("Bookmarks: Failed to remove backup file");
    }

    return true;
}

QList<BookmarkInfo> Bookmarks::getBookmarksInRange(qint64 low, qint64 high)
{
    BookmarkInfo info;
    info.frequency=low;
    QList<BookmarkInfo>::const_iterator lb = qLowerBound(m_bookmarkList, info);
    info.frequency=high;
    QList<BookmarkInfo>::const_iterator ub = qUpperBound(m_bookmarkList, info);

    QList<BookmarkInfo> found;

    while (lb != ub)
    {
        const BookmarkInfo &info = *lb;
        found.append(info);
        lb++;
    }

    return found;
}

const TagInfo &Bookmarks::findOrAddTag(const QString &tagName)
{
    int idx = getTagIndex(tagName);
    if (idx >= 0)
        return m_tagList[idx];

    auto trimTag = tagName.trimmed();

    if (trimTag.isEmpty())
        trimTag = TagInfo::UNTAGGED; // TODO we don't need this dummy tag

    const TagInfo info(trimTag);
    m_tagList.append(info);
    idx = getTagIndex(trimTag);
    emit tagListChanged();

    return m_tagList[idx];
}

void Bookmarks::getTagInfo(const TagInfo *tagInfo, const QString &tagName) const
{
    tagInfo = nullptr;
    if (tagName.isNull() || tagName.isEmpty())
        return;
    const int i = m_tagList.indexOf(tagName.trimmed());
    if (i < 0)
        return;
    tagInfo = &m_tagList[i];
}

bool Bookmarks::removeTag(const QString &tagName)
{
    int idx = getTagIndex(tagName);
    if (idx < 0)
        return false;

    // Do not delete "Untagged" tag.
    if(QString::compare(TagInfo::UNTAGGED, tagName.trimmed()) == 0)
        return false;

    // Delete Tag from all Bookmarks that use it.
    // TODO: we need this often, so don't loop over and over
    TagInfo *pTagToDelete = &m_tagList[idx];
    for(int i = 0; i < m_bookmarkList.size(); ++i)
    {
        BookmarkInfo &bmi = m_bookmarkList[i];
        for(int t = 0; t < bmi.tags.size(); ++t)
        {
            TagInfo *pTag = bmi.tags[t];
            if(pTag == pTagToDelete)
            {
                if(bmi.tags.size() > 1)
                    bmi.tags.removeAt(t);
                else
                    bmi.tags[0] = &findOrAddTag(TagInfo::UNTAGGED);
            }
        }
    }

    // Delete Tag.
    m_tagList.removeAt(idx);

    emit bookmarksChanged();
    emit tagListChanged();

    return true;
}

bool Bookmarks::setTagActive(const QString &tagName, bool is_active)
{
    TagInfo *tagInfo = nullptr;
    getTagInfo(tagInfo, tagName);
    if (!tagInfo)
        return false;

    tagInfo->active = is_active;

    emit bookmarksChanged();
    emit tagListChanged();
    return true;
}

int Bookmarks::getTagIndex(const QString &tagName)
{
    if (tagName.isNull() || tagName.isEmpty())
        return -1;
    return m_tagList.indexOf(tagName.trimmed());
}

const QColor &BookmarkInfo::getColor() const
{
    for(int i = 0; i < tags.size(); ++i)
    {
        if(tags[i]->active)
            return tags[i]->color;
    }
    return TagInfo::DEFAULT_COLOR;
}

bool BookmarkInfo::isActive() const
{
    for(int i = 0; i < tags.size(); ++i)
    {
        if(tags[i]->active)
            return true;
    }
    return false;
}
