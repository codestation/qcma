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

#include "sforeader.h"

#include <QFile>

SfoReader::SfoReader()
{
}

bool SfoReader::load(const QString &path)
{
    QFile file(path);
    if(file.open(QIODevice::ReadOnly)) {
        data = file.readAll();
        key_offset = data.constData();
        header = (sfo_header *)key_offset;
        index = (sfo_index *)(key_offset + sizeof(sfo_header));
        return true;
    }
    return false;
}

const char *SfoReader::value(const char *key, const char *defaultValue)
{
    const char *base_key = key_offset + header->key_offset;
    for(uint i = 0; i < header->pair_count; i++) {
        const char *curr_key = base_key + index[i].key_offset;
        if(strcmp(key, curr_key) == 0) {
            return key_offset + header->value_offset + index[i].data_offset;
        }
    }
    return defaultValue;
}
