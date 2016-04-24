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
#include <QtEndian>

template<class T> class uilsb
{
public:
    operator T () const { return qFromLittleEndian<T>(data); }
    const T operator=(const T v) {
        qToLittleEndian<T>(v, data);
        return v;
    }

private:
    static const int nBytes = sizeof(T);
    uchar data[nBytes];
};

class SfoReader
{
public:
    SfoReader();
    bool load(const QString &path);
    const char *value(const char *key, const char *defaultValue);

private:
    typedef struct {
        uilsb<quint16> key_offset;
        uchar alignment;
        uchar data_type;
        uilsb<quint32> value_size;
        uilsb<quint32> value_size_with_padding;
        uilsb<quint32> data_offset;
    } sfo_index;

    typedef struct {
        char id[4];
        uilsb<quint32> version;
        uilsb<quint32> key_offset;
        uilsb<quint32> value_offset;
        uilsb<quint32> pair_count;
    } sfo_header;

    QByteArray data;
    const char *key_offset;
    const sfo_header *header;
    const sfo_index *index;
};

#endif // SFOREADER_H
