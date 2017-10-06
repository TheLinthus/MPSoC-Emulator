#ifndef VIEW_MPSOCBOX_H
#define VIEW_MPSOCBOX_H

#include <QWidget>
#include <QMessageBox>
#include <QtCore>
#include <QtGui>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>
#include <QtScript/QtScriptDepends>
#include <QDebug>
#include <View/viewprocessor.h>
#include <View/viewchannel.h>
#include <Core/mpsoc.h>

#include <QDebug>

namespace View {

namespace Ui {
class MPSoCBox;
}

class MPSoCBox : public QWidget
{
    Q_OBJECT

public:
    explicit MPSoCBox(QWidget *parent = 0);
    ~MPSoCBox();

    int getGridHeight() const;
    int getGridWidth() const;
    Core::MPSoC *getMpsoc() const;

public slots:
    void setMPSoC(Core::MPSoC *value);
    void setHeuristic(QString heuristic);

private slots:
    void on_closeButton_clicked();

protected:
    void resizeEvent(QResizeEvent *) override;
    void showEvent(QShowEvent *) override;

private:
    int gridWidth;
    int gridHeight;
    Core::MPSoC *mpsoc;
    QScriptEngine engine;

    QGraphicsScene *mpsocScene;
    Ui::MPSoCBox *ui;
};


} // namespace View
#endif // VIEW_MPSOCBOX_H
