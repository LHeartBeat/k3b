/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BDATAITEM_H
#define K3BDATAITEM_H


class K3bDirItem;
class K3bDataDoc;

#include <qstring.h>

#include <kio/global.h>

/**
  *@author Sebastian Trueg
  */

class K3bDataItem
{
 public: 
  K3bDataItem( K3bDataDoc* doc, K3bDataItem* parent = 0 );
  virtual ~K3bDataItem();
	
  K3bDirItem* parent() const { return m_parentDir; }
	
  K3bDataDoc* doc() const { return m_doc; }
  virtual const QString& k3bName();
  const QString& jolietName();
  virtual void setK3bName( const QString& );
  void setJolietName( const QString& );
  /** 
   * returns the path as defined by the k3b-hierachy, NOT starting with a slash
   * (since this is used for graft-points!) 
   * directories have a trailing "/"
   */
  virtual QString k3bPath();
  virtual QString jolietPath();

  virtual K3bDataItem* nextSibling();
	
  /** returns the path to the file on the local filesystem */
  virtual QString localPath() = 0;
  virtual int localInode() const { return 0; }
		
  virtual KIO::filesize_t k3bSize() const { return 0; }

  /** returnes the dir of the item (or the item if it's a dir) */
  virtual K3bDirItem* getDirItem() = 0;

  virtual void reparent( K3bDirItem* );

  virtual bool isDir() const { return false; }
  virtual bool isFile() const { return false; }
  virtual bool isSpecialFile() const { return false; }
  virtual bool isSymLink() const { return false; }	

  bool hideOnRockRidge() const;
  bool hideOnJoliet() const;

  virtual void setHideOnRockRidge( bool b );
  virtual void setHideOnJoliet( bool b );

  virtual int depth() const;

  virtual bool isValid() const { return true; }

  // these are all needed for special fileitems like
  // imported sessions or the movix filesystem
  virtual bool isRemoveable() const { return m_bRemoveable; }
  virtual bool isMoveable() const { return m_bMovable; }
  virtual bool isRenameable() const { return m_bRenameable; }
  virtual bool isHideable() const { return m_bHideable; }
  virtual bool writeToCd() const { return m_bWriteToCd; }
  virtual const QString& extraInfo() const { return m_extraInfo; }

  void setRenameable( bool b ) { m_bRenameable = b; }
  void setMoveable( bool b ) { m_bMovable = b; }
  void setRemoveable( bool b ) { m_bRemoveable = b; }
  void setHideable( bool b ) { m_bHideable = b; }
  void setWriteToCd( bool b ) { m_bWriteToCd = b; }
  void setExtraInfo( const QString& i ) { m_extraInfo = i; }

 protected:
  QString m_k3bName;
  /**
   * this is used if Joliet extensions are enabled for cutting the filenames
   */
  QString m_jolietName;

 private:
  K3bDataDoc* m_doc;
  K3bDirItem* m_parentDir;

  bool m_bHideOnRockRidge;
  bool m_bHideOnJoliet;
  bool m_bRemoveable;
  bool m_bRenameable;
  bool m_bMovable;
  bool m_bHideable;
  bool m_bWriteToCd;
  QString m_extraInfo;
};

#endif
