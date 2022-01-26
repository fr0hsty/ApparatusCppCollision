



#include "Physics/PhysicsFiltering.h"

FCollisionFilterData EthanCreateObjectQueryFilterData(const bool bTraceComplex, const int32 MultiTrace/*=1 if multi. 0 otherwise*/, const struct FCollisionObjectQueryParams & ObjectParam)
{
	/**
	* Format for QueryData :
	*		word0 (meta data - ECollisionQuery. Extendable)
	*
	*		For object queries
	*
	*		word1 (object type queries)
	*		word2 (unused)
	*		word3 (Multi (1) or single (0) (top 8) + Flags (lower 24))
	*/

	FCollisionFilterData NewData;

	NewData.Word0 = (uint32)ECollisionQuery::ObjectQuery;

	if (bTraceComplex)
	{
		NewData.Word3 |= EPDF_ComplexCollision;
	}
	else
	{
		NewData.Word3 |= EPDF_SimpleCollision;
	}

	// get object param bits
	NewData.Word1 = ObjectParam.GetQueryBitfield();

	// if 'nothing', then set no bits
	NewData.Word3 |= CreateChannelAndFilter((ECollisionChannel)MultiTrace, ObjectParam.IgnoreMask);

	return NewData;
}
