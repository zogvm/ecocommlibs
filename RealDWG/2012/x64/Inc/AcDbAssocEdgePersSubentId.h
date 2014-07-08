// $Header: //depot/release/ironman2012/Develop/global/inc/dbxsdk/AcDbAssocEdgePersSubentId.h#1 $ 
// $Change: 237375 $
// $DateTime: 2011/01/30 18:32:54 $ $Author: integrat $ 
// $NoKeywords: $ 
//
//////////////////////////////////////////////////////////////////////////////
//
// Copyright � 2007-2011 by Autodesk, Inc. 
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
// CREATED BY: - Randy Kintzley 10/16/2008
//
// DESCRIPTION:
//
// AcDbAssocEdgePersSubentId concrete derived class.
//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "acarray.h"
#include "dbsubeid.h"
#include "AcDbAssocPersSubentId.h"
#pragma pack (push, 8)


/// <summary>
/// Concrete derived AcDbAssocPersSubentId class that is used to identify edge 
/// subentities by indices of its two end vertices, or generally, by two arbitrary
/// index identifiers.
/// </summary>
///
class ACDB_PORT AcDbAssocEdgePersSubentId : public AcDbAssocPersSubentId
{
public:
	ACRX_DECLARE_MEMBERS(AcDbAssocEdgePersSubentId);

    AcDbAssocEdgePersSubentId();

    /// <summary> 
    /// Constructor that creates an AcDbAssocEdgePersSubentId and initializes it 
    /// by indices of the two edge vertices, or generally, by two arbitrary index
    /// identifiers. 
    /// If either index is 0, it means "no index". Otherwise it is up to the 
    /// client code to interpret the meaning of these two index identifiers.
    ///
    /// Polyline vertex subentities are identified by vertexIdentifier2 being 0.
    /// Polyline segment midpoint and segment arc center vertex subentities are
    /// identified by both vertexIdentifier1/2 being not 0 and by a special flag.
    /// </summary>
    /// <param  name="vertexIdentifier1"> Identifier of the start vertex of the edge. </param>
    /// <param  name="vertexIdentifier2"> Identifier of the end vertex of the edge. </param>
    ///
    explicit AcDbAssocEdgePersSubentId(Adesk::Int32 vertexIdentifier1,
                                       Adesk::Int32 vertexIdentifier2);

    explicit AcDbAssocEdgePersSubentId(Adesk::Int32 vertexIdentifier1,
                                       Adesk::Int32 vertexIdentifier2,
                                       bool isSegArcCenter,
                                       bool isSegMidpoint);

    /// <summary>
    /// Returns number 1 if not null or 0 if null.
    /// </summary>
    /// <param  name="pEntity"> Not used. </param>
    /// <returns> Returns count 1 or 0. </returns>
    ///
    virtual int transientSubentCount(const AcDbEntity* pEntity) const { return !isNull() ? 1 : 0; }

    /// <summary> Returns AcDb::kEdgeSubentType. </summary>
    /// <param  name="pEntity"> Not used. </param>
    /// <returns> AcDb::kEdgeSubentType. </returns>
    ///
    virtual AcDb::SubentType subentType(const AcDbEntity* pEntity) const;

    /// <summary> 
    /// Returns true iff the AcDbAssocEdgePersSubentId does not identify any subentity.
    /// </summary>
    /// <returns> True iff the AcDbAssocEdgePersSubentId does not identify any subentity. </returns>
    ///
    virtual bool isNull() const { return mIndex1 == 0; }

    /// <summary>
    /// Returns true iff this and the other AcDbAssocEdgePersSubentId reference
    /// exactly the same subentity of the same entity. It simply compares the 
    /// stored indices.
    /// </summary>
    /// <param  name="pEntity"> 
    /// The entity owning the subentities of this and the other AcDbAssocPersSubentId. 
    /// </param>
    /// <param  name="pOther"> The other AcDbAssocEdgePersSubentId. </param>
    /// <returns> True iff this and the other AcDbAssocEdgePersSubentId are equal. </returns>
    ///
    virtual bool isEqualTo(const AcDbEntity* pEntity, const AcDbAssocPersSubentId* pOther) const
    {
        if (!AcDbAssocPersSubentId::isEqualTo(pEntity, pOther))
            return false;
    
        const AcDbAssocEdgePersSubentId* const pOtherEdge = dynamic_cast<const AcDbAssocEdgePersSubentId*>(pOther);
        return ( VERIFY(pOtherEdge != NULL)
                && ((mIndex1 == pOtherEdge->mIndex1) && (mIndex2 == pOtherEdge->mIndex2))
               );
    }

    /// <summary> The standard filing protocol. </summary>
    /// <param  name="pFiler"> The filer to write the object data to. </param>
    /// <returns> Acad::ErrorStatus. </returns>
    ///
    virtual Acad::ErrorStatus dwgOutFields(AcDbDwgFiler* pFiler) const;

    /// <summary> The standard filing protocol. </summary>
    /// <param  name="pFiler"> The filer to read the object data from. </param>
    /// <returns> Acad::ErrorStatus. </returns>
    ///
    virtual Acad::ErrorStatus dwgInFields(AcDbDwgFiler* pFiler);

    /// <summary> The standard filing protocol. </summary>
    /// <param  name="pFiler"> The filer to write the object data to. </param>
    /// <returns> Acad::ErrorStatus. </returns>
    ///
    virtual Acad::ErrorStatus dxfOutFields(AcDbDxfFiler* pFiler) const;

    /// <summary> The standard filing protocol. </summary>
    /// <param  name="pFiler"> The filer to read the object data from. </param>
    /// <returns> Acad::ErrorStatus. </returns>
    ///
    virtual Acad::ErrorStatus dxfInFields(AcDbDxfFiler* pFiler);

    /// <summary> The standard protocol. </summary>
    /// <param  name="pAuditInfo"> See the AcDbAuditInfo documentation. </param>
    /// <returns> Acad::ErrorStatus. </returns>
    ///
    virtual Acad::ErrorStatus audit(AcDbAuditInfo* pAuditInfo);

    /// <summary> Returns the value of the internal index1 data member. </summary>
    /// <returns> Value of the internal index1 data member. </returns>
    ///
    Adesk::GsMarker index1() const;

    /// <summary> Returns the value of the internal index2 data member. </summary>
    /// <returns> Value of the internal index2 data member. </returns>
    ///
    Adesk::GsMarker index2() const;

    /// <summary> 
    /// Returns true if the AcDbAssocEdgePersSubentId identifies a vertex subentity 
    /// that is the center of an arc polyline segment. 
    /// </summary>
    ///
    bool isSegmentArcCenter() const;

    /// <summary> 
    /// Returns true if the AcDbAssocEdgePersSubentId identifies a vertex subentity
    /// that is the midpoint of a polyline segment. 
    /// </summary>
    ///
    bool isSegmentMidpoint () const;

private:
    Adesk::GsMarker  mIndex1; // vertex 1
    Adesk::GsMarker  mIndex2; // vertex 2

}; // class AcDbAssocEdgePersSubentId

#pragma pack (pop)

