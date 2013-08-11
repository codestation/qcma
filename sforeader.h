/*
 *  QCMA: Cross-platform content manager assistant for the PS Vita
 *
 *  Copyright (C) 2013  Codestation
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SFOREADER_H
#define SFOREADER_H

#include <QString>

class SfoReader
{
public:
    SfoReader();
    bool load(const QString &path);
    const char *value(const char *key, const char *defaultValue);

private:
    typedef struct {
        quint16 key_offset;
        uchar alignment;
        uchar data_type;
        quint32 value_size;
        quint32 value_size_with_padding;
        quint32 data_offset;
    }__attribute__((packed)) sfo_index;

    typedef struct {
        char id[4];
        quint32 version;
        quint32 key_offset;
        quint32 value_offset;
        quint32 pair_count;
    }__attribute__((packed)) sfo_header;

    QByteArray data;
    const char *key_offset;
    const sfo_header *header;
    const sfo_index *index;
};

#endif // SFOREADER_H
