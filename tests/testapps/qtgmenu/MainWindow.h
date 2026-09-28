#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <libqtgmenu/QtGMenuImporter.h>
#include <gio/gio.h>

class MainWindow : public QMainWindow
{
Q_OBJECT

public:
  MainWindow(const QString &name, const QDBusObjectPath &actionPath, const QDBusObjectPath &menuPath, const QDBusConnection& connection, QSharedPointer<GDBusConnection> gconnection);

  ~MainWindow();

private Q_SLOTS:
  void RefreshMenus();

private:
  qtgmenu::QtGMenuImporter m_menu_importer;
  std::shared_ptr< QMenu > m_top_menu = nullptr;
  QMetaObject::Connection m_refresh_connection;
};

#endif
