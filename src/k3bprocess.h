/***************************************************************************
                          k3bprocess.h  -  
                   KProcess with enhanced stderr handling
                             -------------------
    begin                : Wed Sep  4 12:01:14 CEST 2002
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3B_PROCESS_H
#define K3B_PROCESS_H


#include <kprocess.h>
#include <qstring.h>

class K3bProcess : public KProcess
{
  Q_OBJECT

 public:
  K3bProcess();
  ~K3bProcess();

  bool start( RunMode run = NotifyOnExit, Communication com = NoCommunication );

  /** get stdin socket */
  int stdin() const;
  /** get stdout socket */
  int stdout() const;

  /** if set to true one needs to create a socketnotifier on one's own */
  void setRawStdin(bool b) { m_rawStdin = b; }
  /** if set to true K3bProcess emits stdoutReady instead of the KProcess signal
   *  and the data has to read by the user */
  void setRawStdout(bool b) { m_rawStdout = b; }

 public slots:
  void setSplitStdout( bool b ) { m_bSplitStdout = b; }

 private slots:
  void slotSplitStderr( KProcess*, char*, int );
  void slotSplitStdout( KProcess*, char*, int );

 signals:
  void stderrLine( const QString& line );
  void stdoutLine( const QString& line );
  /** gets emitted if raw stdout mode has been requested */
  void stdoutReady(int);

 protected:
  /**
   * reimplemeted from KProcess
   */
  int commSetupDoneP();

 private:
  void splitOutput( char*, int, bool );

  QString m_unfinishedStdoutLine;
  QString m_unfinishedStderrLine;
  bool m_bSplitStdout;

  bool m_rawStdin;
  bool m_rawStdout;
};


#endif
