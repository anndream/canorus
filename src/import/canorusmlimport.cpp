/*!
	Copyright (c) 2006-2020, Matevž Jekovec, Canorus development team
	All Rights Reserved. See AUTHORS for a complete list of authors.

	Licensed under the GNU GENERAL PUBLIC LICENSE. See LICENSE.GPL for details.
*/

#include <QDebug>
#include <QFileInfo>
#include <QIODevice>
#include <QVariant>
#include <QVersionNumber>

#include "import/canorusmlimport.h"

#include "control/resourcectl.h"

#include "score/barline.h"
#include "score/clef.h"
#include "score/context.h"
#include "score/document.h"
#include "score/keysignature.h"
#include "score/muselement.h"
#include "score/note.h"
#include "score/resource.h"
#include "score/rest.h"
#include "score/sheet.h"
#include "score/staff.h"
#include "score/timesignature.h"
#include "score/voice.h"

#include "score/articulation.h"
#include "score/bookmark.h"
#include "score/crescendo.h"
#include "score/dynamic.h"
#include "score/fermata.h"
#include "score/fingering.h"
#include "score/instrumentchange.h"
#include "score/mark.h"
#include "score/repeatmark.h"
#include "score/ritardando.h"
#include "score/tempo.h"
#include "score/text.h"

#include "score/lyricscontext.h"
#include "score/syllable.h"

#include "score/figuredbasscontext.h"
#include "score/figuredbassmark.h"

#include "score/functionmark.h"
#include "score/functionmarkcontext.h"

#include "score/chordname.h"
#include "score/chordnamecontext.h"

/*!
	\class CACanorusMLImport
	\brief Class for opening the Canorus documents

	CACanorusMLImport class opens the XML based Canorus documents.
	It uses SAX parser for reading.

	\sa CAImport, CACanorusMLExport
*/

CACanorusMLImport::CACanorusMLImport(QTextStream* stream)
    : CAImport(stream)
    , QXmlDefaultHandler()
{
    initCanorusMLImport();
}

CACanorusMLImport::CACanorusMLImport(const QString stream)
    : CAImport(stream)
    , QXmlDefaultHandler()
{
    initCanorusMLImport();
}

CACanorusMLImport::~CACanorusMLImport()
{
}

void CACanorusMLImport::initCanorusMLImport()
{
    _document = nullptr;
    _curSheet = nullptr;
    _curContext = nullptr;
    _curVoice = nullptr;

    _curMusElt = nullptr;
    _prevMusElt = nullptr;
    _curMark = nullptr;
    _curClef = nullptr;
    _curTimeSig = nullptr;
    _curKeySig = nullptr;
    _curBarline = nullptr;
    _curNote = nullptr;
    _curRest = nullptr;
    _curTie = nullptr;
    _curSlur = nullptr;
    _curPhrasingSlur = nullptr;
    _curTuplet = nullptr;
}

CADocument* CACanorusMLImport::importDocumentImpl()
{
    QIODevice* device = stream()->device();
    QXmlInputSource* src;
    if (device)
        src = new QXmlInputSource(device);
    else {
        src = new QXmlInputSource();
        src->setData(*stream()->string());
    }
    QXmlSimpleReader* reader = new QXmlSimpleReader();
    reader->setContentHandler(this);
    reader->setErrorHandler(this);
    reader->parse(src);

    if (document() && !_fileName.isEmpty()) {
        document()->setFileName(_fileName);
    }

    delete reader;
    delete src;

    return document();
}

/*!
	This method should be called when a critical error occurs while parsing the XML source.

	\sa startElement(), endElement()
*/
bool CACanorusMLImport::fatalError(const QXmlParseException& exception)
{
    qWarning() << "Fatal error on line " << exception.lineNumber()
               << ", column " << exception.columnNumber() << ": "
               << exception.message() << "\n\nParser message:\n"
               << _errorMsg;

    return false;
}

