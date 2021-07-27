#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class GraphQlsubscriberPool;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private:

public slots:
    void on_subscribe_pressed();
    void on_unsubscribe_pressed();
    void on_clear_pressed();
/*
    void signSubscriptionRemoved(int id);
    //слот срабатывает по запуску подписки
    void signSubscriptionStarted(int id);
    //сигнал передает данные от подписки с идентификатором
    void signSubscriptionData(int id, const QByteArray &ba);
    //сигнал передает сводную информацию о работе классов подписок
    void signSubscriptionMessage(int id, const QString &message);
    //сигнал принимает возвращаемые данные при инициализации подписки
    void signSubscriptionPayload(int id, const QString &payload);
    //сигнал срабатывает, когда подписка готова к работе
    void signSubscriptionReady(int id);
    //сигнал срабатывает, когда подписка закрыта (можно сработать внезапно)
    void signSubscriptionClose(int id);
*/
    void slotData(int, QByteArray ba);
    void slotDebugMessage(int, QString message);
    void slotOnGetPayload(int, QString payload);
    void slotOnConnectionReady(int);
    void slotOnConnectionClose(int);
private:
    Ui::MainWindow      *ui;
    GraphQlsubscriberPool   *m_sub_pool;
signals:
    void signStatus(const QString& status);
};
#endif // MAINWINDOW_H
