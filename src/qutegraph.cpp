/*
	Copyright (C) 2008, 2009 Andres Cabrera
	mantaraya36@gmail.com

	This file is part of CsoundQt.

	CsoundQt is free software; you can redistribute it
	and/or modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	CsoundQt is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with Csound; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
	02111-1307 USA
*/

#include "qutegraph.h"
#include "curve.h"
#include <cmath>
#include <QPalette>


#include <QColorDialog>


enum CsoundEngineStatus {
    Running=0,
    UserDataNotSet,
    CsoundInstanceNotSet,
    EngineNotSet,
    NotRunning
};


inline CsoundEngineStatus csoundEngineStatus(CsoundUserData *ud) {
    if(ud == nullptr) {
        return CsoundEngineStatus::UserDataNotSet;
    }
    if(ud->csound == nullptr) {
        return CsoundEngineStatus::CsoundInstanceNotSet;
    }
    if(ud->csEngine == nullptr) {
        return CsoundEngineStatus::EngineNotSet;
    }
    if(!ud->csEngine->isRunning()) {
        // qDebug() << "csound not running";
        return CsoundEngineStatus::NotRunning;
    }
    return CsoundEngineStatus::Running;
}


// -----------------------------------------------------------------------------------------

QuteGraph::QuteGraph(QWidget *parent) : QuteWidget(parent)
{
	m_widget = new StackedLayoutWidget(this);
	m_widget->show();
	//  m_widget->setAutoFillBackground(true);
	m_widget->setMouseTracking(true); // Necessary to pass mouse tracking to widget panel for _MouseX channels
	m_widget->setContextMenuPolicy(Qt::NoContextMenu);
    m_numticksY = 6;
	m_label = new QLabel(this);
	QPalette palette = m_widget->palette();
    palette.setColor(QPalette::WindowText, QColor(150, 150, 150));
	m_label->setPalette(palette);
	m_label->setText("");
    m_label->setFont(QFont({"Helvetica", 7}));
    m_label->move(120, -4);
	m_label->resize(500, 25);
	m_pageComboBox = new QComboBox(this);
    m_pageComboBox->setMinimumWidth(120);
    m_pageComboBox->setMaximumHeight(14);
    // m_pageComboBox->resize(140, 14);
    m_pageComboBox->setFont(QFont({"Sans", 7}));
    m_pageComboBox->setFocusPolicy(Qt::NoFocus);
    m_pageComboBox->setStyleSheet("QComboBox QAbstractItemView"
                                  "{ min-width: 160px; }");
    m_pageComboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
	m_label->setFocusPolicy(Qt::NoFocus);
    m_drawGrid = true;
    m_drawTableInfo = true;
    m_showScrollbars = true;
	canFocus(false);
	connect(m_pageComboBox, SIGNAL(currentIndexChanged(int)),
			this, SLOT(indexChanged(int)));
	polygons.clear();

	QPalette Pal(this->palette());
    // set black background
    Pal.setColor(QPalette::Background, Qt::black);
    this->setAutoFillBackground(true);
    this->setPalette(Pal);

	// Default properties
	setProperty("QCS_zoomx", 1.0);
	setProperty("QCS_zoomy", 1.0);
	setProperty("QCS_dispx", 1.0);
	setProperty("QCS_dispy", 1.0);
	setProperty("QCS_modex", "auto");
	setProperty("QCS_modey", "auto");
    setProperty("QCS_showSelector", true);
    setProperty("QCS_showGrid", true);
    setProperty("QCS_showTableInfo", true);
    setProperty("QCS_showScrollbars", true);
	setProperty("QCS_all", true);
}

QuteGraph::~QuteGraph()
{
}

QString QuteGraph::getWidgetLine()
{
	// Extension to MacCsound: type of graph (table, ftt, scope), value (which hold the index of the
	// table displayed) zoom and channel name
	// channel number is unused in QuteGraph, but selects channel for scope
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QString line = "ioGraph {" + QString::number(x()) + ", " + QString::number(y()) + "} ";
	line += "{"+ QString::number(width()) +", "+ QString::number(height()) +"} table ";
	line += QString::number(m_value, 'f', 6) + " ";
	line += QString::number(property("QCS_zoomx").toDouble(), 'f', 6) + " ";
	line += m_channel;
	//   qDebug("QuteGraph::getWidgetLine(): %s", line.toStdString().c_str());
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return line;
}

QString QuteGraph::getWidgetXmlText()
{
	// Graphs are not implemented in blue
	xmlText = "";
	QXmlStreamWriter s(&xmlText);
	createXmlWriter(s);
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif

	s.writeTextElement("value", QString::number((int)m_value));
	s.writeTextElement("objectName2", m_channel2);
	s.writeTextElement("zoomx", QString::number(property("QCS_zoomx").toDouble(), 'f', 8));
	s.writeTextElement("zoomy", QString::number(property("QCS_zoomy").toDouble(), 'f', 8));
	s.writeTextElement("dispx", QString::number(property("QCS_dispx").toDouble(), 'f', 8));
	s.writeTextElement("dispy", QString::number(property("QCS_dispy").toDouble(), 'f', 8));
	s.writeTextElement("modex", property("QCS_modex").toString());
	s.writeTextElement("modey", property("QCS_modey").toString());
    s.writeTextElement("showSelector",
                       property("QCS_showSelector").toBool() ? "true" : "false");
    s.writeTextElement("showGrid", property("QCS_showGrid").toBool() ? "true" : "false");
    s.writeTextElement("showTableInfo",
                       property("QCS_showTableInfo").toBool() ? "true" : "false");
    s.writeTextElement("showScrollbars",
                       property("QCS_showScrollbars").toBool() ? "true" : "false");

	s.writeTextElement("all", property("QCS_all").toBool() ? "true" : "false");
	s.writeEndElement();
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return xmlText;
}


QString QuteGraph::getWidgetType() {
    return QString("BSBGraph");
}

void QuteGraph::setWidgetGeometry(int x,int y,int width,int height)
{
	QuteWidget::setWidgetGeometry(x,y,width, height);
	static_cast<StackedLayoutWidget *>(m_widget)->setWidgetGeometry(0,0,width, height);
	int index = static_cast<StackedLayoutWidget *>(m_widget)->currentIndex();
    changeCurve(-2);

    if (index < 0)
		return;
    // changeCurve(index);
}

