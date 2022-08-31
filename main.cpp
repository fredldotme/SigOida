#include <QCoreApplication>
#include <QDebug>
#include <QSet>
#include <QWebSocket>
#include <QWebSocketServer>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QSet<QWebSocket*> connections;

    QWebSocketServer server("sigoida", QWebSocketServer::NonSecureMode);

    QObject::connect(&server, &QWebSocketServer::newConnection,
                     &server, [&server, &connections](){
        while(server.hasPendingConnections()) {
            QWebSocket* client = server.nextPendingConnection();
            qDebug() << "New connection:" << client;

            // Post-disconnect cleanup
            QObject::connect(client, &QWebSocket::disconnected,
                             &server, [client, &connections] {
                connections.remove(client);
                client->deleteLater();
                qDebug() << "Client disconnected:" << client;
            });

            // Text message broadcasting
            QObject::connect(client, &QWebSocket::textMessageReceived,
                             &server, [&connections, client](const QString& msg) {
                for (QWebSocket* broadcastTarget : connections) {
                    if (broadcastTarget == client)
                        continue;

                    broadcastTarget->sendTextMessage(msg);
                    broadcastTarget->flush();
                }
                qDebug() << "Broadcasted message:" << msg;
            });

            // Accounting
            connections.insert(client);
        }
    });

    server.listen(QHostAddress("0.0.0.0"), 8089);
    return a.exec();
}
