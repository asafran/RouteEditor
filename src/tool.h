#ifndef TOOL_H
#define TOOL_H

#include <QWidget>
#include "undo-redo.h"

class Tool : public QWidget
{
    Q_OBJECT
public:
    explicit Tool(DatabaseManager *database, QWidget *parent = nullptr);
    virtual ~Tool();

    virtual void intersection(const FindNode&) = 0;

signals:
    void sendStatusText(const QString &message, int timeout);

protected:
    DatabaseManager *_database;
};

#endif // TOOL_H