void QuteGraph::keyPressEvent(QKeyEvent *event) {
    bool flag;
    switch(event->key()) {
    case Qt::Key_S:
        flag = !property("QCS_showSelector").toBool();
        setProperty("QCS_showSelector", flag?"true":"false");
        if(flag)
            m_pageComboBox->show();
        else
            m_pageComboBox->hide();
        event->accept();
        break;
    case Qt::Key_G:
        flag = !property("QCS_showGrid").toBool();
        setProperty("QCS_showGrid", flag?"true":"false");
        m_drawGrid = flag;
        event->accept();
        break;
    case Qt::Key_C:
        flag = !property("QCS_showTableInfo").toBool();
        setProperty("QCS_showTableInfo", flag?"true":"false");
        m_drawTableInfo = flag;
        if(flag)
            m_label->show();
        else
            m_label->hide();
        event->accept();
        break;
    case Qt::Key_Plus:
        setProperty("QCS_zoomx", property("QCS_zoomx").toDouble()*2);
        applyInternalProperties();
        event->accept();
        break;
    case Qt::Key_Minus:
        setProperty("QCS_zoomx", qMax(1.0, property("QCS_zoomx").toDouble()*0.5));
        applyInternalProperties();
        event->accept();
        break;
    case Qt::Key_H:
        QMessageBox mb;
        mb.setText(tr("<span style=\"font-size : 12pt\">"
                      "<ul>"
                      "<li><code>S</code> : show Selector"
                      "<li><code>G</code> : show Grid"
                      "<li><code>C</code> : show table information"
                      "<li><code>+</code> : zoom in"
                      "<li><code>-</code> : zoom out"
                      "</ul>"
                      "</span>"
                      ));
        mb.setWindowTitle("Graph Shortcuts");
        mb.exec();
        event->accept();
        break;
    }
}

void QuteGraph::setValue(double value)
{
	QuteWidget::setValue(value);
	m_value2 = getTableNumForIndex((int) value);
}

void QuteGraph::setValue(QString text)
{
    if(text.startsWith("@find")) {
        auto pattern = text.mid(5).trimmed();
        int index = findCurve(pattern);
        if(index < 0) {
            qDebug() << "@find: display/dispfft not found";
            return;
        }
        qDebug() << "-------------- Command: " << text << "found index" << index;
        this->setValue(index);
    } else {
        qDebug() << "Command" << text << "not found";
    }
}


void QuteGraph::refreshWidget()
{
    bool needsUpdate = false;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	int index = 0;
	if (m_valueChanged) {
        index = (int) m_value;
        m_value2 = getTableNumForIndex(index);
		m_value2Changed = false;
		m_valueChanged = false;
		needsUpdate = true;
	}
	else if (m_value2Changed) {
        index = getIndexForTableNum((int)m_value2);
        if (index >= 0) {
            m_value = index;
			//      m_valueChanged = false;
        }
        m_value2Changed = false;
        needsUpdate = true;
	}
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();  // unlock
#endif
    if (needsUpdate) {
		if (index < 0) {
			index = getIndexForTableNum(-index);
		}
        if (index < 0 ||
            index >= curves.size() ||
            curves[index]->get_caption().isEmpty()) {
            // Don't show if curve has no name. Is this likely?
			return;
		}
		//    m_pageComboBox->blockSignals(true);
		//    m_pageComboBox->setCurrentIndex(index);
		//    m_pageComboBox->blockSignals(false);
		changeCurve(index);

	}
    // QComboBox *cb = this->m_pageComboBox;
    // cb->move(this->width() - cb->width(),  this->height()-cb->height());
}

void QuteGraph::createPropertiesDialog()
{
	QuteWidget::createPropertiesDialog();
	dialog->setWindowTitle("Graph");

	channelLabel->setText("Index Channel name =");
	channelLabel->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
	//  nameLineEdit->setText(getChannelName());

	QLabel *label = new QLabel(dialog);
	label = new QLabel(dialog);
	label->setText("F-table Channel name =");
	layout->addWidget(label, 4, 0, Qt::AlignRight|Qt::AlignVCenter);
	name2LineEdit = new QLineEdit(dialog);
	name2LineEdit->setText(getChannel2Name());
	name2LineEdit->setMinimumWidth(320);
	layout->addWidget(name2LineEdit, 4,1,1,3, Qt::AlignLeft|Qt::AlignVCenter);

	label = new QLabel(dialog);
	label->setText("Zoom X");
	layout->addWidget(label, 8, 0, Qt::AlignRight|Qt::AlignVCenter);
	zoomxBox = new QDoubleSpinBox(dialog);
	zoomxBox->setRange(0.1, 10.0);
	zoomxBox->setDecimals(1);
	zoomxBox->setSingleStep(0.1);
	layout->addWidget(zoomxBox, 8, 1, Qt::AlignLeft|Qt::AlignVCenter);

	label = new QLabel(dialog);
	label->setText("Zoom Y");
	layout->addWidget(label, 8, 2, Qt::AlignRight|Qt::AlignVCenter);
	zoomyBox = new QDoubleSpinBox(dialog);
	zoomyBox->setRange(0.1, 10.0);
	zoomyBox->setDecimals(1);
	zoomyBox->setSingleStep(0.1);
	layout->addWidget(zoomyBox, 8, 3, Qt::AlignLeft|Qt::AlignVCenter);

    showSelectorCheckBox = new QCheckBox(dialog);
    showSelectorCheckBox->setText("Show Selector");
    showSelectorCheckBox->setCheckState(
                property("QCS_showSelector").toBool()?Qt::Checked:Qt::Unchecked);
    layout->addWidget(showSelectorCheckBox, 9, 0, Qt::AlignLeft|Qt::AlignVCenter);

    showGridCheckBox = new QCheckBox(dialog);
    showGridCheckBox->setText("Show Grid");
    showGridCheckBox->setChecked(property("QCS_showGrid").toBool());
    showGridCheckBox->setToolTip("Show the grid. Has effect only for spectral graphs");
    layout->addWidget(showGridCheckBox, 9, 1, Qt::AlignLeft|Qt::AlignVCenter);

    showTableInfoCheckBox = new QCheckBox(dialog);
    showTableInfoCheckBox->setText("Show Table Information");
    showTableInfoCheckBox->setCheckState(
                property("QCS_showTableInfo").toBool()?Qt::Checked:Qt::Unchecked);
    showTableInfoCheckBox->setToolTip("Show the grid. Has effect only for spectral graphs");
    layout->addWidget(showTableInfoCheckBox, 10, 0, Qt::AlignLeft|Qt::AlignVCenter);

    showScrollbarsCheckBox = new QCheckBox(dialog);
    showScrollbarsCheckBox->setText("Show Scrollbars");
    showScrollbarsCheckBox->setCheckState(
                property("QCS_showScrollbars").toBool()?Qt::Checked:Qt::Unchecked);
    layout->addWidget(showScrollbarsCheckBox, 10, 1, Qt::AlignLeft|Qt::AlignVCenter);

#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	zoomxBox->setValue(property("QCS_zoomx").toDouble());
	zoomyBox->setValue(property("QCS_zoomy").toDouble());
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	//   channelLabel->hide();
	//   nameLineEdit->hide();
}

