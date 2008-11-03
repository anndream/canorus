/*!
	Copyright (c) 2008, Reinahrd Katzmann, Matevž Jekovec, Canorus development team
	All Rights Reserved. See AUTHORS for a complete list of authors.

	Licensed under the GNU GENERAL PUBLIC LICENSE. See LICENSE.GPL for details.
*/

#ifndef PDFEXPORT_H_
#define PDFEXPORT_H_

// Includes
#include "export/export.h"

// Forward declarations
class CATypesetCtl;

// PDF Export class doing lilypond export internally
// !! exportDocument does not support threading !!
class CAPDFExport : public CAExport {
	Q_OBJECT

public:
	CAPDFExport( QTextStream *stream=0 );
	~CAPDFExport();

  QString getTempFilePath();

signals:
	void pdfIsFinished( int iExitCode );

protected slots:
	void outputTypsetterOutput( const QByteArray &roOutput );
	void pdfFinished( int iExitCode );

private:
	void exportDocumentImpl(CADocument *doc);

protected:
	CATypesetCtl *_poTypesetCtl;
};

#endif // PDFEXPORT_H_
