#include <MainWindow.h>

#include <QtWidgets>

MainWindow::MainWindow(const QString &name, const QDBusObjectPath &actionPath, const QDBusObjectPath &menuPath, const QDBusConnection& connection, QSharedPointer<GDBusConnection> gconnection)
    : m_menu_importer( name, menuPath, "", actionPath, connection, gconnection)
{
  m_refresh_connection = connect( &m_menu_importer, SIGNAL( MenuItemsChanged() ), this,
      SLOT( RefreshMenus() ) );
}

MainWindow::~MainWindow()
{
  disconnect( m_refresh_connection );
}

void MainWindow::RefreshMenus()
{
  menuBar()->clear();

  m_top_menu = m_menu_importer.GetQMenu();
  if( m_top_menu )
  {
    menuBar()->addActions( m_top_menu->actions() );
  }

  menuBar()->repaint();
}