void QuteGraph::applyProperties()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	setProperty("QCS_objectName2", name2LineEdit->text());
	setProperty("QCS_zoomx", zoomxBox->value());
	setProperty("QCS_zoomy", zoomyBox->value());
    setProperty("QCS_dispx", 1);
	setProperty("QCS_dispy", 1);
	setProperty("QCS_modex", "lin");
	setProperty("QCS_modey", "lin");
	setProperty("QCS_all", true);
    setProperty("QCS_showSelector", showSelectorCheckBox->checkState());
    setProperty("QCS_showGrid", showGridCheckBox->checkState());
    setProperty("QCS_showScrollbars", showScrollbarsCheckBox->isChecked());
    showTableInfo(showTableInfoCheckBox->checkState());

#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	QuteWidget::applyProperties();
}

int QuteGraph::findCurve(QString text) {
    // returns the index or -1 if not found
    for (int i = 0; i < curves.size(); i++) {
        QString caption = curves[i]->get_caption();
        if (caption.contains(text)) {
            qDebug() << "finding" << text << "found: " << caption;
            QStringList parts = text.split(QRegExp("[ :]"), QString::SkipEmptyParts);
            return i;
        }
    }
    return -1;
}

void QuteGraph::changeCurve(int index)
{    
    if(curves.size() <= 0)
        return;

    int origRequest = index;
    StackedLayoutWidget *stacked =  static_cast<StackedLayoutWidget *>(m_widget);
    if (index == -1) { // goto last curve
        index = stacked->count() - 1;
	}
	else if (index == -2) { // update curve but don't change which
        if (m_value < 0)
			index = getIndexForTableNum(-m_value);
        else
			index = (int) m_value;
	}
    else if (stacked->currentIndex() == index) {
        return;
    } else if (index >= stacked->count()) {
        qDebug() << "changeCurve: index out of range. Num indices:"<<stacked->size();
        return;
    } else {
        qDebug()<<"changeCurve"<<index;

        // change curve
        auto view = stacked->currentWidget();
        view->hide();
        stacked->blockSignals(true);
        stacked->setCurrentIndex(index);
        stacked->blockSignals(false);
        m_pageComboBox->blockSignals(true);
        m_pageComboBox->setCurrentIndex(index);
        m_pageComboBox->blockSignals(false);
    }

    if (index < 0  || index >= curves.size())  // Invalid index
        return;

    m_value = index;
    switch(graphtypes[index]) {
    case GraphType::GRAPH_FTABLE: {
        int ftable = getTableNumForIndex(index);
        if (m_value2 != ftable) {
            m_value2 = ftable;
            m_value2Changed = true;
        }
        if(m_drawTableInfo) {
            auto curve = curves[index];
            auto text = QString("%1 pts (%2, %3)")
                    .arg(curve->get_size())
                    .arg(curve->get_max(), 0, 'f', 3)
                    .arg(curve->get_min(), 0, 'f', 3);
            m_label->setText(text);
            m_label->show();
        }
        if(origRequest >= 0)
            drawGraph(curves[index], index);
        break;
    }
    case GraphType::GRAPH_SPECTRUM:
        m_value2 = -1;
        m_label->hide();
        break;
    case GraphType::GRAPH_AUDIOSIGNAL:
        m_value2 = -1;
        m_label->hide();
        break;
    }
    scaleGraph(index);
}

void QuteGraph::indexChanged(int index)
{
    qDebug()<<"indexChanged"<<index<<"\n";
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	if (m_channel == "") {
		setInternalValue(index);
	}
	else {
        qDebug() << "channel value" << m_channel << index << "\n";
		QPair<QString, double> channelValue(m_channel, index);
		emit newValue(channelValue);
	}
	if (m_channel2 == "") {
		setValue2(getTableNumForIndex(index));
	}
	else {
		QPair<QString, double> channel2Value(m_channel2, getTableNumForIndex(index));
		emit newValue(channel2Value);
	}

#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif

}

void QuteGraph::clearCurves()
{
	//  curveLock.lock();
	m_widget->blockSignals(true);
	static_cast<StackedLayoutWidget *>(m_widget)->clearCurves();
	m_widget->blockSignals(false);
	m_pageComboBox->blockSignals(true);
	m_pageComboBox->clear();
	m_pageComboBox->blockSignals(false);
	curves.clear();
	lines.clear();
	polygons.clear();
	m_gridlines.clear();
    m_gridTextsX.clear();
    m_gridTextsY.clear();
	//  curveLock.unlock();
}

