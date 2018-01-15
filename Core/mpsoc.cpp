#include "mpsoc.h"
#include <QVector>

namespace Core {

MPSoC::MPSoC(int h, int w, QPoint master, QObject *parent)
    : QObject(parent)
    , width(w)
    , height(h)
    , master(master)
    , processors(w, QVector<Processor *>(h))
{
    qDebug("Construindo nova MPSoC %dx%d", width, height);
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            if (i == master.x() && j == master.y()) {
                processors[i][j] = new Processor(i,j, Processor::Master);
            } else {
                processors[i][j] = new Processor(i,j);
            }
            connect(processors[i][j], SIGNAL(changed()), this, SLOT(update()));
            if (i > 0) { // If isn't 1st col, set west channel form west core
                processors[i][j]->setChannel(In, West, processors[i-1][j]->getChannel(Out, East));
                processors[i][j]->setChannel(Out, West, processors[i-1][j]->getChannel(In, East));
            }
            if (j > 0) { // If isn't 1st line, set noth channel form north core
                processors[i][j]->setChannel(In, North, processors[i][j-1]->getChannel(Out, South));
                processors[i][j]->setChannel(Out, North, processors[i][j-1]->getChannel(In, South));
            }
            if (i < w - 1) { // If isn't the last col, set new channel to the east
                processors[i][j]->setChannel(Out, East, new Channel(i,j,i+1,j));
                processors[i][j]->setChannel(In, East, new Channel(i+1,j,i,j));
                connect(processors[i][j]->getChannel(In, East), SIGNAL(loadChanged(int)), this, SLOT(update()));
                connect(processors[i][j]->getChannel(Out, East), SIGNAL(loadChanged(int)), this, SLOT(update()));
            }
            if (j < h - 1) { // If isn't the last line, set new channel to the south
                processors[i][j]->setChannel(Out, South, new Channel(i,j,i,j+1));
                processors[i][j]->setChannel(In, South, new Channel(i,j+1,i,j));
                connect(processors[i][j]->getChannel(In, South), SIGNAL(loadChanged(int)), this, SLOT(update()));
                connect(processors[i][j]->getChannel(Out, South), SIGNAL(loadChanged(int)), this, SLOT(update()));
            }
        }
    }
}

MPSoC::MPSoC(int w, int h, int masterX, int masterY, QObject *parent)
    : MPSoC(h,w,QPoint(masterX,masterY),parent)
{

}

MPSoC::~MPSoC()
{
    qDebug("MPSoC %dx%d sendo destruida", width, height);
}

Processor *MPSoC::getCore(const QPoint &p) const {
    return getCore(p.x(), p.y());
}

Processor* MPSoC::getCore(int x, int y) const {
    if (x >= width || y >= height || x < 0 || y < 0) {
        return 0;
    }
    return processors[x][y];
}

int MPSoC::getHeight() const {
    return height;
}

int MPSoC::getWidth() const {
    return width;
}

QSize MPSoC::getSize() const {
    return QSize(width, height);
}

QVector<Channel *> MPSoC::getPath(const QPoint &a, const QPoint &b) const {
    return getPath(a.x(), a.y(), b.x(), b.y());
}

QVector<Channel *> MPSoC::getPath(int x1, int y1, int x2, int y2) const {
    if (x1 > width || y1 > height || x2 > width || y2 > height) {
        return QVector<Channel *>();    // If out of bound return empety list
    }
    QVector<Channel *> path;

    int math = x1 < x2 ? +1 : -1;
    Direction dir = x1 < x2 ? East : West ;
    while (x1 != x2) {
        path.append(processors[x1][y1]->getChannel(Out, dir));
        x1 += math;
    }
    math = y1 < y2 ? +1 : -1;
    dir = y1 < y2 ? South : North;
    while (y1 != y2) {
        path.append(processors[x1][y1]->getChannel(Out, dir));
        y1 += math;
    }

    return path;
}

QVector<Processor *> MPSoC::getFree() const {
    QVector<Processor *> list = QVector<Processor *>();

    for (int i = 0; i < width; i ++) {
        for (int j = 0; j < height; j++) {
            Processor * p = processors[i][j];
            for (int k = 0; k < p->nOfThreads(); k++) {
                if (p->isIdle(k)) {
                    list.append(p); // If at least one thread is free add it into return list
                    continue;
                }
            }
        }
    }

    return list;
}

QVector<Processor *> MPSoC::getBusy() const {
    QVector<Processor *> list = QVector<Processor *>();

    for (int i = 0; i < width; i ++) {
        for (int j = 0; j < height; j++) {
            Processor * p = processors[i][j];
            list.append(p);
            for (int k = 0; k < p->nOfThreads(); k++) {
                if (p->isIdle(k)) {
                    list.removeLast(); // If at least one thread is free doesn't put it into return list
                    continue;
                }
            }
        }
    }

    return list;
}

AppNode *MPSoC::popProcess() {
    AppNode *node = 0;
    do {
        if (processList.isEmpty()) {
            return 0;
        }
        node = processList.first();
        processList.removeFirst();
    } while (node == 0 || node->isDone());    // Auto removes: null and killed nodes
    return node;
}

void MPSoC::pushProcess(QVector<AppNode *> list) {
    for (AppNode *node : list) {
        connect(node, SIGNAL(destroyed(QObject*)), this, SLOT(removeKilledNode(AppNode*)));
        processList.append(node);
    }
}

void MPSoC::clearApps() {
    qDeleteAll(runningApps);
    runningApps.clear();
    processList.clear();
}

void MPSoC::run(Application *app) {
    runningApps.append(app->clone());
    processList += runningApps.last()->getAllNodes();
}

Application *MPSoC::getApp(int i) const {
    return runningApps.value(i);
}

Core::Heuristic *MPSoC::getHeuristic() const {
    return heuristic;
}

void MPSoC::setHeuristic(Core::Heuristic *value) {
    heuristic = value;
}

// Prototype, dynamic modify mpsoc
//void MPSoC::setWidth(int w) {
//    width = w;
//    // destroy or create new processors
//    emit changed();
//}
//
//void MPSoC::destroyPorcessor(int x, int y) {
//  emit changed();
//}

void MPSoC::update() {
}

} // namespace Core
