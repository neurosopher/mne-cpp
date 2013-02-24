//=============================================================================================================
/**
* @file		measurement.cpp
* @author	Christoph Dinh <christoph.dinh@live.de>;
* @version	1.0
* @date		October, 2010
*
* @section	LICENSE
*
* Copyright (C) 2010 Christoph Dinh. All rights reserved.
*
* No part of this program may be photocopied, reproduced,
* or translated to another program language without the
* prior written consent of the author.
*
*
* @brief	Contains the implementation of the Measurement base class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "measurement.h"
#include "../DesignPatterns/observerpattern.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Measurement::Measurement()
: Subject()
, m_qString_Name("")
//, m_MDL_ID(MDL_ID::_default)
, m_MSR_ID(MSR_ID::_default)
, m_bVisibility(true)
{

}


//*************************************************************************************************************

Measurement::~Measurement()
{

}