void QuteGraph::addCurve(Curve * curve)
{
    Q_ASSERT(curve != nullptr);
	QGraphicsView *view = new QGraphicsView(m_widget);
	QGraphicsScene *scene = new QGraphicsScene(view);
	view->setContextMenuPolicy(Qt::NoContextMenu);
    view->setScene(scene);
    view->setObjectName(curve->get_caption());
    auto scrollbarPolicy = property("QCS_showScrollbars").toBool() ? Qt::ScrollBarAsNeeded
                                                                   : Qt::ScrollBarAlwaysOff;
    view->setHorizontalScrollBarPolicy(scrollbarPolicy);
    view->show();
    scene->setBackgroundBrush(QBrush(Qt::black));
    lines.append(QVector<QGraphicsLineItem *>());
    QVector<QGraphicsLineItem *> gridLinesVector;
    QVector<QGraphicsTextItem *> gridTextVectorX;
    QVector<QGraphicsTextItem *> gridTextVectorY;
    qreal freqStep = 1000.0;
    // Max. samplerate. We generate a grid for this and can make lines/labels visible or
    // not depending on the actual sample rate
    qreal sr = 96000;
    int numTicksX = (int)(sr*0.5 / freqStep);
    int numTicksY = m_numticksY;
    auto gridpen = QPen(QColor(90, 90, 90));
    auto gridpen2 = QPen(QColor(60, 60, 60));
    gridpen.setCosmetic(true);
    gridpen2.setCosmetic(true);
    QString caption = curve->get_caption();
    GraphType graphType;
    if(caption.contains("fft")) {
        graphType = GraphType::GRAPH_SPECTRUM;
        view->setRenderHint(QPainter::Antialiasing);
    } else if(caption.contains("ftable")) {
        graphType = GraphType::GRAPH_FTABLE;
        view->setRenderHint(QPainter::Antialiasing);
    } else {
        graphType = GraphType::GRAPH_AUDIOSIGNAL;
    }

    if(graphType == GraphType::GRAPH_SPECTRUM) {
        for (int i = 0 ; i < numTicksY; i++) {
            // 0 - 0db, 5 -> 100 dB (6:120
            QGraphicsLineItem *gridLine = new QGraphicsLineItem();
            gridLine->setPen(gridpen);
            scene->addItem(gridLine);
            gridLinesVector.append(gridLine);
            QGraphicsTextItem *gridText = new QGraphicsTextItem();
            gridText->setDefaultTextColor(Qt::gray);
            gridText->setFlags(QGraphicsItem::ItemIgnoresTransformations);
            int dbs = round(float(i)/numTicksY * 120.0);
            gridText->setHtml(QString("<div style=\"background:#000000;\">-%1 </p>"
                                      ).arg(dbs));
            gridText->setFont(QFont("Sans", 6));
            gridText->setVisible(false);
            scene->addItem(gridText);
            gridTextVectorY.append(gridText);

        }

        for (int i = 0; i < numTicksX; i++) {
            QGraphicsLineItem *gridLine = new QGraphicsLineItem();
            gridLine->setPen(i%2 == 0 ? gridpen : gridpen2);
            scene->addItem(gridLine);
            gridLinesVector.append(gridLine);
            QGraphicsTextItem *gridText = new QGraphicsTextItem();
            gridText->setDefaultTextColor(Qt::gray);
            gridText->setFlags(QGraphicsItem::ItemIgnoresTransformations);
            if (i > 0 && i%2==0) {
                // double kHz = i*((numTicksX-1.0)/numTicksX) * 2.0;
                double kHz = i * freqStep / 1000.0 ;
                if(i%10==0)
                    gridText->setHtml(
                                QString("<div style=\"background:#000000;\">%1k</p>")
                                .arg(kHz, 2, 'f', 1));
                else
                    gridText->setHtml(
                                QString("<div style=\"background:#000000;\">%1</p>")
                                .arg(kHz, 2, 'f', 1));

            }
            gridText->setFont(QFont("Sans", 6));
            gridText->setVisible(false);
            scene->addItem(gridText);
            gridTextVectorX.append(gridText);
        }
    }

    m_gridlines.append(gridLinesVector);
    m_gridTextsX.append(gridTextVectorX);
    m_gridTextsY.append(gridTextVectorY);

    graphtypes.append(graphType);

    QGraphicsPolygonItem * item = new QGraphicsPolygonItem();
    auto graphPen = QPen(Qt::yellow);
    graphPen.setCosmetic(true);
    item->setPen(graphPen);
    item->show();
    polygons.append(item);
    scene->addItem(item);
    view->setResizeAnchor (QGraphicsView::NoAnchor);
    // view->setFocusPolicy(Qt::NoFocus);
	m_pageComboBox->blockSignals(true);
	m_pageComboBox->addItem(curve->get_caption());
	m_pageComboBox->blockSignals(false);
	//  curveLock.lock();
	static_cast<StackedLayoutWidget *>(m_widget)->addWidget(view);
    curves.append(curve);
    if (m_value == curves.size() - 1) {
        // If new curve created corresponds to current stored value
		changeCurve(m_value);
	}
}

int QuteGraph::getCurveIndex(Curve * curve)
{
    Q_ASSERT(curve != nullptr);
	int index = -1;
	for (int i = 0; i < curves.size(); i++) {
		if (curves[i] == curve) {
			index = i;
			break;
		}
	}
    return index;
}

QGraphicsView * QuteGraph::getView(int index) {
    StackedLayoutWidget *widget_ = static_cast<StackedLayoutWidget *>(m_widget);
    QGraphicsView *view = static_cast<QGraphicsView *>(widget_->widget(index));
    return view;
}

void QuteGraph::drawGraph(Curve *curve, int index) {
    // QString caption = curve->get_caption();
    // auto view = getView(index);

    switch(graphtypes[index]) {
    case GraphType::GRAPH_FTABLE:
        // drawFtable(curve, index);
        // view->setRenderHint(QPainter::Antialiasing);

        drawFtablePath(curve, index);
        break;
    case GraphType::GRAPH_SPECTRUM:
        // view->setRenderHint(QPainter::Antialiasing);
        drawSpectrum(curve, index);
        // drawSpectrumPath(curve, index);
        break;
    case GraphType::GRAPH_AUDIOSIGNAL:
        // drawSignal(curve, index);
        drawSignalPath(curve, index);
        break;
    }
    changeCurve(-2); //update curve
}

void QuteGraph::setCurveData(Curve * curve)
{
    Q_ASSERT(curve != nullptr);
	int index = getCurveIndex(curve);

    if (index >= curves.size() ||
        index < 0 ||
        index != m_value) {
        return;
	}

    auto view = getView(index);

    // Refitting curves in view resets the scrollbar so we need the previous value
    int viewPosx = view->horizontalScrollBar()->value();
	int viewPosy = view->verticalScrollBar()->value();

    drawGraph(curve, index);
    view->horizontalScrollBar()->setValue(viewPosx);
    view->verticalScrollBar()->setValue(viewPosy);
}

void QuteGraph::applyInternalProperties()
{
	QuteWidget::applyInternalProperties();
    if(property("QCS_showSelector").toBool()) {
        m_pageComboBox->show();
    } else {
        m_pageComboBox->hide();
    }
	changeCurve(-2);  // Redraw
    m_drawGrid = property("QCS_showGrid").toBool();
    m_drawTableInfo = property("QCS_showTableInfo").toBool();

    showScrollbars(property("QCS_showScrollbars").toBool());
}