/*!
	This function is called automatically by Qt SAX parser while reading the CanorusML
	source. This function is called when a new node is opened. It already reads node
	attributes.

	The function returns true, if the node was successfully recognized and parsed;
	otherwise false.

	\sa endElement()
*/
bool CACanorusMLImport::startElement(const QString&, const QString&, const QString& qName, const QXmlAttributes& attributes)
{
    if (attributes.value("color") != "") {
        _color = QVariant(attributes.value("color")).value<QColor>();
        if (_version <= QVersionNumber(0, 7, 3)) {
            // before Canorus 0.7.4, color was incorrectly saved (always #000000)
            _color = QColor();
        }
    } else {
        _color = QColor();
    }

    if (qName == "document") {
        // CADocument
        _document = new CADocument();
        _document->setTitle(attributes.value("title"));
        _document->setSubtitle(attributes.value("subtitle"));
        _document->setComposer(attributes.value("composer"));
        _document->setArranger(attributes.value("arranger"));
        _document->setPoet(attributes.value("poet"));
        _document->setTextTranslator(attributes.value("text-translator"));
        _document->setCopyright(attributes.value("copyright"));
        _document->setDedication(attributes.value("dedication"));
        _document->setComments(attributes.value("comments"));

        _document->setDateCreated(QDateTime::fromString(attributes.value("date-created"), Qt::ISODate));
        _document->setDateLastModified(QDateTime::fromString(attributes.value("date-last-modified"), Qt::ISODate));
        _document->setTimeEdited(attributes.value("time-edited").toUInt());

    } else if (qName == "sheet") {
        // CASheet
        QString sheetName = attributes.value("name");

        if (sheetName.isEmpty())
            sheetName = QObject::tr("Sheet%1").arg(_document->sheetList().size() + 1);
        _curSheet = new CASheet(sheetName, _document);

        _document->addSheet(_curSheet);

    } else if (qName == "staff") {
        // CAStaff
        QString staffName = attributes.value("name");
        if (!_curSheet) {
            _errorMsg = "The sheet where to add the staff doesn't exist yet!";
            return false;
        }

        if (staffName.isEmpty())
            staffName = QObject::tr("Staff%1").arg(_curSheet->staffList().size() + 1);
        _curContext = new CAStaff(staffName, _curSheet, attributes.value("number-of-lines").toInt());

        _curSheet->addContext(_curContext);

    } else if (qName == "lyrics-context") {
        // CALyricsContext
        QString lcName = attributes.value("name");
        if (!_curSheet) {
            _errorMsg = "The sheet where to add the lyrics context doesn't exist yet!";
            return false;
        }

        if (lcName.isEmpty())
            lcName = QObject::tr("LyricsContext%1").arg(_curSheet->contextList().size() + 1);
        _curContext = new CALyricsContext(lcName, attributes.value("stanza-number").toInt(), _curSheet);

        // voices are not neccesseraly completely read - store indices of the voices internally and then assign them at the end
        if (!attributes.value("associated-voice-idx").isEmpty())
            _lcMap[static_cast<CALyricsContext*>(_curContext)] = attributes.value("associated-voice-idx").toInt();

        _curSheet->addContext(_curContext);

    } else if (qName == "figured-bass-context") {
        // CAFiguredBassContext
        QString fbcName = attributes.value("name");
        if (!_curSheet) {
            _errorMsg = "The sheet where to add the figured bass context doesn't exist yet!";
            return false;
        }

        if (fbcName.isEmpty())
            fbcName = QObject::tr("FiguredBassContext%1").arg(_curSheet->contextList().size() + 1);
        _curContext = new CAFiguredBassContext(fbcName, _curSheet);

        _curSheet->addContext(_curContext);

    } else if (qName == "function-mark-context" || qName == "function-marking-context") {
        // CAFunctionMarkContext
        QString fmcName = attributes.value("name");
        if (!_curSheet) {
            _errorMsg = "The sheet where to add the function mark context doesn't exist yet!";
            return false;
        }

        if (fmcName.isEmpty())
            fmcName = QObject::tr("FunctionMarkContext%1").arg(_curSheet->contextList().size() + 1);
        _curContext = new CAFunctionMarkContext(fmcName, _curSheet);

        _curSheet->addContext(_curContext);

    } else if (qName == "chord-name-context") {
        // CAChordNameContext
        QString cncName = attributes.value("name");
        if (!_curSheet) {
            _errorMsg = "The sheet where to add the chord name context doesn't exist yet!";
            return false;
        }

        if (cncName.isEmpty())
            cncName = QObject::tr("ChordNameContext%1").arg(_curSheet->contextList().size() + 1);
        _curContext = new CAChordNameContext(cncName, _curSheet);

        _curSheet->addContext(_curContext);

    } else if (qName == "voice") {
        // CAVoice
        QString voiceName = attributes.value("name");
        if (!_curContext) {
            _errorMsg = "The context where the voice " + voiceName + " should be added doesn't exist yet!";
            return false;
        } else if (_curContext->contextType() != CAContext::Staff) {
            _errorMsg = "The context type which contains voice " + voiceName + " isn't staff!";
            return false;
        }

        CAStaff* staff = static_cast<CAStaff*>(_curContext);

        int voiceNumber = staff->voiceList().size() + 1;

        if (voiceName.isEmpty())
            voiceName = QObject::tr("Voice%1").arg(voiceNumber);

        CANote::CAStemDirection stemDir = CANote::StemNeutral;
        if (!attributes.value("stem-direction").isEmpty())
            stemDir = CANote::stemDirectionFromString(attributes.value("stem-direction"));

        _curVoice = new CAVoice(voiceName, staff, stemDir);
        if (!attributes.value("midi-channel").isEmpty()) {
            _curVoice->setMidiChannel(static_cast<unsigned char>(attributes.value("midi-channel").toUInt()));
        }
        if (!attributes.value("midi-program").isEmpty()) {
            _curVoice->setMidiProgram(static_cast<unsigned char>(attributes.value("midi-program").toUInt()));
        }
        if (!attributes.value("midi-pitch-offset").isEmpty()) {
            _curVoice->setMidiPitchOffset(static_cast<char>(attributes.value("midi-pitch-offset").toInt()));
        }

        staff->addVoice(_curVoice);

    } else if (qName == "clef") {
        // CAClef
        _curClef = new CAClef(CAClef::clefTypeFromString(attributes.value("clef-type")),
            attributes.value("c1").toInt(),
            _curVoice->staff(),
            attributes.value("time-start").toInt(),
            attributes.value("offset").toInt());
        _curMusElt = _curClef;
        _curMusElt->setColor(_color);
    } else if (qName == "time-signature") {
        // CATimeSignature
        _curTimeSig = new CATimeSignature(attributes.value("beats").toInt(),
            attributes.value("beat").toInt(),
            _curVoice->staff(),
            attributes.value("time-start").toInt(),
            CATimeSignature::timeSignatureTypeFromString(attributes.value("time-signature-type")));
        _curMusElt = _curTimeSig;
        _curMusElt->setColor(_color);
    } else if (qName == "key-signature") {
        // CAKeySignature
        CAKeySignature::CAKeySignatureType type = CAKeySignature::keySignatureTypeFromString(attributes.value("key-signature-type"));
        switch (type) {
        case CAKeySignature::MajorMinor: {
            _curKeySig = new CAKeySignature(CADiatonicKey(),
                _curVoice->staff(),
                attributes.value("time-start").toInt());
            break;
        }
        case CAKeySignature::Modus: {
            _curKeySig = new CAKeySignature(CAKeySignature::modusFromString(attributes.value("modus")),
                _curVoice->staff(),
                attributes.value("time-start").toInt());
            break;
        }
        case CAKeySignature::Custom:
            break;
        }

        _curMusElt = _curKeySig;
        _curMusElt->setColor(_color);
    } else if (qName == "barline") {
        // CABarline
        _curBarline = new CABarline(CABarline::barlineTypeFromString(attributes.value("barline-type")),
            _curVoice->staff(),
            attributes.value("time-start").toInt());
        _curMusElt = _curBarline;
    } else if (qName == "note") {
        // CANote
        if (QVersionNumber(0, 5).isPrefixOf(_version)) {
            _curNote = new CANote(CADiatonicPitch(attributes.value("pitch").toInt(), attributes.value("accs").toInt()),
                CAPlayableLength(CAPlayableLength::musicLengthFromString(attributes.value("playable-length")), attributes.value("dotted").toInt()),
                _curVoice,
                attributes.value("time-start").toInt(),
                attributes.value("time-length").toInt());
        } else {
            _curNote = new CANote(CADiatonicPitch(),
                CAPlayableLength(),
                _curVoice,
                attributes.value("time-start").toInt(),
                attributes.value("time-length").toInt());
        }

        if (!attributes.value("stem-direction").isEmpty()) {
            _curNote->setStemDirection(CANote::stemDirectionFromString(attributes.value("stem-direction")));
        }

        if (_curTuplet) {
            _curNote->setTuplet(_curTuplet);
            _curTuplet->addNote(_curNote);
        }

        _curMusElt = _curNote;
        _curMusElt->setColor(_color);
    } else if (qName == "tie") {
        _curTie = new CASlur(CASlur::TieType, CASlur::SlurPreferred, _curNote->staff(), _curNote, nullptr);
        _curNote->setTieStart(_curTie);
        if (!attributes.value("slur-style").isEmpty())
            _curTie->setSlurStyle(CASlur::slurStyleFromString(attributes.value("slur-style")));
        if (!attributes.value("slur-direction").isEmpty())
            _curTie->setSlurDirection(CASlur::slurDirectionFromString(attributes.value("slur-direction")));
        _prevMusElt = _curMusElt;
        _curMusElt = _curTie;
        _curMusElt->setColor(_color);
    } else if (qName == "slur-start") {
        _curSlur = new CASlur(CASlur::SlurType, CASlur::SlurPreferred, _curNote->staff(), _curNote, nullptr);
        _curNote->setSlurStart(_curSlur);
        if (!attributes.value("slur-style").isEmpty())
            _curSlur->setSlurStyle(CASlur::slurStyleFromString(attributes.value("slur-style")));
        if (!attributes.value("slur-direction").isEmpty())
            _curSlur->setSlurDirection(CASlur::slurDirectionFromString(attributes.value("slur-direction")));
        _prevMusElt = _curMusElt;
        _curMusElt = _curSlur;
        _curMusElt->setColor(_color);
    } else if (qName == "slur-end") {
        if (_curSlur) {
            _curNote->setSlurEnd(_curSlur);
            _curSlur->setNoteEnd(_curNote);
            _curSlur->setTimeLength(_curNote->timeStart() - _curSlur->noteStart()->timeStart());
            _curSlur = nullptr;
        }
    } else if (qName == "phrasing-slur-start") {
        _curPhrasingSlur = new CASlur(CASlur::PhrasingSlurType, CASlur::SlurPreferred, _curNote->staff(), _curNote, nullptr);
        _curNote->setPhrasingSlurStart(_curPhrasingSlur);
        if (!attributes.value("slur-style").isEmpty())
            _curPhrasingSlur->setSlurStyle(CASlur::slurStyleFromString(attributes.value("slur-style")));
        if (!attributes.value("slur-direction").isEmpty())
            _curPhrasingSlur->setSlurDirection(CASlur::slurDirectionFromString(attributes.value("slur-direction")));
        _prevMusElt = _curMusElt;
        _curMusElt = _curPhrasingSlur;
        _curMusElt->setColor(_color);
    } else if (qName == "phrasing-slur-end") {
        if (_curPhrasingSlur) {
            _curNote->setPhrasingSlurEnd(_curPhrasingSlur);
            _curPhrasingSlur->setNoteEnd(_curNote);
            _curPhrasingSlur->setTimeLength(_curNote->timeStart() - _curPhrasingSlur->noteStart()->timeStart());
            _curPhrasingSlur = nullptr;
        }
    } else if (qName == "tuplet") {
        _curTuplet = new CATuplet(attributes.value("number").toInt(), attributes.value("actual-number").toInt());
        _curTuplet->setColor(_color);
    } else if (qName == "rest") {
        // CARest
        if (QVersionNumber(0, 5).isPrefixOf(_version)) {
            _curRest = new CARest(CARest::restTypeFromString(attributes.value("rest-type")),
                CAPlayableLength(CAPlayableLength::musicLengthFromString(attributes.value("playable-length")), attributes.value("dotted").toInt()),
                _curVoice,
                attributes.value("time-start").toInt(),
                attributes.value("time-length").toInt());
        } else {
            _curRest = new CARest(CARest::restTypeFromString(attributes.value("rest-type")),
                CAPlayableLength(),
                _curVoice,
                attributes.value("time-start").toInt(),
                attributes.value("time-length").toInt());
        }

        if (_curTuplet) {
            _curRest->setTuplet(_curTuplet);
            _curTuplet->addNote(_curRest);
        }

        _curMusElt = _curRest;
        _curMusElt->setColor(_color);
    } else if (qName == "syllable") {
        // CASyllable
        CASyllable* s = new CASyllable(
            attributes.value("text"),
            attributes.value("hyphen") == "1",
            attributes.value("melisma") == "1",
            static_cast<CALyricsContext*>(_curContext),
            attributes.value("time-start").toInt(),
            attributes.value("time-length").toInt());
        // Note: associatedVoice property is set when finishing parsing the sheet

        static_cast<CALyricsContext*>(_curContext)->addSyllable(s);
        if (!attributes.value("associated-voice-idx").isEmpty())
            _syllableMap[s] = attributes.value("associated-voice-idx").toInt();
        _curMusElt = s;
        _curMusElt->setColor(_color);
    } else if (qName == "figured-bass-mark") {
        // CAFiguredBassMark
        CAFiguredBassMark* f = new CAFiguredBassMark(
            static_cast<CAFiguredBassContext*>(_curContext),
            attributes.value("time-start").toInt(),
            attributes.value("time-length").toInt());

        static_cast<CAFiguredBassContext*>(_curContext)->addFiguredBassMark(f);
        _curMusElt = f;
        _curMusElt->setColor(_color);

    } else if (qName == "figured-bass-number") {
        // CAFiguredBassMark
        CAFiguredBassMark* f = static_cast<CAFiguredBassMark*>(_curMusElt);
        if (attributes.value("accs").isEmpty()) {
            f->addNumber(attributes.value("number").toInt());
        } else {
            f->addNumber(attributes.value("number").toInt(), attributes.value("accs").toInt());
        }

    } else if (qName == "function-mark" || (QVersionNumber(0, 5).isPrefixOf(_version) && qName == "function-marking")) {
        // CAFunctionMark
        CAFunctionMark* f = new CAFunctionMark(
            CAFunctionMark::functionTypeFromString(attributes.value("function")),
            (attributes.value("minor") == "1" ? true : false),
            (QVersionNumber(0, 5).isPrefixOf(_version) ? (attributes.value("key").isEmpty() ? "C" : attributes.value("key")) : CADiatonicKey()),
            static_cast<CAFunctionMarkContext*>(_curContext),
            attributes.value("time-start").toInt(),
            attributes.value("time-length").toInt(),
            CAFunctionMark::functionTypeFromString(attributes.value("chord-area")),
            (attributes.value("chord-area-minor") == "1" ? true : false),
            CAFunctionMark::functionTypeFromString(attributes.value("tonic-degree")),
            (attributes.value("tonic-degree-minor") == "1" ? true : false),
            "",
            (attributes.value("ellipse") == "1" ? true : false));

        static_cast<CAFunctionMarkContext*>(_curContext)->addFunctionMark(f);
        _curMusElt = f;
        _curMusElt->setColor(_color);
    } else if (qName == "chord-name") {
        // CAChordName
        CAChordName* cn = new CAChordName(
            CADiatonicPitch(),
            attributes.value("quality-modifier"),
            static_cast<CAChordNameContext*>(_curContext),
            attributes.value("time-start").toInt(),
            attributes.value("time-length").toInt());

        _curMusElt = cn;
        _curMusElt->setColor(_color);
    } else if (qName == "mark") {
        // CAMark and subvariants
        importMark(attributes);
        _curMark->setColor(_color);
    } else if (qName == "playable-length") {
        CAPlayableLength pl = CAPlayableLength(CAPlayableLength::musicLengthFromString(attributes.value("music-length")), attributes.value("dotted").toInt());
        if (_depth.top() == "mark") {
            _curTempoPlayableLength = pl;
        } else {
            _curPlayableLength = pl;
        }
    } else if (qName == "diatonic-pitch") {
        _curDiatonicPitch = CADiatonicPitch(attributes.value("note-name").toInt(), attributes.value("accs").toInt());
    } else if (qName == "diatonic-key") {
        _curDiatonicKey = CADiatonicKey(CADiatonicPitch(), CADiatonicKey::genderFromString(attributes.value("gender")));
    } else if (qName == "resource") {
        importResource(attributes);
    }

    _depth.push(qName);
    return true;
}

