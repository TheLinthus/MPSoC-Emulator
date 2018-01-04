#include "mpsocbox.h"
#include "ui_mpsocbox.h"

namespace View {

MPSoCBox::MPSoCBox(QWidget *parent) :
    QWidget(parent)
    , gridWidth(1)
    , gridHeight(1)
    , mpsoc(0)
    , heuristicName(QString("Null"))
    , ui(new Ui::MPSoCBox)
    , max(new QtCharts::QLineSeries())
    , average(new QtCharts::QLineSeries())
    , axisX(new QtCharts::QValueAxis())
    , axisY(new QtCharts::QValueAxis())
    , chart(new QtCharts::QChart())
    , stepCount(0)
{
    ui->setupUi(this);
    mpsocScene = new QGraphicsScene(this);
    ui->mpsocGV->setScene(mpsocScene);
    ui->mpsocGV->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);

    QPen pen;
    pen.setWidth(3);
    pen.setColor(Qt::red);
    max->setPen(pen);
    pen.setColor(Qt::cyan);
    average->setPen(pen);

    max->append(0,0);
    max->setName(tr("Max."));
    average->append(0,0);
    average->setName(tr("Avrg."));

    axisY->setRange(0,120);
    axisY->setTickCount(7);
    axisX->setRange(0,1);
    axisX->setTickCount(2);
    axisX->setLabelFormat("%d");

    chart->setTitle(tr("Channel Allocation"));
    chart->addSeries(max);
    chart->addSeries(average);
    chart->setAxisY(axisY, max);
    chart->setAxisX(axisX, max);
    chart->setAxisY(axisY, average);
    chart->setAxisX(axisX, average);
    chart->setMargins(QMargins(5,5,15,5));
    chart->legend()->setScale(0.8);

    QtCharts::QChartView *chartView = new QtCharts::QChartView(chart);
    chartView->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);

    this->layout()->addWidget(chartView);
}

MPSoCBox::~MPSoCBox() {
    delete ui;
}

} // namespace View

void View::MPSoCBox::on_closeButton_clicked() {
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, QWidget::window()->windowTitle(), "Are you sure you want to remove this MPSoC?\nIt will resut in reseting all active simulations.",
                                QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        this->deleteLater();
        mpsocs->remove(mpsoc);
    }
}

Core::MPSoC *View::MPSoCBox::getMpsoc() const {
    return mpsoc;
}

int View::MPSoCBox::getGridWidth() const {
    return gridWidth;
}

void View::MPSoCBox::setMPSoC(Core::MPSoC *value) {
    if (mpsoc != 0) {
        disconnect(mpsoc, SIGNAL(changed()), this, SLOT(drawnMPSoC()));
        disconnect(mpsoc, SIGNAL(destroyed(QObject*)), this, SLOT(deleteLater()));
    }
    mpsoc = value;
    gridWidth = mpsoc->getWidth();
    gridHeight = mpsoc->getHeight();

    connect(mpsoc, SIGNAL(changed()), this, SLOT(drawnMPSoC()));
    connect(mpsoc, SIGNAL(destroyed(QObject*)), this, SLOT(deleteLater()));

    heuristicName = mpsoc->getHeuristic()->getName();

    ui->titleLabel->setText(title.arg(QString::number(gridWidth), QString::number(gridHeight), heuristicName));

    drawnMPSoC();
}

// Move this to a thread
void View::MPSoCBox::updateChart() {
    qreal maxLoad = 0;
    qreal averageLoad = 0;
    qreal temp;
    stepCount++;
    int count = 0;
    for (int i = 0; i < mpsoc->getWidth(); i++) {
        for (int j = 0; j < mpsoc->getHeight(); j++) {
            if (mpsoc->getCore(i,j)->getChannel(Core::DataDirection::In, Core::Direction::South) != 0) {
                temp = mpsoc->getCore(i,j)->getChannel(Core::DataDirection::In, Core::Direction::South)->val();
                maxLoad = qMax(temp, maxLoad);
                averageLoad += temp;
                count++;
                temp = mpsoc->getCore(i,j)->getChannel(Core::DataDirection::Out, Core::Direction::South)->val();
                maxLoad = qMax(temp, maxLoad);
                averageLoad += temp;
                count++;
            }
            if (mpsoc->getCore(i,j)->getChannel(Core::DataDirection::In, Core::Direction::East) != 0) {
                temp = mpsoc->getCore(i,j)->getChannel(Core::DataDirection::In, Core::Direction::East)->val();
                maxLoad = qMax(temp, maxLoad);
                averageLoad += temp;
                count++;
                temp = mpsoc->getCore(i,j)->getChannel(Core::DataDirection::Out, Core::Direction::East)->val();
                maxLoad = qMax(temp, maxLoad);
                averageLoad += temp;
                count++;
            }
        }
    }
    averageLoad /= count;

    axisX->setRange(0, stepCount + 1);
    if (stepCount < 5)
        axisX->setTickCount(stepCount + 2);
    else
        axisX->setTickCount(5);

    max->append(stepCount, qMin(maxLoad, 120.0));
    average->append(stepCount, qMin(averageLoad, 120.0));
}

void View::MPSoCBox::clearChart() {
    stepCount = 0;
    max->clear();
    average->clear();
    axisY->setRange(0,120);
    axisY->setTickCount(7);
    axisX->setRange(0,1);
    axisX->setTickCount(2);
}

void View::MPSoCBox::drawnMPSoC() {
    mpsocScene->clear();

    mpsocScene->setSceneRect(-50,-50,gridWidth*200,gridHeight*200);

    mpsocScene->addRect(-50,-50,gridWidth*200,gridHeight*200,QPen(QColor(32,32,32),5), QBrush(QColor(64,64,64)));

    // Desenha MPSoC na scene;
    for (int i = 0; i < gridWidth; i++) {
        for (int j = 0; j < gridHeight; j++) {
            Core::Processor * cp = mpsoc->getCore(i,j);
            View::Processor * vp = new View::Processor(i,j,cp);
            mpsocScene->addItem(vp);
            if (Core::Channel * cc = mpsoc->getCore(i,j)->getChannel(Core::In, Core::North)) {
                View::Channel * vc = new View::Channel(cc, true, true);
                mpsocScene->addItem(vc);
            }
            if (Core::Channel * cc = mpsoc->getCore(i,j)->getChannel(Core::In, Core::South)) {
                View::Channel * vc = new View::Channel(cc, true, false);
                mpsocScene->addItem(vc);
            }
            if (Core::Channel * cc = mpsoc->getCore(i,j)->getChannel(Core::In, Core::West)) {
                View::Channel * vc = new View::Channel(cc, false, true);
                mpsocScene->addItem(vc);
            }
            if (Core::Channel * cc = mpsoc->getCore(i,j)->getChannel(Core::In, Core::East)) {
                View::Channel * vc = new View::Channel(cc, false, false);
                mpsocScene->addItem(vc);
            }
        }
    }
}

void View::MPSoCBox::fitInView() {
    ui->mpsocGV->setMaximumHeight(ui->mpsocGV->width());
    ui->mpsocGV->fitInView(mpsocScene->sceneRect(),Qt::KeepAspectRatio);
}

void View::MPSoCBox::paintEvent(QPaintEvent *) {
    fitInView();
}

int View::MPSoCBox::getGridHeight() const {
    return gridHeight;
}