void QuteGraph::drawFtablePath(Curve *curve, int index) {
    Q_ASSERT(index >= 0);
    QGraphicsScene *scene = this->getView(index)->scene();
    // QGraphicsScene *scene = static_cast<QGraphicsView *>(static_cast<StackedLayoutWidget *>(m_widget)->widget(index))->scene();
    double max = curve->get_max();
    max = max == 0 ? 1: max;
    int curveSize = curve->get_size();

    int decimate = curveSize /1024;
    if (decimate == 0) {
        decimate = 1;
    }
    auto pen = QPen(QColor(255, 45, 7), 0);

    auto rect = this->rect();
    int width = rect.width();
    int step = curveSize / width;
    if(step == 0)
        step = 1;

    QPainterPath path;
    for (int i = 0; i < curveSize; i+=step) {
        double value = curve->get_data(i);
        path.lineTo(QPointF(i, -value));
    }
    scene->clear();
    if(step > 1) {
        pen.setWidth(0);
    }
    scene->addPath(path, pen);
}


void QuteGraph::drawFtable(Curve * curve, int index)
{
	//  bool live = curve->getOriginal() != 0;
	Q_ASSERT(index >= 0);
    QString caption = curve->get_caption();
    //  qDebug() << "QuteGraph::drawCurve" << caption << curve->getOriginal() << curve->get_size() << curve->getOriginal()->npts << curve->get_max() << curve->get_min();
    if (caption.isEmpty()) {
        return;
    }
    QGraphicsScene *scene = static_cast<QGraphicsView *>(static_cast<StackedLayoutWidget *>(m_widget)->widget(index))->scene();
    double max = curve->get_max();
    max = max == 0 ? 1: max;
    int size = (int) curve->get_size();
    int decimate = size /1024;
    if (decimate == 0) {
        decimate = 1;
    }
    auto pen = QPen(QColor(255, 45, 7));
    pen.setCosmetic(true);
    if (lines[index].size() != size) {
        foreach (QGraphicsLineItem *line, lines[index]) {
            scene->removeItem(line);
            delete line;
        }
        lines[index].clear();
        for (int i = 0; i < size; i++) {
            if (decimate == 0 || i%decimate == 0) {
                QGraphicsLineItem *line = new QGraphicsLineItem(i, 0, i, 0);
                line->setPen(pen);
                lines[index].append(line);
                scene->addItem(line);
            }
        }
    }
    for (int i = 0; i < lines[index].size(); i++) { //skip first item, which is base line
        QGraphicsLineItem *line = static_cast<QGraphicsLineItem *>(lines[index][i]);
        MYFLT value = curve->get_data((i * decimate));
        line->setLine((i * decimate), 0, (i * decimate),  -value );
        line->show();
    }
    scaleGraph(index);
}

void QuteGraph::drawSpectrumPath(Curve *curve, int index) {
    int curveSize = curve->get_size();
    QGraphicsScene *scene = static_cast<QGraphicsView *>(static_cast<StackedLayoutWidget *>(m_widget)->widget(index))->scene();
    QPainterPath path;
    double db0 = m_ud->zerodBFS;
    path.moveTo(0, -20.0*log10(fabs(curve->get_data(0))/db0));
    for(int i=1; i < curveSize; i++) {
        double value = 20.0*log10(fabs(curve->get_data(i))/db0);
        path.lineTo(QPointF(i, -value));
    }
    scene->clear();
    auto pen = QPen(Qt::yellow);
    pen.setCosmetic(true);

    QPainterPath gridPath;
    QPainter painter;

    if(m_drawGrid) {
        int sr = (int)this->getSr();
        int nyquist = sr / 2;
        int step = 1000;
        int numTicksX = nyquist / step;
        int numTicksY = 7;
        printf("grid: nyquist: %d, numticks: %d\n", nyquist, numTicksX);

        auto gridPen = QPen(QColor(40, 40, 40));
        gridPen.setCosmetic(true);
        auto textColor = QColor(128, 128, 128);
        qreal curveSizeF = (qreal)curveSize;
        auto font = QFont("Sans");
        font.setPixelSize(9);
        const int maxy = 110;
        for (int i = 1; i < numTicksX; i++) {
            qreal freq = i * step;
            qreal x = freq/nyquist * curveSizeF;
            gridPath.moveTo(x, 0);
            gridPath.lineTo(x, maxy);
            if(i%2 == 0) {
                auto item = scene->addText(QString::number(freq/1000.0, 'f', 1), font);
                item->setDefaultTextColor(textColor);
                item->setPos(x, 0);
                item->setFlag(item->ItemIgnoresTransformations, true);
            }
        }
        for (int i=1; i < numTicksY-1; i++) {
            qreal y = (qreal)i/numTicksY * maxy;
            gridPath.moveTo(0, y);
            gridPath.lineTo(curveSizeF, y);
        }
        scene->addPath(gridPath, gridPen);
    }
    scene->addPath(path, pen);
}

void QuteGraph::spectrumGetPeak(Curve *curve, int index,
                                double freq, double relativeBandwidth) {
    qreal sr = this->getSr(44100.);
    // TODO
}


void QuteGraph::drawSpectrum(Curve *curve, int index) {
    int curveSize = curve->get_size();
    QVector<QPointF> polygonPoints;
    polygonPoints.resize(curveSize + 2);
    polygonPoints[0] = QPointF(0,110);
    double db0 = m_ud->zerodBFS;

    for (int i = 0; i < (int) curveSize; i++) {
        auto data = curve->get_data(i);
        double db = data > 0.000001 ? 20.0*log10(data/db0) : -120;
        // double value = 20.0*log10(fabs(curve->get_data(i))/db0);
        polygonPoints[i+1] = QPointF(i, -db); //skip first item, which is base line
    }

    polygonPoints.back() = QPointF(curveSize - 1,110);
    polygons[index]->setPolygon(QPolygonF(polygonPoints));

    // m_pageComboBox->setItemText(index, curve->get_caption());
    // draw Grid
    int numTicksY = m_numticksY;
    auto gridlinesvec = m_gridlines[index];
    auto gridtextvecx = m_gridTextsX[index];
    auto gridtextvecy = m_gridTextsY[index];
    qreal sr = this->getSr(44100.0);
    qreal nyquist = sr * 0.5;
    qreal freqStep = 1000.0;  // TODO: allow to configure this
    int numTicksX = (int)(nyquist / freqStep);
    // TODO: fix y axis
    if(m_drawGrid) {
        gridtextvecy[0]->setVisible(true);
        for (int i = 1; i < numTicksY; i++) {
            int y = float(i)/(numTicksY) * 100.0;
            gridlinesvec[i]->setLine(0, y, curveSize, y);
            gridlinesvec[i]->setVisible(true);
            gridtextvecy[i]->setPos(0, y);
            gridtextvecy[i]->setVisible(true);
        }

        for (int i = 1; i < numTicksX; i++) {
            // qreal x = i * qreal(curveSize)/numTicksX;
            qreal freq = i * freqStep;
            qreal x = freq/nyquist * curveSize;
            int idx = i + m_numticksY;
            gridlinesvec[idx]->setLine(x, 0, x, 120);
            gridlinesvec[idx]->setVisible(true);
            gridtextvecx[i]->setPos(x, 0);
            gridtextvecx[i]->setVisible(true);
        }
    } else {
        for(int i=0; i < numTicksY; i++) {
            gridlinesvec[i]->setVisible(false);
            gridtextvecy[i]->setVisible(false);
        }
        for (int i = 0; i < numTicksX; i++) {
            gridlinesvec[i+m_numticksY]->setVisible(false);
            gridtextvecx[i]->setVisible(false);
        }
    }
}