/*!
	This function is called automatically by Qt SAX parser while reading the CanorusML
	source. This function is called when a node has been closed (\</nodeName\>). Attributes
	for closed notes are usually not set in CanorusML format. That's why we need to store
	local node attributes (set when the node is opened) each time.

	The function returns true, if the node was successfully recognized and parsed;
	otherwise false.

	\sa startElement()
*/
bool CACanorusMLImport::endElement(const QString&, const QString&, const QString& qName)
{
    if (qName == "canorus-version") {
        // version of Canorus which saved the document
        _version = QVersionNumber::fromString(_cha);
    } else if (qName == "document") {
        //fix voice errors like shared voice elements not being present in both voices etc.
        for (int i = 0; _document && i < _document->sheetList().size(); i++) {
            for (int j = 0; j < _document->sheetList()[i]->staffList().size(); j++) {
                _document->sheetList()[i]->staffList()[j]->synchronizeVoices();
            }
        }
    } else if (qName == "sheet") {
        // CASheet
        QList<CAVoice*> voices = _curSheet->voiceList();
        QList<CALyricsContext*> lcs = _lcMap.keys();
        for (int i = 0; i < lcs.size(); i++) // assign voices from voice indices
            lcs.at(i)->setAssociatedVoice(voices.at(_lcMap[lcs[i]]));

        QList<CASyllable*> syllables = _syllableMap.keys();
        for (int i = 0; i < syllables.size(); i++) { // assign voices from voice indices
            if (_syllableMap[syllables[i]] >= 0 && _syllableMap[syllables[i]] < voices.count()) {
                syllables.at(i)->setAssociatedVoice(voices.at(_syllableMap[syllables[i]]));
            }
        }

        _lcMap.clear();
        _syllableMap.clear();
        _curSheet = nullptr;
    } else if (qName == "staff") {
        // CAStaff
        _curContext = nullptr;
    } else if (qName == "voice") {
        // CAVoice
        _curVoice = nullptr;
    }
    // Every voice *must* contain signs on their own (eg. a clef is placed in all voices, not just the first one).
    // The following code finds a sign with the same properties at the same time in other voices. If such a sign exists, only place a pointer to this sign in the current voice. Otherwise, add a sign to all the voices read so far.
    else if (qName == "clef") {
        // CAClef
        if (!_curContext || !_curVoice || _curContext->contextType() != CAContext::Staff) {
            return false;
        }

        // lookup an element with the same type at the same time
        QList<CAMusElement*> foundElts = static_cast<CAStaff*>(_curContext)->getEltByType(CAMusElement::Clef, _curClef->timeStart());
        CAMusElement* sign = nullptr;
        for (int i = 0; i < foundElts.size(); i++) {
            if (!foundElts[i]->compare(_curClef)) // element has exactly the same properties
                if (!_curVoice->musElementList().contains(foundElts[i])) { // if such an element already exists, it means there are two different with the same timestart
                    sign = foundElts[i];
                    break;
                }
        }

        if (!sign) {
            // the element doesn't exist yet - add it to all the voices
            _curVoice->append(_curClef);
        } else {
            //the element was found, insert only a reference to the current voice
            _curVoice->append(sign);
            delete _curClef;
            _curClef = nullptr;
        }
    } else if (qName == "key-signature") {
        // CAKeySignature
        if (!_curContext || !_curVoice || _curContext->contextType() != CAContext::Staff) {
            return false;
        }

        switch (_curKeySig->keySignatureType()) {
        case CAKeySignature::MajorMinor:
            _curKeySig->setDiatonicKey(_curDiatonicKey);
            break;
        case CAKeySignature::Modus:
        case CAKeySignature::Custom:
            break;
        }

        // lookup an element with the same type at the same time
        QList<CAMusElement*> foundElts = static_cast<CAStaff*>(_curContext)->getEltByType(CAMusElement::KeySignature, _curKeySig->timeStart());
        CAMusElement* sign = nullptr;
        for (int i = 0; i < foundElts.size(); i++) {
            if (!foundElts[i]->compare(_curKeySig)) // element has exactly the same properties
                if (!_curVoice->musElementList().contains(foundElts[i])) { // if such an element already exists, it means there are two different with the same timestart
                    sign = foundElts[i];
                    break;
                }
        }

        if (!sign) {
            // the element doesn't exist yet - add it to all the voices
            _curVoice->append(_curKeySig);
        } else {
            // the element was found, insert only a reference to the current voice
            _curVoice->append(sign);
            delete _curKeySig;
            _curKeySig = nullptr;
        }
    } else if (qName == "time-signature") {
        // CATimeSignature
        if (!_curContext || !_curVoice || _curContext->contextType() != CAContext::Staff) {
            return false;
        }

        // lookup an element with the same type at the same time
        QList<CAMusElement*> foundElts = static_cast<CAStaff*>(_curContext)->getEltByType(CAMusElement::TimeSignature, _curTimeSig->timeStart());
        CAMusElement* sign = nullptr;
        for (int i = 0; i < foundElts.size(); i++) {
            if (!foundElts[i]->compare(_curTimeSig)) // element has exactly the same properties
                if (!_curVoice->musElementList().contains(foundElts[i])) { // if such an element already exists, it means there are two different with the same timestart
                    sign = foundElts[i];
                    break;
                }
        }

        if (!sign) {
            // the element doesn't exist yet - add it to all the voices
            _curVoice->append(_curTimeSig);
        } else {
            // the element was found, insert only a reference to the current voice
            _curVoice->append(sign);
            delete _curTimeSig;
            _curTimeSig = nullptr;
        }
    } else if (qName == "barline") {
        // CABarline
        if (!_curContext || !_curVoice || _curContext->contextType() != CAContext::Staff) {
            return false;
        }

        // lookup an element with the same type at the same time
        QList<CAMusElement*> foundElts = static_cast<CAStaff*>(_curContext)->getEltByType(CAMusElement::Barline, _curBarline->timeStart());
        CAMusElement* sign = nullptr;
        for (int i = 0; i < foundElts.size(); i++) {
            if (!foundElts[i]->compare(_curBarline)) // element has exactly the same properties
                if (!_curVoice->musElementList().contains(foundElts[i])) { // if such an element already exists, it means there are two different with the same timestart
                    sign = foundElts[i];
                    break;
                }
        }

        if (!sign) {
            // the element doesn't exist yet - add it to all the voices
            _curVoice->append(_curBarline);
        } else {
            // the element was found, insert only a reference to the current voice
            _curVoice->append(sign);
            delete _curBarline;
            _curBarline = nullptr;
        }
    } else if (qName == "note") {
        // CANote
        if (QVersionNumber(0, 5).isPrefixOf(_version)) {
        } else {
            _curNote->setPlayableLength(_curPlayableLength);
            if (!_curNote->tuplet()) {
                _curNote->calculateTimeLength();
            }
            _curNote->setDiatonicPitch(_curDiatonicPitch);
        }

        if (_curVoice->lastNote() && _curVoice->lastNote()->timeStart() == _curNote->timeStart())
            _curVoice->append(_curNote, true);
        else
            _curVoice->append(_curNote, false);

        _curNote->updateTies();
        _curNote = nullptr;
    } else if (qName == "tie") {
        // CASlur - tie
    } else if (qName == "tuplet") {
        _curTuplet->assignTimes();
        _curTuplet = nullptr;
    } else if (qName == "rest") {
        // CARest
        if (QVersionNumber(0, 5).isPrefixOf(_version)) {
        } else {
            _curRest->setPlayableLength(_curPlayableLength);
            if (!_curRest->tuplet()) {
                _curRest->calculateTimeLength();
            }
        }

        _curVoice->append(_curRest);
        _curRest = nullptr;
    } else if (qName == "mark") {
        if (!QVersionNumber(0, 5).isPrefixOf(_version) && _curMark->markType() == CAMark::Tempo) {
            static_cast<CATempo*>(_curMark)->setBeat(_curTempoPlayableLength);
        }
    } else if (qName == "function-mark") {
        if (!QVersionNumber(0, 5).isPrefixOf(_version) && _curMusElt->musElementType() == CAMusElement::FunctionMark) {
            static_cast<CAFunctionMark*>(_curMusElt)->setKey(_curDiatonicKey);
        }
    } else if (qName == "diatonic-key") {
        _curDiatonicKey.setDiatonicPitch(_curDiatonicPitch);
    } else if (qName == "chord-name") {
        CAChordName* cn = static_cast<CAChordName*>(_curMusElt);
        cn->setDiatonicPitch(_curDiatonicPitch);
        static_cast<CAChordNameContext*>(_curContext)->addChordName(cn);
    }

    _cha = "";
    _depth.pop();

    if (_prevMusElt) {
        _curMusElt = _prevMusElt;
        _prevMusElt = nullptr;
    }
    return true;
}

