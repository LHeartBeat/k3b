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


#include "k3bdirview.h"
#include <k3bcore.h>

#include "rip/k3baudiocdview.h"
#include "k3bfileview.h"
#include "rip/k3bmovieview.h"
#include "k3bfiletreeview.h"
#include "cdinfo/k3bdiskinfodetector.h"
#include "cdinfo/k3bdiskinfoview.h"
#include <device/k3bdevicehandler.h>
#include <device/k3bdevice.h>
#include <device/k3bdevicemanager.h>

#include <unistd.h>
// QT-includes
#include <qdir.h>
#include <qlistview.h>
#include <qstring.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qpixmap.h>
#include <qstringlist.h>
#include <qdragobject.h>
#include <qstrlist.h>
#include <qheader.h>
#include <qsplitter.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qiconset.h>
#include <qvaluelist.h>
#include <qlabel.h>
#include <qwidgetstack.h>
#include <qscrollview.h>
#include <qpainter.h>

// KDE-includes
#include <kmimetype.h>
#include <kcursor.h>
#include <kfiledetailview.h>
#include <ktoolbar.h>
#include <kiconloader.h>
#include <kurl.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kio/file.h>
#include <kio/global.h>
#include <krun.h>
#include <kprocess.h>
#include <kio/job.h>
#include <kcombobox.h>
#include <kfiletreeview.h>
#include <kdialog.h>
#include <kmessagebox.h>
#include <kstdaction.h>
#include <kconfig.h>
#include <kaction.h>


class K3bNoViewView : public QWidget
{
public:
  K3bNoViewView( QWidget* parent )
    : QWidget( parent ) {
    setPaletteBackgroundColor( QColor(201, 208, 255) );
  }

protected:
  void paintEvent( QPaintEvent* ) {
    QPainter p( this );

    QPixmap pix(locate( "data", "k3b/pics/k3b_probing_cd.png" ));
    p.drawPixmap( 0, 0, pix );
    p.setPen( Qt::white );
    p.drawText( pix.width() + 10, pix.height() /3, i18n("K3b is trying to retrieve information about the inserted disk.") );
  }
};



class K3bDirView::Private
{
public:
  bool ejectRequested;
};