void QuteGraph::drawSignalPath(Curve *curve, int index) {
    int curveSize = curve->get_size();
    QPainterPath path;
    auto zerodbfs = m_ud->zerodBFS;
    for(int i=0; i<curveSize; i++) {
        auto value = curve->get_data(i)/zerodbfs;
        path.lineTo(i, value);
    }
    QPainterPath grid;
    grid.moveTo(0, 0);
    grid.lineTo(curveSize, 0);

    QGraphicsScene *scene = static_cast<QGraphicsView *>(static_cast<StackedLayoutWidget *>(m_widget)->widget(index))->scene();
    auto pen = QPen(QColor(255, 193, 7), 0);
    scene->clear();
    scene->addPath(grid, QPen(QColor(40, 40, 40), 0));
    scene->addPath(path, pen);
}

void QuteGraph::drawSignal(Curve *curve, int index)
{
    int curveSize = curve->get_size();
    QVector<QPointF> polygonPoints;
    polygonPoints.resize(curveSize + 2);
    polygonPoints[0] = QPointF(0,0);
    for (int i = 0; i < (int) curveSize; i++) {
        double value = curve->get_data(i)/m_ud->zerodBFS;
        polygonPoints[i + 1] = QPointF(i, value); //skip first item, which is base line
    }
    polygonPoints.back() = QPointF(curveSize - 1,0);
    polygons[index]->setPolygon(QPolygonF(polygonPoints));
    auto pen = QPen(QColor(255, 193, 7));
    pen.setCosmetic(true);
    polygons[index]->setPen(pen);
    polygons[index]->setBrush(Qt::NoBrush);
    m_pageComboBox->setItemText(index, curve->get_caption());
}

void QuteGraph::scaleGraph(int index)
{
    auto curve = curves[index];

    double max = curves[index]->get_max();
    double min = curves[index]->get_min();
	double zoomx = property("QCS_zoomx").toDouble();
	double zoomy = property("QCS_zoomy").toDouble();
	//  double span = max - min;
    //  FIXME implement dispx, dispy and modex, modey
    int size = curve->get_size();
    double sizef = (double)size;
    auto view = getView(index);
    //  view->setResizeAnchor(QGraphicsView::NoAnchor);
    auto graphType = graphtypes[index];
    if(graphType == GraphType::GRAPH_FTABLE && max != min) {
        double factor = 1.17;
        view->setSceneRect(0, -max*factor - 0.05, sizef, (max - min)*factor);
        view->fitInView(0, -max*factor/zoomy, sizef/zoomx, (max - min)*factor/zoomy);
    } else if(graphType == GraphType::GRAPH_SPECTRUM) {
        double dbRange = 90.;
        view->setSceneRect (0, -3, size, dbRange);
        view->fitInView(0, -3/zoomy, sizef/zoomx, dbRange/zoomy);
    } else { //from display opcode
        view->setSceneRect (0, -1, size, 2);
        // view->fitInView(0, -10./zoomy, (double) size/zoomx, 10./zoomy);
        view->fitInView(0, -2./zoomy, sizef/zoomx, 2./zoomy);
	}

}

int QuteGraph::getTableNumForIndex(int index) {
    if (index < 0 || index >= curves.size() || curves.size() <= 0) {
        // Invalid index
		return -1;
	}
	int ftable = -1;
    if(graphtypes[index] == GraphType::GRAPH_FTABLE) {
        QString caption = curves[index]->get_caption();
		ftable= caption.mid(caption.indexOf(" ") + 1,
							caption.indexOf(":") - caption.indexOf(" ") - 1).toInt();
	}
    return ftable;
}

int QuteGraph::getIndexForTableNum(int ftable)
{
	int index = -1;
	for (int i = 0; i < curves.size(); i++) {
		QString text = curves[i]->get_caption();
		if (text.contains("ftable")) {
			QStringList parts = text.split(QRegExp("[ :]"), QString::SkipEmptyParts);
            if (parts.size() > 1) {
				int num = parts.last().toInt();
				if (ftable == num) {
					index = i;
					break;
				}
			}
		}
	}
	return index;
}

void QuteGraph::setInternalValue(double value)
{
	m_value = value;
	m_valueChanged = true;
}

void QuteGraph::showScrollbars(bool show) {
    auto policy = show ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff;
    for(int i=0; i < this->curves.size(); i++) {
        auto view = getView(i);
        view->setHorizontalScrollBarPolicy(policy);
    }
    m_showScrollbars = show;
}

// ----------------------
QuteTableWidget::~QuteTableWidget() {
};

void QuteTableWidget::reset() {
    QMutexLocker locker(&mutex);
    m_tabnum = 0;
    m_running = false;
    m_data = nullptr;
    m_tabsize = 0;
    if(m_autorange)
        m_maxy = 1.0;
    if(m_path != nullptr) {
        delete m_path;
        m_path = nullptr;
    }
}