/*!
	Stores the characters between the greater-lesser signs while parsing the XML file.
	This is usually needed for getting the property values stored not as node attributes,
	but between greater-lesser signs.

	eg.
	\code
		<length>127</length>
	\endcode
	Would set _cha value to "127".

	\sa startElement(), endElement()
*/
bool CACanorusMLImport::characters(const QString& ch)
{
    _cha = ch;

    return true;
}

void CACanorusMLImport::importMark(const QXmlAttributes& attributes)
{
    CAMark::CAMarkType type = CAMark::markTypeFromString(attributes.value("mark-type"));
    _curMark = nullptr;

    switch (type) {
    case CAMark::Text: {
        _curMark = new CAText(
            attributes.value("text"),
            static_cast<CAPlayable*>(_curMusElt));
        break;
    }
    case CAMark::Tempo: {
        if (QVersionNumber(0, 5).isPrefixOf(_version)) {
            _curMark = new CATempo(
                CAPlayableLength(CAPlayableLength::musicLengthFromString(attributes.value("beat")), attributes.value("beat-dotted").toInt()),
                static_cast<unsigned char>(attributes.value("bpm").toUInt()),
                _curMusElt);
        } else {
            _curMark = new CATempo(
                CAPlayableLength(),
                static_cast<unsigned char>(attributes.value("bpm").toUInt()),
                _curMusElt);
        }
        break;
    }
    case CAMark::Ritardando: {
        _curMark = new CARitardando(
            attributes.value("final-tempo").toInt(),
            static_cast<CAPlayable*>(_curMusElt),
            attributes.value("time-length").toInt(),
            CARitardando::ritardandoTypeFromString(attributes.value("ritardando-type")));
        break;
    }
    case CAMark::Dynamic: {
        _curMark = new CADynamic(
            attributes.value("text"),
            attributes.value("volume").toInt(),
            static_cast<CANote*>(_curMusElt));
        break;
    }
    case CAMark::Crescendo: {
        _curMark = new CACrescendo(
            attributes.value("final-volume").toInt(),
            static_cast<CANote*>(_curMusElt),
            CACrescendo::crescendoTypeFromString(attributes.value("crescendo-type")),
            attributes.value("time-start").toInt(),
            attributes.value("time-length").toInt());
        break;
    }
    case CAMark::Pedal: {
        _curMark = new CAMark(
            CAMark::Pedal,
            _curMusElt,
            attributes.value("time-start").toInt(),
            attributes.value("time-length").toInt());
        break;
    }
    case CAMark::InstrumentChange: {
        _curMark = new CAInstrumentChange(
            attributes.value("instrument").toInt(),
            static_cast<CANote*>(_curMusElt));
        break;
    }
    case CAMark::BookMark: {
        _curMark = new CABookMark(
            attributes.value("text"),
            _curMusElt);
        break;
    }
    case CAMark::RehersalMark: {
        _curMark = new CAMark(
            CAMark::RehersalMark,
            _curMusElt);
        break;
    }
    case CAMark::Fermata: {
        if (_curMusElt->isPlayable()) {
            _curMark = new CAFermata(
                static_cast<CAPlayable*>(_curMusElt),
                CAFermata::fermataTypeFromString(attributes.value("fermata-type")));
        } else if (_curMusElt->musElementType() == CAMusElement::Barline) {
            _curMark = new CAFermata(
                static_cast<CABarline*>(_curMusElt),
                CAFermata::fermataTypeFromString(attributes.value("fermata-type")));
        }
        break;
    }
    case CAMark::RepeatMark: {
        _curMark = new CARepeatMark(
            static_cast<CABarline*>(_curMusElt),
            CARepeatMark::repeatMarkTypeFromString(attributes.value("repeat-mark-type")),
            attributes.value("volta-number").toInt());
        break;
    }
    case CAMark::Articulation: {
        _curMark = new CAArticulation(
            CAArticulation::articulationTypeFromString(attributes.value("articulation-type")),
            static_cast<CANote*>(_curMusElt));
        break;
    }
    case CAMark::Fingering: {
        QList<CAFingering::CAFingerNumber> fingers;
        for (int i = 0; !attributes.value(QString("finger%1").arg(i)).isEmpty(); i++)
            fingers << CAFingering::fingerNumberFromString(attributes.value(QString("finger%1").arg(i)));

        _curMark = new CAFingering(
            fingers,
            static_cast<CANote*>(_curMusElt),
            attributes.value("original").toInt());
        break;
    }
    case CAMark::Undefined:
        break;
    }

    if (_curMark) {
        _curMusElt->addMark(_curMark);
    }
}

