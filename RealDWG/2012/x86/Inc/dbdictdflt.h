// $Header: //depot/release/ironman2012/develop/global/inc/dbxsdk/dbdictdflt.h#1 $ $Change: 237375 $ $DateTime: 2011/01/30 18:32:54 $ $Author: integrat $
// $NoKeywords: $
#ifndef AD_DBDICTDFLT_H
#define AD_DBDICTDFLT_H
//
// (C) Copyright 1998-1999 by Autodesk, Inc.
//
// Permission to use, copy, modify, and distribute this software in
// object code form for any purpose and without fee is hereby granted, 
// provided that the above copyright notice appears in all copies and 
// that both that copyright notice and the limited warranty and
// restricted rights notice below appear in all supporting 
// documentation.
//
// AUTODESK PROVIDES THIS PROGRAM "AS IS" AND WITH ALL FAULTS. 
// AUTODESK SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTY OF
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR USE.  AUTODESK, INC. 
// DOES NOT WARRANT THAT THE OPERATION OF THE PROGRAM WILL BE
// UNINTERRUPTED OR ERROR FREE.
//
// Use, duplication, or disclosure by the U.S. Government is subject to 
// restrictions set forth in FAR 52.227-19 (Commercial Computer
// Software - Restricted Rights) and DFAR 252.227-7013(c)(1)(ii)
// (Rights in Technical Data and Computer Software), as applicable.
//
//

#include "dbdict.h"

#pragma pack(push, 8)

class AcDbImpDictionaryWithDefault;

class AcDbDictionaryWithDefault : public AcDbDictionary
{
public:
    ACDB_DECLARE_MEMBERS(AcDbDictionaryWithDefault);
    AcDbDictionaryWithDefault();
    virtual ~AcDbDictionaryWithDefault();

    Acad::ErrorStatus setDefaultId(const AcDbObjectId& newId);
    AcDbObjectId defaultId() const;

    virtual Acad::ErrorStatus getObjectBirthVersion(
                                 AcDb::AcDbDwgVersion& ver,
                                 AcDb::MaintenanceReleaseVersion& maintVer);
};

#pragma pack(pop)

#endif