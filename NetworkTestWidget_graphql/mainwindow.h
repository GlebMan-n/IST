#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkReply>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class HttpClientQt;
class WebSocket;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    HttpClientQt* m_client;
    WebSocket* m_webSocket;
    QStringList m_list_queries;
    bool m_block_changes;
private slots:
    void on_execute_clicked();
    void update_query_combo_box();
    void update_query_list();
    void on_clear_clicked();
    void setResponseText(const QString &string);
    void on_data_sended(const QByteArray &data);
    void on_query_editTextChanged(const QString & text);
    void on_addQuery_pressed();
    void on_query_currentIndexChanged(int i);
    void on_del_pressed();

};
#endif // MAINWINDOW_H
