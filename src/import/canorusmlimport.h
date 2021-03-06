/*!
	Copyright (c) 2006-2009, Matevž Jekovec, Canorus development team
	All Rights Reserved. See AUTHORS for a complete list of authors.

	Licensed under the GNU GENERAL PUBLIC LICENSE. See LICENSE.GPL for details.
*/

#ifndef CANORUSMLIMPORT_H_
#define CANORUSMLIMPORT_H_

#include <QColor>
#include <QHash>
#include <QStack>
#include <QVersionNumber>
#include <QXmlDefaultHandler>

#include "import/import.h"

#include "score/diatonickey.h"
#include "score/diatonicpitch.h"
#include "score/playablelength.h"

class CAContext;
class CAKeySignature;
class CATimeSignature;
class CAClef;
class CABarline;
class CANote;
class CARest;
class CASlur;
class CASyllable;
class CAMusElement;
class CAMark;
class CATuplet;

class CACanorusMLImport : public CAImport, public QXmlDefaultHandler {
public:
    CACanorusMLImport(QTextStream* stream = 0);
    CACanorusMLImport(const QString stream);
    virtual ~CACanorusMLImport();

    void initCanorusMLImport();

    CADocument* importDocumentImpl();

    bool startElement(const QString& namespaceURI, const QString& localName, const QString& qName,
        const QXmlAttributes& attributes);
    bool endElement(const QString& namespaceURI, const QString& localName,
        const QString& qName);
    bool fatalError(const QXmlParseException& exception);
    bool characters(const QString& ch);

private:
    void importMark(const QXmlAttributes& attributes);
    void importResource(const QXmlAttributes& attributes);

    inline CADocument* document() { return _document; }
    CADocument* _document;

    QVersionNumber _version; // version of Canorus the imported file was created with
    QString _errorMsg;
    QStack<QString> _depth;

    // Pointers to the current elements when reading the XML file
    CASheet* _curSheet;
    CAContext* _curContext;
    CAVoice* _curVoice;
    CAKeySignature* _curKeySig;
    CATimeSignature* _curTimeSig;
    CAClef* _curClef;
    CABarline* _curBarline;
    CANote* _curNote;
    CARest* _curRest;
    CAMusElement* _curMusElt;
    CAMusElement* _prevMusElt; // previous musElt by depth
    CAMark* _curMark;
    CASlur* _curTie;
    CASlur* _curSlur;
    CATuplet* _curTuplet;
    CASlur* _curPhrasingSlur;
    CADiatonicPitch _curDiatonicPitch;
    CADiatonicKey _curDiatonicKey;
    CAPlayableLength _curPlayableLength;
    CAPlayableLength _curTempoPlayableLength;
    QHash<CALyricsContext*, int> _lcMap; // lyrics context associated voice indices
    QHash<CASyllable*, int> _syllableMap; // syllable associated voice indices
    QColor _color; // foreground color of elements

    //////////////////////////////////////////////
    // Temporary properties for each XML stanza //
    //////////////////////////////////////////////
    QString _cha;
};

#endif /* CANORUSMLIMPORT_H_ */
