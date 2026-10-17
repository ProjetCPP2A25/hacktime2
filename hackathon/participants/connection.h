#ifndef CONNECTION_H
#define CONNECTION_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>

class Connection
{
private:
    QSqlDatabase db;

public:
    Connection();
    bool createconnect();
    QString lastError() const;
    void closeConnection();
};

#endif // CONNECTION_H