K3bDirView::K3bDirView(K3bFileTreeView* treeView, QWidget *parent, const char *name )
  : QVBox(parent, name), 
    m_fileTreeView(treeView),
    m_bViewDiskInfo(false), 
    m_lastDevice(0)
{
  d = new Private;

  m_diskInfoDetector = new K3bDiskInfoDetector( this );
  connect( m_diskInfoDetector, SIGNAL(diskInfoReady(const K3bCdDevice::DiskInfo&)),
	   k3bcore, SLOT(requestBusyFinish()) );
  connect( m_diskInfoDetector, SIGNAL(diskInfoReady(const K3bCdDevice::DiskInfo&)),
	   this, SLOT(slotDiskInfoReady(const K3bCdDevice::DiskInfo&)) );

  //  KToolBar* toolBar = new KToolBar( this, "dirviewtoolbar" );


  if( !m_fileTreeView ) {
    m_mainSplitter = new QSplitter( this );
    m_fileTreeView = new K3bFileTreeView( m_mainSplitter );
    m_viewStack    = new QWidgetStack( m_mainSplitter );
  }
  else {  
    m_viewStack    = new QWidgetStack( this );
    m_mainSplitter = 0;
  }

  m_fileTreeView->header()->hide();

  m_fileView     = new K3bFileView(m_viewStack, "fileView");
  m_cdView       = new K3bAudioCdView(m_viewStack, "cdview");
  m_movieView    = new K3bMovieView(m_viewStack, "movieview");
  m_infoView     = new K3bDiskInfoView(m_viewStack, "infoView");

  m_noViewView = new K3bNoViewView( m_viewStack );

  m_viewStack->raiseWidget( m_fileView );

  m_fileTreeView->addDefaultBranches();
  m_fileTreeView->addCdDeviceBranches( k3bcore->deviceManager() );

  if( m_mainSplitter ) {
    // split
    QValueList<int> sizes = m_mainSplitter->sizes();
    int all = sizes[0] + sizes[1];
    sizes[1] = all*2/3;
    sizes[0] = all - sizes[1];
    m_mainSplitter->setSizes( sizes );
  }


  m_actionCollection = new KActionCollection( this );

  // add some actions to the toolbar
  //  m_fileView->actionCollection()->action("up")->plug( toolBar );
//   KStdAction::home( this, SLOT(home()), m_actionCollection )->plug( toolBar );
//   KStdAction::redisplay( this, SLOT(reload()), m_actionCollection )->plug( toolBar );
//   toolBar->insertSeparator();

  // add a url combobox to the toolbar
//   (void)new QLabel( i18n("Location"), toolBar );
//   m_urlCombo = new KComboBox( true, toolBar );
//   m_urlCombo->setEditText( QDir::homeDirPath() );
//   m_urlCombo->setDuplicatesEnabled( false );


  m_devicePopupMenu = new KActionMenu( m_actionCollection, "device_popup_menu" );
  KAction* actionDiskInfo = new KAction( i18n("&Disk Info"), "info", 0, this, SLOT(slotShowDiskInfo()),
					 m_actionCollection, "disk_info");
  KAction* actionUnmount = new KAction( i18n("&Unmount"), "cdrom_unmount", 0, this, SLOT(slotUnmountDisk()),
					m_actionCollection, "disk_unmount");
  KAction* actionEject = new KAction( i18n("&Eject"), "", 0, this, SLOT(slotEjectDisk()),
					m_actionCollection, "disk_eject");
  KAction* actionUnlock = new KAction( i18n("Un&lock"), "", 0, this, SLOT(slotUnlockDevice()),
				       m_actionCollection, "unlock" );

  m_devicePopupMenu->insert( actionDiskInfo );
  m_devicePopupMenu->insert( new KActionSeparator( this ) );
  m_devicePopupMenu->insert( actionUnmount );
  m_devicePopupMenu->insert( actionEject );
  m_devicePopupMenu->insert( actionUnlock );


//   connect( m_urlCombo, SIGNAL(returnPressed(const QString&)), this, SLOT(slotDirActivated(const QString&)) );
//   connect( m_urlCombo, SIGNAL(activated(const QString&)), this, SLOT(slotDirActivated(const QString&)) );

  connect( m_fileTreeView, SIGNAL(urlExecuted(const KURL&)), 
	   this, SLOT(slotDirActivated(const KURL&)) );
  connect( m_fileTreeView, SIGNAL(deviceExecuted(K3bCdDevice::CdDevice*)), 
	   this, SLOT(slotDetectDiskInfo(K3bCdDevice::CdDevice*)) );
  connect( m_fileTreeView, SIGNAL(contextMenu(K3bCdDevice::CdDevice*, const QPoint&)),
	   this, SLOT(slotFileTreeContextMenu(K3bCdDevice::CdDevice*, const QPoint&)) );
  connect( m_fileTreeView, SIGNAL(mountFinished(K3bDeviceBranch*, const QString&)),
	   this, SLOT(slotMountFinished(K3bDeviceBranch*, const QString&)) );
  connect( m_fileTreeView, SIGNAL(unmountFinished(K3bDeviceBranch*, bool)),
	   this, SLOT(slotUnmountFinished(K3bDeviceBranch*, bool)) );
  connect( m_fileView, SIGNAL(urlEntered(const KURL&)), m_fileTreeView, SLOT(followUrl(const KURL&)) );
  connect( m_fileView, SIGNAL(urlEntered(const KURL&)), this, SLOT(slotUpdateURLCombo(const KURL&)) );
}

K3bDirView::~K3bDirView()
{
  delete d;
}


void K3bDirView::showUrl( const KURL& url )
{
  slotDirActivated( url );
}


void K3bDirView::showDevice( K3bCdDevice::CdDevice* dev )
{
  slotDetectDiskInfo( dev );
}


void K3bDirView::slotDetectDiskInfo( K3bCdDevice::CdDevice* dev )
{
  // to speed things up we first check if the media is already mounted
  QString mp = KIO::findDeviceMountPoint( dev->mountDevice() );
  if( !mp.isEmpty() ) {
    slotDirActivated( mp );
  }
  else {
    m_viewStack->raiseWidget( m_noViewView );
    m_fileTreeView->setSelectedDevice( dev );
    k3bcore->requestBusyInfo( i18n("Trying to fetch information about the inserted disk.") );
    m_diskInfoDetector->detect( dev );
  }
}


