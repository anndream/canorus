/** @file core/functionmarking.cpp
 * 
 * Copyright (c) 2006, Matevž Jekovec, Canorus development team
 * All Rights Reserved. See AUTHORS for a complete list of authors.
 * 
 * Licensed under the GNU GENERAL PUBLIC LICENSE. See COPYING for details.
 */

#include "core/functionmarking.h"
#include "core/functionmarkingcontext.h"

CAFunctionMarking::CAFunctionMarking(CAFunctionType function, const QString key, CAFunctionMarkingContext* context, int timeStart, int timeLength, CAFunctionType chordArea, bool chordAreaMinor, CAFunctionType tonicDegree, bool minor, bool ellipseSequence)
 : CAMusElement(context, timeStart, timeLength) {
 	_musElementType = CAMusElement::FunctionMarking;
 	
 	_function = function;
 	_tonicDegree = tonicDegree;
 	_key = key;
	_chordArea = chordArea;
	_chordAreaMinor = chordAreaMinor;
	_minor = minor;
	_ellipseSequence = ellipseSequence;
}

CAFunctionMarking::~CAFunctionMarking() {
}

bool CAFunctionMarking::isSideDegree() {
	if (_function==I ||
	    _function==II ||
	    _function==III ||
	    _function==IV ||
	    _function==V ||
	    _function==VI ||
	    _function==VII)
		return true;
	else
		return false;
}

CAFunctionMarking *CAFunctionMarking::clone() {
	return new CAFunctionMarking(function(), key(), (CAFunctionMarkingContext*)_context, timeStart(), timeLength(), chordArea(), isChordAreaMinor(), tonicDegree(), isMinor(), isPartOfEllipse());
}

int CAFunctionMarking::compare(CAMusElement *func) {
	return 0;	//TODO
}