/*!
	Imports the current resource.
 */
void CACanorusMLImport::importResource(const QXmlAttributes& attributes)
{
    bool isLinked = attributes.value("linked").toInt();

    std::shared_ptr<CAResource> r;
    QUrl url = attributes.value("url");
    QString name = attributes.value("name");
    QString description = attributes.value("description");
    CAResource::CAResourceType type = CAResource::resourceTypeFromString(attributes.value("resource-type"));
    QString rUrl = url.toString();

    if (!isLinked && file()) {
        rUrl = QFileInfo(file()->fileName()).absolutePath() + "/" + url.toLocalFile();
    }

    r = CAResourceCtl::importResource(name, rUrl, isLinked, _document, type);
    r->setDescription(description);
}

/*!
	\fn CACanorusMLImport::document()
	Returns the newly created document when reading the XML file.
*/

/*!
	\var CACanorusMLImport::_cha
	Current characters being read using characters() method between the greater/lesser
	separators in XML file.

	\sa characters()
*/

/*!
	\var CACanorusMLImport::_depth
	Stack which represents the current depth of the document while SAX parsing. It contains
	the tag names as the values.

	\sa startElement(), endElement()
*/

/*!
	\var CACanorusMLImport::_errorMsg
	The error message content stored as QString, if the error happens.

	\sa fatalError()
*/

/*!
	\var CACanorusMLImport::_version
	Document program version - which Canorus saved the file?

	\sa startElement(), endElement()
*/

/*!
	\var CACanorusMLImport::_document
	Pointer to the document being read.

	\sa CADocument
*/
