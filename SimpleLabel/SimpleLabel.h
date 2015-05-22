/*	SimpleLabel - a simple and light program for semi automatic labeling of regions
	of interest on images or image sequences. 
	Developed at Laboratory for Active and Attentive Vision, York University, Toronto.
	http://www.cse.yorku.ca/LAAV/home/ headed by John K. Tsotsos.

    Copyright (C) 2010  Eugene Simine.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

	Contact: 	Eugene Simine <eugene@cse.yorku.ca> or
				John K. Tsotsos <tsotsos@cse.yorku.ca>

*/
#ifndef LABELME_H
#define LABELME_H

#include <QtGui/QMainWindow>
#include <QPolygon>
#include "ui_SimpleLabel.h"
#include "Constants.h"

class Monitor;
class About;
class SaveDialog;

class SimpleLabel : public QMainWindow
{
	Q_OBJECT

public:
	SimpleLabel(QWidget *parent = 0, Qt::WFlags flags = 0);
	~SimpleLabel();


public:
	QList<Label> mLabels;

private:
	void resetLabels();
	void resetFilenames();
	void releaseCapture();
	QPoint screenToImage(QPoint scr);
	QPoint imageToScreen(QPoint im);
	QPoint imageToImage(int srcW, int srcH, QPoint srcP, int destW, int destH);
	QRect screenToImage(QRect scr);
	QRect imageToScreen(QRect im);
	QRect imageToImage(int srcW, int srcH, QRect srcP, int destW, int destH);
	QPolygon screenToImage(QPolygon scr);
	QPolygon imageToScreen(QPolygon im);
	QPolygon imageToImage(int srcW, int srcH, QPolygon srcP, int destW, int destH);
	void updateListView();
	void addViaPoint(int frame, QRect rc);
	void addViaPoint(int frame, ViaPoint v);
	void addViaPointPoly(int frame, QPolygon pl, bool addRect = true);
	void rebuildLabelBoxes();
	void rebuildLabelPolygons();
	void setDrawRectToFrame(int v);
	void setDrawPolygonToFrame(int v);
	QRect linearInterpolation(QRect initPoint, double dx, double dy, double dw, double dh, int t);
	QPoint linearInterpolation(QPoint src, QPointF dt, int t);
	void exportFrameToLabelMeXML(QString path, int frame);
	void exportFrameToLabelMeXMLWebTool(QString path, int frame);
	Label* findLabel(int number);
	void exportToMovie(bool origBkgrd);
	void exportToSimpleLabelXML();
	void exportToLabelMeXML();
	void exportToMatlabStruct();
	void drawCirclesAtVertices(QPainter *pt, QRect rc);
	void drawCirclesAtVertices(QPainter *pt, QPolygon &pl);
	void drawModeRect(QPainter *pt, int frame);
	void drawModePoly(QPainter *pt, int frame);
	//QRect rotateRect(QRect rc, float a);

private slots:
	virtual void on_actionOpen_triggered();
	virtual void on_actionLoad_XML_triggered();
	virtual void showImage();
	virtual void paintEvent (QPaintEvent*);
	virtual void on_hSliderFrames_valueChanged(int v);
	virtual void mousePressEvent( QMouseEvent * e );
    virtual void mouseMoveEvent( QMouseEvent * e );
	virtual void mouseReleaseEvent( QMouseEvent* );
	virtual void on_btnAddLabel_pressed();
	virtual void on_btnDeleteLabel_pressed();
	virtual void on_btnToBeginning_pressed();
	virtual void on_btnPrevViaPoint_pressed();
	virtual void on_btnStop_pressed();
	virtual void on_btnRemoveViaPoint_pressed();
	virtual void on_btnPlay_pressed();
	virtual void on_btnNextViaPoint_pressed();
	virtual void on_btnToEnd_pressed();
	virtual void on_listLabels_itemChanged(QListWidgetItem* item);
	virtual void on_listLabels_currentItemChanged (QListWidgetItem * current, QListWidgetItem * previous);
	virtual void on_chkBoxShowAllLabels_toggled(bool b);
	virtual void on_actionAbout_triggered();
	virtual void on_actionLoad_LabelMe_XML_triggered();
	virtual void on_actionExport_triggered();
	virtual void on_SaveDialog_accept();
	virtual void on_cmbBoxLabelShape_currentIndexChanged ( int index );
	virtual void on_actionNewPolygon_triggered();
	virtual void on_actionFinish_triggered();
	virtual void on_textEditDescription_textChanged();

private:
	Ui::SimpleLabelClass ui;

	QImage *mDisplayImage;
	Monitor *mMonitor;

	QPoint mDrawPoint;
	ViaPoint mDrawRect;
	QPolygon mDrawPolygon;
	
	LabelShape mShapeMode;
	bool mMoveRect;
	bool mRotateRect;
	bool mMovePolygon;
	InterpolationMethod mIntrMethod;
	int mResizeVertex;
	QString mFileName;
	QString mFileNamePrefix;
	QString mPath;
	QString mExtention;
	int mFirstFrameNumber;
	bool mSomethingChanged;
	QLabel mStatus_Mode;
	
	bool mNewPolygon;	//when set to true, we are in the process of creating new polygon
	QPolygon mFirstPolygon;

	About *mAboutDlg;

	SaveDialog *mSaveDgl;

	QMenu *mPopupMenu;
	QCursor *mCrossHairCursor;
};

#endif // LABELME_H