void QuteTableWidget::paintGrid(QPainter *painter) {
    int margin = m_margin;
    QString maxystr;
    if(m_maxy >= 10)
        maxystr = QString::number((int)m_maxy);
    else if(m_maxy >= 1)
        maxystr = QString::number(m_maxy, 'f', 1);
    else
        maxystr = QString::number(m_maxy, 'f', 2);
    auto rect = this->rect();
    const int yoffset = 0;
    auto x0 = rect.x() + margin;
    auto x1 = rect.x() + rect.width() - margin + 1;
    auto y0 = rect.y() + margin + yoffset;
    auto y1 = rect.y() + rect.height() - margin + yoffset ;
    auto ycenter = (y0+y1) / 2;

    auto font = QFont({"Sans", 8});
    QFontMetrics fm(font);
    auto tabsizestr = QString::number(m_tabsize);
    const int textMargin = 4;
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(QColor(96, 96, 96), 0));
    painter->drawLine(rect.x() + fm.boundingRect(maxystr).width()+textMargin*2, y0, x1, y0);
    painter->drawLine(x0, ycenter, x1, ycenter);
    painter->drawLine(x0, y1, x1 - fm.boundingRect(tabsizestr).width() - 2, y1);
    painter->setPen(QColor(48, 48, 48));
    painter->drawLine(x0, (y0+ycenter)/2, x1, (y0+ycenter)/2);
    painter->drawLine(x0, (y1+ycenter)/2, x1, (y1+ycenter)/2);

    painter->setPen(QColor(200, 200, 200));
    painter->setFont(font);
    painter->drawText(rect.x()+textMargin, y0+textMargin, maxystr);
    // right padding
    rect.setWidth(rect.width() - textMargin);
    painter->drawText(rect, Qt::AlignRight|Qt::AlignBottom, tabsizestr);

}

void QuteTableWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    bool running = csoundEngineStatus(m_ud) == CsoundEngineStatus::Running;
    if(!running) {
        painter.setBrush(QColor(176, 0, 32));
        painter.drawRect(this->rect());
        painter.setPen(Qt::white);
        painter.drawText(this->rect(), Qt::AlignCenter, "Stopped");
        return;
    }
    if(m_tabnum <= 0) {
        // table not set
        painter.setBrush(QColor(0, 176, 32));
        painter.drawRect(this->rect());
        painter.setPen(Qt::white);
        painter.drawText(this->rect(), Qt::AlignCenter, "Table not set");
        return;
    }
    // running OK
    painter.setBrush(QColor(24, 24, 24));
    painter.drawRect(this->rect());
    painter.setBrush(Qt::NoBrush);
    if(m_path == nullptr)
        return;
    mutex.lock();
    if(m_showGrid) {
        this->paintGrid(&painter);
    }
    painter.setPen(QPen(m_color, 0));
    painter.drawPath(*m_path);

    mutex.unlock();
}

void QuteTableWidget::setRange(double maxy) {
    if(maxy == 0.0) {
        m_autorange = true;
    } else {
        m_autorange = false;
        m_maxy = maxy;
    }
}

void QuteTableWidget::updatePath() {
    if(m_tabnum <= 0 && m_data == nullptr)
        return;
    if(csoundEngineStatus(m_ud) != CsoundEngineStatus::Running)
        return;
    int margin = m_margin;
    auto rect = this->rect();
    auto width = rect.width() - margin*2;
    auto height = rect.height() - margin*2;
    double maxy = this->m_maxy;
    double newmaxy = maxy;
    double xscale = width / (double)m_tabsize;
    double yscale = height * 0.5 / maxy;
    double y0 = rect.y() + margin;
    double x0 = rect.x() + margin;
    int step = m_tabsize / width;
    if(step == 0)
        step = 1;

    auto path = new QPainterPath();
    double ydata = m_data[0];
    path->moveTo(x0, (ydata+maxy)*yscale+y0);
    for(int i=0; i < m_tabsize; i+=step) {
        ydata = m_data[i];
        double x2 = i*xscale + x0;
        double y2 = (ydata+maxy)*yscale + y0;
        path->lineTo(x2, y2);
        if(ydata > newmaxy)
            newmaxy = ydata;
    }
    mutex.lock();
    if(m_autorange) {
        m_maxy = ceil(newmaxy);
    }
    if(m_path != nullptr)
        delete m_path;
    m_path = path;
    mutex.unlock();
}

void QuteTableWidget::updateData(int tabnum, bool check) {
    if(check && csoundEngineStatus(m_ud) != CsoundEngineStatus::Running) {
        m_tabnum = 0;
        return;
    }
    if(tabnum == -1 || tabnum == m_tabnum ) {
        if(m_tabnum == 0 || m_data == nullptr || m_tabsize == 0) {
            qDebug() << "Table not set, can't update";
            return;
        }
        this->updatePath();
        this->update();
        return;
    }
    // Asked to change table ( or set for the first time )
    MYFLT *data;
    int tabsize = csoundGetTable(m_ud->csound, &data, tabnum);
    if(tabsize == 0 || data == nullptr) {
        qDebug() << "Table" << tabnum << "not found";
        this->reset();
        return;
    }
    mutex.lock();
    m_data = data;
    m_tabsize = tabsize;
    if(m_autorange && m_tabnum != tabnum) {
        m_maxy = 1.0;
    }
    m_tabnum = tabnum;
    mutex.unlock();
    this->updatePath();
    this->update();
}


// -------------------------

QuteTable::~QuteTable() {};

// void QuteTable::mousePressEvent(QMouseEvent *event) {};
// void QuteTable::mouseReleaseEvent(QMouseEvent *event) {};

QuteTable::QuteTable(QWidget *parent) : QuteWidget(parent) {
    m_widget = new QuteTableWidget(this);
    m_value = 0;
    m_tabnum = 0;
    // auto w = static_cast<QuteTableWidget*>(m_widget);
    setProperty("QCS_randomizable", false);
    m_widget->setContextMenuPolicy(Qt::NoContextMenu);
    m_widget->setMouseTracking(true);
    setProperty("QCS_range", 0.0);
    setColor(QColor(255, 193, 3));
    setProperty("QCS_showGrid", true);
}

void QuteTable::setColor(QColor color) {
    setProperty("QCS_color", color);
    static_cast<QuteTableWidget*>(m_widget)->setColor(color);
}

void QuteTable::applyInternalProperties() {
    QuteWidget::applyInternalProperties();
    auto w = static_cast<QuteTableWidget*>(m_widget);
    auto color = property("QCS_color").value<QColor>();
    w->setColor(color);
    w->setRange(property("QCS_range").toDouble());
    w->showGrid(property("QCS_showGrid").toBool());
}

