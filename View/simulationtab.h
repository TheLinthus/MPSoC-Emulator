#ifndef VIEW_SIMULATIONTAB_H
#define VIEW_SIMULATIONTAB_H

#include <QDebug>

#include <QFrame>
#include <QItemSelection>
#include <QStringListModel>
#include <View/newmpsocdialog.h>
#include <View/mpsocbox.h>
#include <Core/mpsoc.h>
#include <heuristiccontroller.h>
#include <mpsoccontroller.h>

namespace View {

namespace Ui {
class SimulationTab;
}

class SimulationTab : public QFrame
{
    Q_OBJECT

public:
    explicit SimulationTab(QWidget *parent = 0);
    ~SimulationTab();

public slots:
    void updateListViewModel(const QStringList &list);
    void updateView();
    void on_autoStepToggle(bool);

    void on_pushButtonRunApplication_clicked();
    void on_pushButtonKillApplication_clicked();
    void on_buttonAddMPSoC_clicked();
    void on_ButtonReset_clicked();
    void on_buttonPrevStep_clicked();
    void on_buttonNextStep_clicked();
    void on_buttonPauseTimer_clicked();
    void on_buttonPlayTimer_clicked();
    void on_spinBoxTimer_valueChanged(int value);
    void on_listViewApplications_selectionModel_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void on_listViewRunning_selectionModel_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

private slots:
    void on_sliderStep_valueChanged(int value);

private:
    Ui::SimulationTab *ui;

    QStringListModel *listModelApplications;
    QStringListModel *listModelRunning;
};


} // namespace View
#endif // VIEW_SIMULATIONTAB_H
