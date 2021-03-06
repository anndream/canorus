/*!
        Copyright (c) 2006-2019, Reinhard Katzmann, Matevž Jekovec, Canorus development team
        All Rights Reserved. See AUTHORS for a complete list of authors.

        Licensed under the GNU GENERAL PUBLIC LICENSE. See COPYING for details.
*/

#ifndef EXTERN_PROGRAM_H
#define EXTERN_PROGRAM_H

// Includes
#include <QObject>
#include <QProcess>
#include <QStringList>

#include <memory>

// Forward declarations

// This class is used to run a program in the background
// and to get it's input/output via signals

// Class definition
class CAExternProgram : public QObject {
    Q_OBJECT

public:
    CAExternProgram(bool bRcvStdErr = true, bool bRcvStdOut = true);

    void setProgramName(const QString& roProgram);
    void setProgramPath(const QString& roPath);
    // Warning: Setting all parameters overwrites all
    // parameters added via addParameter method!
    void setParameters(const QStringList& roParams);
    void inline setParamDelimiter(QString oDelimiter = " ")
    {
        _oParamDelimiter = oDelimiter;
    }

    inline const QStringList& getParameters() { return _oParameters; }
    inline bool getRunning()
    {
        return _poExternProgram->state() == QProcess::Running;
    }
    inline const QString& getParamDelimiter() { return _oParamDelimiter; }
    int getExitState();

    void addParameter(const QString& roParam, bool bAddDelimiter = true);
    inline void clearParameters() { _oParameters.clear(); }
    bool execProgram(const QString& roCwd = ".");
    inline bool waitForFinished(int iMSecs) { return _poExternProgram->waitForFinished(iMSecs); }

signals:
    void nextOutput(const QByteArray& roData);
    void programExited(int iExitCode);

protected slots:
    void rcvProgramStdOut() { rcvProgramOutput(_poExternProgram->readAllStandardOutput()); }
    void rcvProgramStdErr() { rcvProgramOutput(_poExternProgram->readAllStandardError()); }
    void programError(QProcess::ProcessError) { programExited(); }
    void programFinished(int, QProcess::ExitStatus) { programExited(); }

protected:
    void rcvProgramOutput(const QByteArray& roData);
    void programExited();

    // References to the real objects(!)
    std::unique_ptr<QProcess> _poExternProgram; // Process object running the watched program
    QString _oProgramName; // Program name to be run
    QString _oProgramPath; // Program path being added to the program name
    QStringList _oParameters; // List of program parameters
    QString _oParamDelimiter; // delimiter between the single parameters
    bool _bRcvStdErr; // 'true': Receive program output from stderr
};

#endif // EXTERN_PROGRAM