void QuteTable::createPropertiesDialog() {
    QuteWidget::createPropertiesDialog();
    dialog->setWindowTitle("Table Plot");
    auto label = new QLabel(dialog);
    label->setText("Color");
    layout->addWidget(label, 4, 0, Qt::AlignRight|Qt::AlignVCenter);

    colorButton = new SelectColorButton(dialog);
    colorButton->setColor(property("QCS_color").value<QColor>());
    layout->addWidget(colorButton, 4, 1, Qt::AlignLeft|Qt::AlignVCenter);

    double range = property("QCS_range").toDouble();
    rangeCheckBox = new QCheckBox("Fixed Range", dialog);
    rangeCheckBox->setToolTip("If checked the range is fixed to the value set on the right."
                              "If the table has values outside this range these will not be"
                              "displayed");
    rangeCheckBox->setChecked(range > 0.0);
    layout->addWidget(rangeCheckBox, 5, 1, Qt::AlignRight|Qt::AlignVCenter);

    rangeSpinBox = new QDoubleSpinBox(dialog);
    rangeSpinBox->setRange(0.1, 999999.0);
    rangeSpinBox->setSingleStep(0.1);
    rangeSpinBox->setToolTip("Sets the max. range to plot. Disable this to use autorange"
                             "With a fixed range, values outside the range will not be "
                             "visiable");
    rangeSpinBox->setValue(range > 0 ? range : 1.0);
    layout->addWidget(rangeSpinBox, 5, 2, Qt::AlignLeft|Qt::AlignVCenter);
    connect(rangeCheckBox, SIGNAL(toggled(bool)), rangeSpinBox, SLOT(setEnabled(bool)));

    gridCheckBox = new QCheckBox("Show Grid", dialog);
    gridCheckBox->setChecked(property("QCS_showGrid").toBool());
    layout->addWidget(gridCheckBox, 6, 1, Qt::AlignLeft|Qt::AlignVCenter);

}

void QuteTable::applyProperties() {
#ifdef  USE_WIDGET_MUTEX
    widgetLock.lockForWrite();
#endif
    setProperty("QCS_color", colorButton->getColor());
    bool fixedrange = rangeCheckBox->isChecked();
    double range = rangeSpinBox->value();
    if(fixedrange) {
        Q_ASSERT(range > 0.0);
        setProperty("QCS_range", range);
    } else {
        setProperty("QCS_range", 0.0);
    }
    setProperty("QCS_showGrid", gridCheckBox->isChecked());

#ifdef  USE_WIDGET_MUTEX
    widgetLock.unlock();
#endif
    QuteWidget::applyProperties();
}

void QuteTable::refreshWidget() {
    Q_ASSERT(m_valueChanged);
    QMutexLocker locker(&mutex);
    m_valueChanged = false;
    if(csoundEngineStatus(m_csoundUserData) != CsoundEngineStatus::Running)
        return;
    auto w = static_cast<QuteTableWidget*>(m_widget);
    int tabnum = (int)m_value;
    w->blockSignals(true);
    w->updateData(tabnum, true);
    w->blockSignals(false);
}

QString QuteTable::getWidgetXmlText() {
    xmlText = "";
    QXmlStreamWriter s(&xmlText);
    createXmlWriter(s);        // --------- start

    QColor color = property("QCS_color").value<QColor>();
    s.writeStartElement("color");
    s.writeTextElement("r", QString::number(color.red()));
    s.writeTextElement("g", QString::number(color.green()));
    s.writeTextElement("b", QString::number(color.blue()));
    s.writeEndElement();

    auto range = property("QCS_range").toDouble();
    s.writeTextElement("range", QString::number(range, 'f', 2));

    s.writeEndElement();      // --------- end
    return xmlText;
}

void QuteTable::onStop() {
    mutex.lock();
    this->blockSignals(true);
    m_tabnum = 0;
    m_value = 0;
    m_valueChanged = true;

    auto w = static_cast<QuteTableWidget*>(m_widget);
    w->setUserData(m_csoundUserData);
    w->blockSignals(true);
    w->reset();
    w->blockSignals(false);
    // this->applyInternalProperties();
    this->blockSignals(false);
    mutex.unlock();
}

void QuteTable::setCsoundUserData(CsoundUserData *ud) {
    qDebug()<<"QuteTable::setCsoundUserData" << ud;
    if(ud == nullptr) {
        qDebug()<<"CsoundUserData is null";
        return;
    }
    QMutexLocker locker(&mutex);
    m_csoundUserData = ud;
    auto w = static_cast<QuteTableWidget*>(m_widget);
    if(w == nullptr) {
        qDebug() << "widget is null";
        return;
    }
    w->setUserData(ud);
    connect(ud->csEngine, SIGNAL(stopSignal()), this, SLOT(onStop()));
}

/*
void QuteTable::setWidgetGeometry(int x, int y, int width, int height) {

    if(csoundEngineStatus(m_csoundUserData)==CsoundEngineStatus::Running) {
        return;
    }
    QuteWidget::setWidgetGeometry(x,y,width, height);
}
*/

void QuteTable::setValue(double value) {
    if(m_value == value) {
        return;
    }

#ifdef USE_WIDGET_MUTEX
    widgetLock.lockForWrite();
#endif
    mutex.lock();
    if(value == -1 && m_tabnum > 0) {
        // update data, don't change table number
        m_valueChanged = false;
        auto w = static_cast<QuteTableWidget*>(m_widget);
        w->blockSignals(true);
        w->updateData(m_tabnum);
        w->blockSignals(false);
        mutex.unlock();
        return;
    }
    m_value = value;
    m_valueChanged = true;
    m_tabnum = (int)value;
    mutex.unlock();
    static_cast<QuteTableWidget*>(m_widget)->updateData(m_tabnum);

#ifdef USE_WIDGET_MUTEX
    widgetLock.unlock();
#endif

};

void QuteTable::setValue(QString s) {
    if(s.isEmpty())
        return;
    auto parts = s.splitRef(' ', QString::SkipEmptyParts);
    if(parts.size() == 0)
        return;
    if(parts[0] == "@set") {
        if(parts.size() != 2) {
            qDebug() << "@set message expects a table number (example: @set 103)";
            return;
        }
        int tabnum = parts[1].toInt();
        setValue((double)tabnum);
    } else if (parts[0] == "@update") {
        setValue(-1);
    } else
        qDebug() << "Message not supported:" << s;
}
