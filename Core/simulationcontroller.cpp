#include "simulationcontroller.h"
#include "singleton.h"

namespace Core {

SimulationController::SimulationController(QObject *parent) :
    QObject(parent),
    timer(this),
    pointer(-1),
    started(false),
    processingMPSoCs(0),
    error(false)
{
    timer.setInterval(500);
    connect(&worker, SIGNAL(processed(int)), this, SLOT(processingDone(int)));
    connect(&worker, SIGNAL(failed(int)), this, SLOT(fail(int)));
    connect(&worker, SIGNAL(finished()), this, SLOT(workerFinished()));
    connect(&timer, SIGNAL(timeout()), this, SLOT(autoStep()));
    connect(mpsocs, SIGNAL(mpsocDestroyed(QObject*)), this, SLOT(reset()));
}

SimulationController *SimulationController::createInstance() {
    return new SimulationController();
}

void SimulationController::autoStep() {
    if (!worker.isRunning()) {
        if (isStepEnable()) {
            this->stepFoward();
        } else {
            qWarning() << "Can't step now";
        }
    } else {
        qWarning() << "Step skiped, processing is to slow for this tick rate.";
    }
}

SimulationController::~SimulationController() {

}

SimulationController *SimulationController::instance() {
    return Singleton<Core::SimulationController>::instance(Core::SimulationController::createInstance);
}

bool SimulationController::isStarted() {
    return started;
}

bool SimulationController::isRunning() {
    return timer.isActive();
}

bool SimulationController::isRunEnabled() {
    if (!error && (steps.value(pointer, -1) == 0 || !started)) {
        return true;
    } else {
        return false;
    }
}

bool SimulationController::isKillEnabled() {
    if (!error && (steps.value(pointer, -1) == 0)) {
        return true;
    } else {
        return false;
    }
}

bool SimulationController::isStepEnable() {
    return !error && !worker.isRunning() && mpsocs->count() > 0;
}

int SimulationController::currentStep() {
    return pointer;
}

int SimulationController::stepsCount() {
    return steps.count();
}

void SimulationController::setInterval(int ms) {
    timer.setInterval(ms);
}

void SimulationController::setAutoStep(bool enabled) {
    if (enabled) {
        timer.start();
    } else {
        timer.stop();
    }
    emit notify();
}

void SimulationController::reset() {
    steps.clear();
    started = false;
    error = false;
    pointer = -1;
    processingMPSoCs = 0;
    timer.stop();
    apps->clearRunning();
    for (int i = 0; i < mpsocs->count(); i++) {
        mpsocs->get(i)->clearApps();
    }
    emit notify();
    emit reseted();
}

void SimulationController::newStep() {
    if (!started) {
        started = true;
        emit start();
    }

    pointer++;
    steps.append(0);
}

void SimulationController::workerFinished() {
    emit step();
}

void SimulationController::processingDone(int index) {
    processingMPSoCs--;
    if (processingMPSoCs == 0) {
        emit notify();
    }
}

void SimulationController::stepFoward() {
    if (started && pointer + 1 < steps.count()) {
        pointer++;
        // TODO - Redo step from history
    } else {
        newStep();
    }
    processingMPSoCs = mpsocs->count();
    worker.start();
}

void SimulationController::stepBackward() {
    if (pointer > 0) {
        pointer--;
        // TODO - Undo step from history
    }
    emit notify();
}

void SimulationController::runAction(Application *app) {
    for (int i = 0; i < mpsocs->count(); i++) {
       mpsocs->get(i)->run(app);
    }
    emit notify();
//    TODO
//    if (pointer + 1 == steps.size()) {
//        newStep();
//    }
//    if (steps.value(pointer, -1) == 0) {
//        steps[pointer] = 1;
//        for (int i = 0; i < mpsocs->count(); i++) {
//            mpsocs->get(i)->run(app);
//        }
//        emit notify();
//    }
//    processingMPSoCs = mpsocs->count();
//    worker.start();
}

void SimulationController::killAction(Application *app) {
//    TODO
//    if (steps.value(pointer, -1) == 0) {
//        steps[pointer] = 2;
//        emit notify();
//    }
}

void SimulationController::fail(int e){
    if (e == 0) /*if (e.fatal())*/ {
        qWarning() << "Simulation Fatal Error";
        error = true;
        timer.stop();
    }
    if (e == 1) {
        processingMPSoCs = 0;
    }
    emit failed(e);
    emit notify();
}

void SimulationWorker::run() {
    Core::Heuristic *heuristic;
    Core::MPSoC *mpsoc;
    for (int i = 0; i < mpsocs->count(); i++) {
        mpsoc = mpsocs->get(i);
        if (mpsoc->getFree().isEmpty()) {
            emit failed(7); // MPSoC Full
            emit processed(i);
            continue;
        }
        Core::AppNode *node = mpsoc->popProcess();
        if (node == 0) {
            emit failed(1); // Nothing to sim
            emit processed(i);
            continue;
        }
        Core::Application *app = qobject_cast<Core::Application *>(node->parent());

        heuristic = mpsoc->getHeuristic();

        QPoint point;
        int thread = 0;

        QScriptValueList args;

        args << ScriptConverter::toScriptValue(heuristic->getEngine(), mpsoc);
        args << node->getN();
        args << ScriptConverter::toScriptValue(heuristic->getEngine(), app);
        int result = heuristic->selectCore(point, args); // Thread number not considered yet (future version)
        if (result != 1) {
            emit failed(result);
        } else {
            if (point.x() >= mpsoc->getWidth() || point.y() >= mpsoc->getHeight()) {
                emit failed(5); // Selection out of index
                heuristic->log(QString("selection out of mpsoc index range, mpsoc size = %1x%2, selection was %3x%4")
                               .arg(mpsoc->getWidth()).arg(mpsoc->getHeight())
                               .arg(point.x()).arg(point.y()));
                return;
            }
            if (mpsoc->getCore(point.x(),point.y())->run(node, thread)) {
                connect(this, SIGNAL(started()), node, SLOT(tick()));
                QMap<int, AppLoad *> *loads = app->getConnectionsFrom(node->getN());
                for (int key : loads->keys()) {
                    AppNode *bNode = app->getNode(key);
                    if (bNode->isRunning()) {
                        QVector<Channel *> path = mpsoc->getPath(node->getInCoreX(), node->getInCoreY(),
                                                                  bNode->getInCoreX(), bNode->getInCoreY());
                        AppLoad *load = loads->value(key);
                        connect(node, SIGNAL(finished(AppNode*)), load, SLOT(remove()));
                        connect(bNode, SIGNAL(finished(AppNode*)), load, SLOT(remove()));
                        for (Channel *channel : path) {
                            channel->add(load);
                        }
                        if (load->getVolume() > node->getLifespan() + node->getCycles()) // Ensures that the node doesn't die before volume
                            node->setLifespan(load->getVolume() + node->getCycles());
                        if (load->getVolume() > bNode->getLifespan() + bNode->getCycles())
                            node->setLifespan(load->getVolume() + bNode->getCycles());
                        path = mpsoc->getPath(bNode->getInCoreX(), bNode->getInCoreY(),
                                               node->getInCoreX(), node->getInCoreY());
                        load = app->getConnectionsFrom(key)->value(node->getN());
                        connect(node, SIGNAL(finished(AppNode*)), load, SLOT(remove()));
                        connect(bNode, SIGNAL(finished(AppNode*)), load, SLOT(remove()));
                        for (Channel *channel : path) {
                            channel->add(load);
                        }
                        if (load->getVolume() > node->getLifespan() + node->getCycles()) // Ensures that the node doesn't die before volume
                            node->setLifespan(load->getVolume() + node->getCycles());
                        if (load->getVolume() > bNode->getLifespan() + bNode->getCycles())
                            node->setLifespan(load->getVolume() + bNode->getCycles());
                    }
                }
            } else {
                emit failed(0); // Core not empty
                heuristic->log(QString("failed to alocate to Core(%1,%2) Thread %3: Core is not empty").arg(point.x()).arg(point.y()).arg(thread));
                return;
            }
        }

        emit processed(i);
    }
}

} // namespace Core