void K3bDirView::slotDiskInfoReady( const K3bDiskInfo& info )
{
  if( m_bViewDiskInfo ||info.empty || info.noDisk ) {
    // show cd info
    m_viewStack->raiseWidget( m_infoView );
    m_infoView->displayInfo( info );
    m_bViewDiskInfo = false;
  }
  else if( info.tocType == K3bDiskInfo::DVD  ) {
    if( info.isVideoDvd ) {
      m_movieView->setDevice( info.device );
      m_viewStack->raiseWidget( m_movieView );
      m_movieView->reload();
    }
    else
      slotMountDevice( info.device );
  }
  else if( info.tocType == K3bDiskInfo::DATA  ) {
    slotMountDevice( info.device );
  }
  else {
    // check for MIXED_MODE and ask
    bool mount = false;
    if( info.tocType == K3bDiskInfo::MIXED  ) {
      mount = ( KMessageBox::questionYesNo( this,
					    i18n("Found Mixed-mode CD. Do you want K3b to mount the data part or show all the tracks?"),
					    i18n("Mixed-mode CD"),
					    i18n("Mount CD"),
					    i18n("Show tracks") ) == KMessageBox::Yes );
    }

    if( mount )
      slotMountDevice( info.device );
    else {
      m_viewStack->raiseWidget( m_cdView );
      m_cdView->setDisk( info );
    }
  }
}

void K3bDirView::slotMountDevice( K3bCdDevice::CdDevice* device )
{
  m_lastDevice = device;

  m_fileTreeView->branch( device )->mount();
}

void K3bDirView::slotMountFinished( K3bDeviceBranch*, const QString& mp )
{
  if( !mp.isEmpty() ) {
    slotDirActivated( mp );
    reload(); // HACK to get the contents shown... FIXME
  }
}

void K3bDirView::slotFileTreeContextMenu( K3bCdDevice::CdDevice* dev, const QPoint& p )
{
  m_lastDevice = dev;
  m_devicePopupMenu->popup( p );
}


void K3bDirView::slotShowDiskInfo()
{
  if( m_lastDevice ) {
    m_bViewDiskInfo = true;
    slotDetectDiskInfo( m_lastDevice );
  }
}


void K3bDirView::slotUnlockDevice()
{
  if( m_lastDevice )
    K3bCdDevice::unblock( m_lastDevice );
}


void K3bDirView::slotUnmountDisk()
{
  k3bcore->requestBusyInfo( i18n("Unmounting disk.") );
  K3bDeviceBranch* branch = m_fileTreeView->branch( m_lastDevice );
  if( branch ) {
    m_fileView->setAutoUpdate( false ); // in case we look at the mounted path
    d->ejectRequested = false;
    branch->unmount();
  }
  else {
    KMessageBox::sorry( this, i18n("No device selected.") );
  }
}

void K3bDirView::slotUnmountFinished( K3bDeviceBranch*, bool success )
{
  k3bcore->requestBusyFinish();

  if( success ) {
    m_fileView->setAutoUpdate( true );

    if( d->ejectRequested )
      K3bCdDevice::eject( m_lastDevice );
  }
}

void K3bDirView::slotEjectDisk()
{
  k3bcore->requestBusyInfo( i18n("Ejecting disk.") );
  K3bDeviceBranch *branch = m_fileTreeView->branch( m_lastDevice );
  if( branch ) {
    m_fileView->setAutoUpdate( false ); // in case we look at the mounted path
    d->ejectRequested = true;
    branch->unmount();
  }
  else {
    KMessageBox::sorry( this, i18n("No device selected.") );
  }
}


// void K3bDirView::slotEjectFinished()
// {
//   K3bDeviceBranch *branch = m_fileTreeView->branch( m_lastDevice );
//   if( branch ) {
//     branch->setAutoUpdate(true);
    
//     K3bCdDevice::eject( branch->device() );
//   }
//   else {
//     KMessageBox::sorry( this, i18n("No device selected.") );
//   }
// }

void K3bDirView::slotUpdateURLCombo( const KURL& )
{
//   m_urlCombo->setEditText( url.path() );
}


void K3bDirView::slotDirActivated( const QString& url )
{
//   m_urlCombo->insertItem( url, 0 );
  slotDirActivated( KURL(url) );
}


void K3bDirView::slotDirActivated( const KURL& url )
{
  // cancel any previous disk info retrieval
//   m_diskInfoDetector->cancel();
  k3bcore->requestBusyFinish();

  m_fileView->setUrl(url, true);
//   m_urlCombo->setEditText( url.path() );

  m_viewStack->raiseWidget( m_fileView );
}


void K3bDirView::reload()
{
  K3bCdContentsView* v = (K3bCdContentsView*)m_viewStack->visibleWidget();

  v->reload();
}


void K3bDirView::home()
{
  slotDirActivated( QDir::homeDirPath() );
}


void K3bDirView::saveConfig( KConfig* c )
{
  m_fileView->saveConfig(c);
}


#include "k3bdirview.moc"
