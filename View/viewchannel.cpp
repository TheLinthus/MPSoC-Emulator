#include "viewchannel.h"

namespace View {

Channel::Channel(int x, int y, bool v, Core::Channel *c)
    : x((v ? 10 : 102) + (x * 200))
    , y((v ? 102 : 10) + (y * 200))
    , w(v ? 25 : 96)
    , h(v ? 96 : 25)
    , vertical(v)
    , over(false)
    , channel(c)
{
    setAcceptHoverEvents(true);
    connect(channel, SIGNAL(loadChanged(int)), this, SLOT(change()));

    updateToolTip();
}

QRectF Channel::boundingRect() const {
    return QRectF(x,y,w,h);
}

void Channel::hoverEnterEvent(QGraphicsSceneHoverEvent *) {
    if (hoverEffect != 0) {
        hoverEffect = new QGraphicsDropShadowEffect();
        hoverEffect->setBlurRadius(25);
        hoverEffect->setColor(QColor(Qt::white));
        hoverEffect->setOffset(0,0);
        setGraphicsEffect(hoverEffect);
    }
    over = true;
}

void Channel::hoverLeaveEvent(QGraphicsSceneHoverEvent *) {
    over = false;
    hoverEffect->deleteLater();
}

void Channel::mousePressEvent(QGraphicsSceneMouseEvent *) {

}

void Channel::updateToolTip() {
    setToolTip(QString("Channel load: %1<hr>From A to B: %2<br>From B to A: %3").arg(channel->val()).arg(channel->valAtB()).arg(channel->valBtA()));
}

void Channel::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    QRectF rect = boundingRect();

    QPainterPath path;
    path.addRect(rect);

    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(QColor(255,255,255,127)));

    if (over) {
        if (vertical) {
            painter->drawRect(rect.adjusted(-2,0,2,0));
        } else {
            painter->drawRect(rect.adjusted(0,-2,0,2));
        }
    }

    int g, b;

    if (channel->val() >= 100) {
        g = b = 0;
    } else if (channel->val() <= 50) {
        g = 255;
        b = (50 - channel->val()) * 5.1;
    } else {
        g = 255 - (channel->val() - 50) * 5.1;
        b = 0;
    }
    painter->setBrush(QBrush(QColor(255,g,b))); // Do branco ao amarelo e ao vermelho, percentual da carga no canal
    painter->drawPath(path);
}

void Channel::change() {
    this->update();
    updateToolTip();
}

} // namespace View
