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

#ifndef CMAROOTOBJECT_H
#define CMAROOTOBJECT_H

#include "cmaobject.h"

#include <QList>

#include <vitamtp.h>

class CMARootObject : public CMAObject
{
public:
    explicit CMARootObject(int ohfi);
    ~CMARootObject();

    void initObject(const QString &path);
    void remove(const CMAObject *obj);
    int getFilters(metadata_t **p_head);

    int num_filters;
    metadata_t *filters;
    static QString uuid;

private:
    void createFilter(metadata_t *filter, const char *name, int type);

    int root_ohfi;
};

#endif // CMAROOTOBJECT_H
