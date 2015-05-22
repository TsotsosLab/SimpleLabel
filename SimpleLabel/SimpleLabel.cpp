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
#include <QFileDialog>
#include <opencv\cv.h>
#include <opencv\highgui.h>
#include <QPainter>
#include <QMouseEvent>
#include <QMessageBox>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>
#include <QDomDocument>
#include <QBitmap>
#include <QProgressBar>
#include "SimpleLabel.h"
#include "Constants.h"
#include "Monitor.h"
#include "About.h"
#include "SaveDialog.h"

const QPoint CommonFunctions::NULL_POINT = QPoint(-1,-1);
const ViaPoint CommonFunctions::NULL_RECT = ViaPoint(-1, 0.0, QRect(0,0,0,0));
const QPolygon CommonFunctions::NULL_POLYGON = QPolygon();

void test_trans()
{
	QRect tmp(10, 10, 21, 21);
	tmp.translate(-tmp.center());
	QTransform tr;
	tr.rotate(30);
	QPointF p = tr.map(QPointF(tmp.topLeft()));
	qDebug() << tmp;


}


SimpleLabel::SimpleLabel(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	//test_trans();

	ui.setupUi(this);
	setMouseTracking(true);
	ui.centralWidget->setMouseTracking(true);
	this->setWindowTitle(this->windowTitle() + " v" + QCoreApplication::applicationVersion());
	mSaveDgl = new SaveDialog(this);
	mAboutDlg = new About(this);
	mDisplayImage = new QImage(W_DISPLAYIMAGE, H_DISPLAYIMAGE, QImage::Format_ARGB32);
//	mDisplayImage= NULL;
	mMonitor= new Monitor();
	
	connect(mMonitor, SIGNAL(imageChanged()), this, SLOT(showImage()), Qt::QueuedConnection);
	connect(mSaveDgl, SIGNAL(accepted()), this, SLOT(on_SaveDialog_accept()));

	
	mDrawPoint = CommonFunctions::NULL_POINT;
	mDrawRect = CommonFunctions::NULL_RECT;
	mMoveRect = false;
	mRotateRect = false;
	mSomethingChanged = false;
	mResizeVertex = -1;
	mIntrMethod = LinearIntr;
	mNewPolygon = false;

	ui.actionLoad_XML->setDisabled(true);
	ui.actionLoad_LabelMe_XML->setDisabled(true);
	ui.actionExport->setDisabled(true);
	ui.hSliderFrames->setDisabled(true);
	ui.actionNewPolygon->setDisabled(true);
	ui.actionSave_Dialog->setEnabled(true);

	mSaveDgl->ui.edtFirstImageIndex->setText("-1");

	statusBar()->addPermanentWidget(&mStatus_Mode);
	on_cmbBoxLabelShape_currentIndexChanged(Rect);

	//setting up the popup menu
	mPopupMenu = new QMenu(this);
	QAction *item = new QAction("Add New Vertex", this);
	item->setObjectName("AddVertex");
	mPopupMenu->addAction(item);
	item = new QAction("Remove This Vertex", this);
	item->setObjectName("RemoveVertex");
	mPopupMenu->addAction(item);

	//create custom cursor
	QBitmap cr(":/cursor_crosshair.bmp");
#ifdef WIN32
	QBitmap mask(":/cursor_crosshair_mask.bmp");
#else
	QBitmap mask(":/cursor_crosshair.bmp");
#endif
	mCrossHairCursor = new QCursor(cr, mask, 15, 15);
}

SimpleLabel::~SimpleLabel()
{
	releaseCapture();
	resetLabels();
	mMonitor->stop();
	mMonitor->wait();
	releaseCapture();
	delete mMonitor;

	delete mDisplayImage;

	delete mCrossHairCursor;
}

void SimpleLabel::resetFilenames()
{
	mFileName = "";
	mFileNamePrefix = "";
	mPath = "";
	mExtention = "";
	mFirstFrameNumber = -1;
}

void SimpleLabel::resetLabels()
{
	mDrawPoint = CommonFunctions::NULL_POINT;
	mDrawRect = CommonFunctions::NULL_RECT;
	mDrawPolygon = CommonFunctions::NULL_POLYGON;
	mMoveRect = false;
	mMovePolygon = false;
	mSomethingChanged = false;

	for(int i = 0; i < mLabels.count(); i++)
	{
		mLabels[i].boxes.clear();
		mLabels[i].viaPoints.clear();
		mLabels[i].polygons.clear();
		mLabels[i].viaPointsPoly.clear();
	}
	mLabels.clear();

	ui.listLabels->clear();
	mSaveDgl->ui.edtFirstImageIndex->setText("-1");
}

void SimpleLabel::on_actionAbout_triggered()
{
	mAboutDlg->show();
}

void SimpleLabel::releaseCapture()
{
	CvCapture *cap = mMonitor->getCvCapture();
	if(cap)
	{
		cvReleaseCapture(&cap);
		mMonitor->setCvCapture(NULL);
	}
}

void SimpleLabel::on_actionOpen_triggered()
{
	int frames = 0;
	int firstframe = 0;
	QString s = QFileDialog::getOpenFileName(this, tr("Open Video"), ".", tr("Image Files (*.png *.tif *.tiff *.jpg);;Video Files (*.avi)"));
	if(!s.isEmpty())
	{
		resetFilenames();
		resetLabels();
		releaseCapture();

		//splitting the full path into parts
		CommonFunctions::splitPath(s, mPath, mFileName, mFileNamePrefix, (uint)5, mFirstFrameNumber, mExtention);

		if(mExtention == ".avi")
		{
			mFileNamePrefix = mFileName;
			//before opening file we make sure that previously opened file if closed
			releaseCapture();

	//		m_Monitor->setLoadBackground(ui.chkBox_Adapt2Bkgd->isChecked());
			//opencv can't open capture from a child thread so we do it here								
			CvCapture* cap = cvCaptureFromFile(s.toAscii());
			mMonitor->setCvCapture(cap);
			frames = mMonitor->getFrameCount();

		}
		else if(mExtention == ".tif" || mExtention == ".tiff" || mExtention == ".png" || mExtention == ".jpg")
		{
			if(mFirstFrameNumber >= 0)
			{
				firstframe = mFirstFrameNumber;
				mMonitor->setFisrtFilenameOfSequence(s);
				frames = mMonitor->getFrameCount();
			}
			else if(mFirstFrameNumber == -1)
			{
				firstframe = mFirstFrameNumber;
				mMonitor->setFisrtFilenameOfSequence(s);
				frames = 1;
			}
			else
			{
				QMessageBox::information(this, "Error", s + ": incorrect numbering format. Expected: fname00000, fname00001, etc...");
			}
		}

		if(frames > 0)
		{
			ui.hSliderFrames->setEnabled(true);
			ui.hSliderFrames->setRange(firstframe, firstframe + frames - 1);
			ui.actionLoad_XML->setEnabled(true);
			ui.actionLoad_LabelMe_XML->setEnabled(true);
			ui.actionExport->setEnabled(true);
		}
		else
			ui.hSliderFrames->setDisabled(true);

	}
}

void SimpleLabel::showImage()
{
	mMonitor->lockImages();
	*mDisplayImage = mMonitor->getImage()->scaled(W_DISPLAYIMAGE, H_DISPLAYIMAGE, Qt::KeepAspectRatio);
	mMonitor->releaseImages();

	if(mMonitor->isRunning())
	{
		ui.hSliderFrames->setSliderPosition(mMonitor->getCurrentFrameNumber());
		update();
	}
	else
		repaint();
}

void SimpleLabel::drawCirclesAtVertices(QPainter *pt, QRect rc)
{
	int rad = CORNER_CIRCLE_RAD;
	pt->setPen(Qt::black);
//	pt->setPen(Qt::PenStyle::SolidLine);

	pt->drawEllipse(rc.topLeft(), rad, rad);
	pt->drawEllipse(rc.topRight(), rad, rad);
	pt->drawEllipse(rc.bottomRight(), rad, rad);
	pt->drawEllipse(rc.bottomLeft(), rad, rad);

	//draw line for rotation
	int x = rc.center().x();
	QLine ln(x, rc.top(), x, rc.top() - LENGTH_OF_ROTATION_LINE);
	pt->drawLine(ln);
	pt->drawEllipse(ln.p2(), rad, rad);
}

void SimpleLabel::drawCirclesAtVertices(QPainter *pt, QPolygon &pl)
{
	int rad = CORNER_CIRCLE_RAD;
	pt->setPen(Qt::black);
//	pt->setPen(Qt::PenStyle::SolidLine);

	for(int i = 0; i < pl.count(); i++)
	{
		pt->drawEllipse(pl[i], rad, rad);
	}
}


void SimpleLabel::paintEvent (QPaintEvent*)
{
	QPainter pt(this);
	int fr = mMonitor->getCurrentFrameNumber();
	if(mDisplayImage)
	{	
		pt.drawImage(WINDOW_OFFSET_X, WINDOW_OFFSET_Y, *mDisplayImage);
	}

	if(ui.chkBoxShowAllLabels->isChecked())
	{
		QBrush br(QColor(255,255,255, 100));
		int row = ui.listLabels->currentRow();
		int n, k;
		QRect rc;

		for(n = 0; n < mLabels.count(); n++)
		{
			if(row == n)
				br.setColor(QColor(0,255,0, 100));
			else
				br.setColor(QColor(255,255,255, 100));

			if(mLabels[n].boxes.count() > 0 && 
				fr >= mLabels[n].boxes.first().frame &&
				fr <= mLabels[n].boxes.last().frame)
			{
				k = fr - mLabels[n].boxes.first().frame;
				rc = imageToScreen(mLabels[n].boxes[k].rc);

				pt.fillRect(rc, br);
				pt.setPen( Qt::black );
				pt.drawRect(rc);
			}
		}
	}
	else
	{
		switch(mShapeMode)
		{
		case Rect:
			drawModeRect(&pt, fr);
			break;
		case Polyg:
			drawModePoly(&pt, fr);
			break;
		default:
			break;
		}
	}
}

void SimpleLabel::drawModeRect(QPainter *pt, int frame)
{
	if(mDrawRect != CommonFunctions::NULL_RECT)
	{
		QBrush br(QColor(255,255,255, 100));
		int row = ui.listLabels->currentRow();
		if(row >= 0)
		{
			if(mLabels[row].viaPoints.count() > 0 &&
				(frame < mLabels[row].viaPoints.first().frame || frame > mLabels[row].viaPoints.last().frame))
			{
				br.setColor(QColor(255,0,0, 100));
			}
			else
			{
				for(int i = 0; i < mLabels[row].viaPoints.count(); i++)
				{
					if(mLabels[row].viaPoints[i].frame == frame)
					{
						br.setColor(QColor(0,255,0, 100));
						break;
					}
				}
			}
		}

		QRect drc = imageToScreen(mDrawRect.rc);
		//rotation
		pt->translate(drc.center());
		pt->rotate(-mDrawRect.angle);
		pt->translate(-drc.center());

		pt->fillRect(drc, br);
		pt->setPen( Qt::black );
		pt->drawRect(drc);

		drawCirclesAtVertices(pt, drc);
		pt->rotate(mDrawRect.angle);
	}
}

//QRect SimpleLabel::rotateRect(QRect rc, float a)
//{
//	qDebug() << "Original rect " << rc;
//	QRect tmp;
//	QTransform tr;
//	tr.translate(rc.top(), rc.left());
//	tmp = tr.mapRect(rc);
//	qDebug() << "Translated rect " << tmp;
//	rc.translate(10,10);
//	tr.rotate(30);
//	tmp = tr.mapRect(tmp);
//	qDebug() << "Translated and rotated rect " << tmp;
//
//	return tmp;
//}

void SimpleLabel::drawModePoly(QPainter *pt, int frame)
{
	int row = ui.listLabels->currentRow();
	if(mNewPolygon)
	{
		if(row >= 0)
		{
			QPolygon p = imageToScreen(mFirstPolygon);
			pt->drawPolyline(p);
			drawCirclesAtVertices(pt, p);
		}
	}
	else if(mDrawPolygon != CommonFunctions::NULL_POLYGON)
	{
		QBrush br(QColor(255,255,255, 100));
		if(row >= 0)
		{
			if(mLabels[row].viaPointsPoly.count() > 0 &&
				(frame < mLabels[row].viaPointsPoly.first().frame || frame > mLabels[row].viaPointsPoly.last().frame))
			{
				br.setColor(QColor(255,0,0, 100));
			}
			else
			{
				for(int i = 0; i < mLabels[row].viaPointsPoly.count(); i++)
				{
					if(mLabels[row].viaPointsPoly[i].frame == frame)
					{
						br.setColor(QColor(0,255,0, 100));
						break;
					}
				}
			}
		}

		QPolygon dpl = imageToScreen(mDrawPolygon);
		pt->setBrush(br);
		pt->setPen( Qt::black );
		pt->drawPolygon(dpl);
		
//		pt->drawRect(drc);

		drawCirclesAtVertices(pt, dpl);
	}
}


void SimpleLabel::on_chkBoxShowAllLabels_toggled(bool b)
{
	ui.btnAddLabel->setDisabled(b);
	update();
}

void SimpleLabel::on_hSliderFrames_valueChanged(int v)
{
	ui.lblCurrentFrame->setText(QString::number(v));

	if(mMonitor->isRunning())
	{
		if(mShapeMode == Rect)
			setDrawRectToFrame(v);
		else if(mShapeMode == Polyg)
			setDrawPolygonToFrame(v);
	}
	else
	{
		mMonitor->moveToFrame(v);
		if(mShapeMode == Rect)
			setDrawRectToFrame(v);
		else if(mShapeMode == Polyg)
			setDrawPolygonToFrame(v);
		showImage();
	}
}

void SimpleLabel::setDrawRectToFrame(int v)
{
	int row = ui.listLabels->currentRow();
	if(row >= 0)
	{
		if(mLabels[row].boxes.count() > 0)
		{
			int fr = v - mLabels[row].viaPoints[0].frame;
			if(fr >= 0 && mLabels[row].boxes.count() > fr)
			{
				if(mLabels[row].boxes[fr].frame != v)
				{
					QString s = QString("Frames are out of sync: box = ") + mLabels[row].boxes[fr].frame + "; v = " + v;
					qDebug() << s;
				}
				//mDrawRect.rc = mLabels[row].boxes[fr].rc;
				mDrawRect = mLabels[row].boxes[fr];
			}
		}
	}
}

void SimpleLabel::setDrawPolygonToFrame(int v)
{
	int row = ui.listLabels->currentRow();
	if(row >= 0)
	{
		if(mLabels[row].polygons.count() > 0)
		{
			int fr = v - mLabels[row].viaPointsPoly[0].frame;
			if(fr >= 0 && mLabels[row].polygons.count() > fr)
			{
				if(mLabels[row].polygons[fr].frame != v)
				{
					QString s = QString("Frames are out of sync: polygon = ") + mLabels[row].polygons[fr].frame + "; v = " + v;
					qDebug() << s;
				}
				mDrawPolygon = mLabels[row].polygons[fr].pl;
			}
		}
	}
}

void SimpleLabel::mousePressEvent( QMouseEvent * e )
{
	QPoint pt = screenToImage(e->pos());

	if(e->button() == Qt::RightButton)
	{
		int row = ui.listLabels->currentRow();
		int vertex = CommonFunctions::checkForVertices(pt, mDrawPolygon);
		if(vertex >= 0 && row >= 0)
		{
			QAction *tmp = mPopupMenu->exec(QCursor::pos());
			if(tmp == NULL)
			{
			}
			else if(tmp->objectName() == "AddVertex")
			{
				QPoint p1, p2, p;
				int v2 = (vertex + 1)%mLabels[row].viaPointsPoly[0].pl.count();

				//add new vertex to each via point and recalculate polygons
				QList<ViaPointPolygon>::iterator i;
				for(i = mLabels[row].viaPointsPoly.begin(); i != mLabels[row].viaPointsPoly.end(); ++i)
				{
					p1 = i->pl.point(vertex);
					p2 = i->pl.point(v2);
					p = (p1 + p2)/2;

					i->pl.insert(v2, p);
				}
				rebuildLabelPolygons();
				mDrawPolygon = mLabels[row].findPolygonByFrame(mMonitor->getCurrentFrameNumber())->pl;
				update();
			}
			else if(tmp->objectName() == "RemoveVertex")
			{
				//add new vertex to each via point and recalculate polygons
				QList<ViaPointPolygon>::iterator i;
				for(i = mLabels[row].viaPointsPoly.begin(); i != mLabels[row].viaPointsPoly.end(); ++i)
				{
					i->pl.remove(vertex);
				}
				rebuildLabelPolygons();
				mDrawPolygon = mLabels[row].findPolygonByFrame(mMonitor->getCurrentFrameNumber())->pl;
				update();
			}
		}
	}
	else if(e->button() == Qt::LeftButton &&  mMonitor->isInitialized() && !ui.chkBoxShowAllLabels->isChecked())
	{
		//check if mouse was clicked inside the image
		if(mDisplayImage->rect().contains(pt))
		{
			switch(mShapeMode)
			{
			case Rect:
				//check if we should resize
				//mResizeVertex = CommonFunctions::checkForVertices(pt, mDrawRect.rc);
				mResizeVertex = CommonFunctions::checkForVerticesWithRotation(pt, mDrawRect);
				if( mResizeVertex >= 0)
				{
					qDebug() << "Found vertex " << mResizeVertex;
					pt = CommonFunctions::getOpposingVertex(mDrawRect.rc, mResizeVertex);

					//bring pt to screen frame of reference by rotating it in opposite direction
					pt = CommonFunctions::rotatePointAboutPoint(pt, -mDrawRect.angle, mDrawRect.rc.center());
					qDebug() << "Rect: " << mDrawRect.rc;
					qDebug() << "Opposing vertex: " << pt << "\n";
				}
				//check if we need to rotate the rectangle
				else if(CommonFunctions::checkForRotation(pt, mDrawRect))
				{
					mRotateRect = true;
				}
				//ckeck if the click is withing the rectangle
				else if(mDrawRect.rc.contains(CommonFunctions::rotatePointAboutPoint(pt, mDrawRect.angle, mDrawRect.rc.center())))
				{
					mMoveRect = true;
				}
				break;
			case Polyg:
				mResizeVertex = CommonFunctions::checkForVertices(pt, mDrawPolygon);
				//drawing new polygon
				if(mNewPolygon)
				{
					int row = ui.listLabels->currentRow();
					if(row >= 0)
					{

						mFirstPolygon << pt;
					}
					update();
				}
				//resizing
				else if(mResizeVertex >= 0)
				{	//this is intentional
				}
				//moving
				else if(mDrawPolygon.containsPoint(pt, Qt::OddEvenFill))
				{
					mMovePolygon = true;
				}
				break;
			default:
				break;
			}
			mDrawPoint = pt;
		}
	}
}


void SimpleLabel::mouseReleaseEvent( QMouseEvent* )
{
	if(mMonitor->isInitialized() && !ui.chkBoxShowAllLabels->isChecked())
	{
		mDrawPoint = CommonFunctions::NULL_POINT;
		mMoveRect = false;
		mRotateRect = false;
		mMovePolygon = false;
		mResizeVertex = -1;
		
		//reculculate boxes if something changed
		if(mSomethingChanged)
		{
			if(mShapeMode == Rect)
			{
				addViaPoint(ui.hSliderFrames->value(), mDrawRect);
			}
			else if(mShapeMode == Polyg)
			{
				addViaPointPoly(ui.hSliderFrames->value(), mDrawPolygon);
			}
			mSomethingChanged = false;
			update();
		}
	}
}

void SimpleLabel::mouseMoveEvent( QMouseEvent * e )
{
	//working in image coordinates
	QPoint pt = screenToImage(e->pos());

	//changing mouse cursor
	if(mMonitor->isInitialized())
	{
		if(mDisplayImage->rect().contains(pt))
		{
			if(this->cursor().shape() != Qt::CrossCursor)
			{
				this->setCursor(*mCrossHairCursor);
			}
		}
		else
		{
			if(this->cursor().shape() != Qt::ArrowCursor)
			{
				QCursor cr(Qt::ArrowCursor);
				this->setCursor(cr);
			}
		}
	}
	
	if(e->buttons() & Qt::LeftButton &&
		mMonitor->isInitialized() && 
		mDrawPoint != CommonFunctions::NULL_POINT &&
		!ui.chkBoxShowAllLabels->isChecked() &&
		!mNewPolygon)
	{
		switch(mShapeMode)
		{
		case Rect:
			//move existing rectangle
			if(mMoveRect)
			{
				QPoint dp = pt - mDrawPoint;

				mDrawRect.rc.translate(dp);
				mDrawPoint = pt;
			}
			//resize the shape
			else if(mResizeVertex >= 0)
			{
				//original
				/*mDrawRect.rc.setCoords(mDrawPoint.x(), mDrawPoint.y(), pt.x(), pt.y());
				mDrawRect.rc = mDrawRect.rc.normalized();*/

				//mouse position in rect frame
				QPoint tpt = CommonFunctions::rotatePointAboutPoint(pt, mDrawRect.angle, mDrawRect.rc.center());
				// reference point that should alwasy remain in the same point on the image
				QPoint ref_pt = CommonFunctions::rotatePointAboutPoint(mDrawPoint, mDrawRect.angle, mDrawRect.rc.center());
				//true size of the new rect
				QSize sz(tpt.x() - ref_pt.x(), tpt.y() - ref_pt.y());
				
				//not rotated rect in screen coords rect
				QRect tmp(mDrawPoint, sz);
				tmp = tmp.normalized();

				//rotate center of this rect about the ref point by -angle 
				//to find the proper position of the new center
				QPoint new_center = CommonFunctions::rotatePointAboutPoint(tmp.center(), -mDrawRect.angle, mDrawPoint);
				//translate rect so that it's center is the new position
				tmp.translate(new_center - tmp.center());

				mDrawRect.rc = tmp;

			}
			//rotate rectangle
			else if(mRotateRect)
			{
				QPoint o = mDrawRect.rc.center();
				QPoint v1 = pt - o;
				QPoint v2 = mDrawPoint - o;
				int row = ui.listLabels->currentRow();
				double ang = CommonFunctions::findAngleBetweenVectors2(v1, v2);
				if(row >= 0)
				{
					mDrawRect.angle = mLabels[row].findBoxByFrame(mDrawRect.frame)->angle + ang;
				}
			}
			//draw new rectangle
			else
			{
				mDrawRect.rc.setCoords(mDrawPoint.x(), mDrawPoint.y(), pt.x(), pt.y());
				mDrawRect.rc = mDrawRect.rc.normalized();
				mDrawRect.angle = 0;
				mDrawRect.frame = ui.lblCurrentFrame->text().toInt();
			}

			mDrawRect.rc = mDisplayImage->rect().intersect(mDrawRect.rc);
			mSomethingChanged = true;
			break;
		case Polyg:
			//move the polygon
			if(mMovePolygon)
			{
				QPoint dp = pt - mDrawPoint;
				mDrawPolygon.translate(dp);
				mDrawPoint = pt;
				mSomethingChanged = true;
			}
			else if(mResizeVertex >= 0)
			{
				mDrawPolygon.setPoint(mResizeVertex, pt);
				mSomethingChanged = true;
			}
			break;
		default:
			break;
		}

		repaint();
	}
}

QPoint SimpleLabel::screenToImage(QPoint scr)
{
	return QPoint(scr.x() - WINDOW_OFFSET_X, scr.y() - WINDOW_OFFSET_Y);
}


QPoint SimpleLabel::imageToScreen(QPoint im)
{
	return QPoint(im.x() + WINDOW_OFFSET_X, im.y() + WINDOW_OFFSET_Y);
}
	
QPoint SimpleLabel::imageToImage(int srcW, int srcH, QPoint srcP, int destW, int destH)
{
	QPoint res;
	double kX, kY;

	kX = (double)destW/(double)srcW;
	kY = (double)destH/(double)srcH;

	res.setX(ROUND(srcP.x() * kX));
	res.setY(ROUND(srcP.y() * kY));

	return res;
}

QRect SimpleLabel::imageToImage(int srcW, int srcH, QRect srcP, int destW, int destH)
{
	QRect res;
	QPoint tl = imageToImage(srcW, srcH, srcP.topLeft(), destW, destH);
	QPoint br = imageToImage(srcW, srcH, srcP.bottomRight(), destW, destH);

	res.setTopLeft(tl);
	res.setBottomRight(br);

	return res;
}


QRect SimpleLabel::screenToImage(QRect scr)
{
	return scr.translated(-WINDOW_OFFSET_X, -WINDOW_OFFSET_Y);
}


QRect SimpleLabel::imageToScreen(QRect im)
{
	return im.translated(WINDOW_OFFSET_X, WINDOW_OFFSET_Y);
}

QPolygon SimpleLabel::screenToImage(QPolygon scr)
{
	return scr.translated(-WINDOW_OFFSET_X, -WINDOW_OFFSET_Y);
}

QPolygon SimpleLabel::imageToScreen(QPolygon im)
{
	return im.translated(WINDOW_OFFSET_X, WINDOW_OFFSET_Y);
}

QPolygon SimpleLabel::imageToImage(int srcW, int srcH, QPolygon srcP, int destW, int destH)
{
	QPolygon res;
	QPoint p;
	for(int i = 0; i < srcP.count(); i++)
	{
		p = imageToImage(srcW, srcH, srcP[i], destW, destH);
		res << p;
	}

	return res;
}

void SimpleLabel::on_btnAddLabel_pressed()
{
	Label lb;
	lb.number = mLabels.count();
	lb.name = "New Label";
	lb.shape = Polyg;
	lb.desc = "";
	mLabels.append(lb);

	updateListView();
}

void SimpleLabel::on_btnDeleteLabel_pressed()
{
	int row = ui.listLabels->currentRow();

	if(row >= 0 && row < mLabels.count())
	{
		mLabels.removeAt(row);
	}

	updateListView();
}

void SimpleLabel::updateListView()
{
	int i;
	ui.listLabels->clear();

	for(i = 0; i < mLabels.count(); i++)
	{
		QListWidgetItem *it = new QListWidgetItem();
		it->setText(mLabels[i].name);
		it->setFlags(it->flags() | Qt::ItemIsEditable);
		ui.listLabels->addItem(it);
	}
}

void SimpleLabel::on_listLabels_itemChanged(QListWidgetItem* item)
{
	int row = ui.listLabels->row(item);

	mLabels[row].name = item->text();
}

void SimpleLabel::on_listLabels_currentItemChanged (QListWidgetItem * current, QListWidgetItem * previous)
{
	int row = ui.listLabels->row(previous);
	
	if(row >= 0 && row < mLabels.count())
	{
		mLabels[row].desc = ui.textEditDescription->toPlainText();
		mLabels[row].shape = mShapeMode;
	}

	row = ui.listLabels->row(current);
	if(row >= 0 && row < mLabels.count())
	{
		mShapeMode = mLabels[row].shape;
		ui.textEditDescription->setText(mLabels[row].desc);
		ui.cmbBoxLabelShape->setCurrentIndex(mShapeMode);
		switch(mShapeMode)
		{
		case Rect:
			if(mLabels[row].boxes.count() > 0)
			{
				ViaPoint *p = mLabels[row].findBoxByFrame(mMonitor->getCurrentFrameNumber());
				if(p != NULL)
					mDrawRect = *p;
			}
			break;
		case Polyg:
			 if(mLabels[row].polygons.count() > 0)
			 {
				ViaPointPolygon *p = mLabels[row].findPolygonByFrame(mMonitor->getCurrentFrameNumber());
				if(p != NULL)
					mDrawPolygon = p->pl;
			 }
			break;
		default:
			break;
		}
	}
	update();
}

void SimpleLabel::on_textEditDescription_textChanged()
{
	int row = ui.listLabels->currentRow();
	if(row >= 0)
	{
		mLabels[row].desc = ui.textEditDescription->toPlainText();
	}
}

//add new polygon via point to the structure
void SimpleLabel::addViaPointPoly(int frame, QPolygon pl, bool addRect)
{
	bool ok = true;
	int row = ui.listLabels->currentRow();
	if(row < 0)
	{
		QMessageBox::information(NULL,"Label", "Select a label.");
		ok = false;
	}

	if(ok)
	{	
		QList<ViaPointPolygon>::iterator i;
		//find the place where to put new polygon
		for(i = mLabels[row].viaPointsPoly.begin(); i != mLabels[row].viaPointsPoly.end(); i++)
		{
			if(i->frame > frame || i->frame == frame)
			{
				break;
			}
		}

		//if the frame is already a via point replace it
		if(i != mLabels[row].viaPointsPoly.end() && i->frame == frame)
		{
			i->frame = frame;
			i->pl = pl;
		}
		else
		{
			//insert new via point
			ViaPointPolygon v;
			v.frame = frame;
			v.pl = pl;
			mLabels[row].viaPointsPoly.insert(i, v);
		}

		rebuildLabelPolygons();

		//last parameter is false bacause we don't want to add a polygon
		//(we just added it)
		if(addRect)
			addViaPoint(frame, pl.boundingRect());
	}
}

//add new point in the right place in the label structure
void SimpleLabel::addViaPoint(int frame, QRect rc)
{
	bool ok = true;
	int row = ui.listLabels->currentRow();
	if(row < 0)
	{
		QMessageBox::information(NULL,"Label", "Select a label.");
		ok = false;
	}

	if(ok)
	{	
		QList<ViaPoint>::iterator i;
		//find the place where to put new point
		for(i = mLabels[row].viaPoints.begin(); i != mLabels[row].viaPoints.end(); i++)
		{
			if(i->frame > frame || i->frame == frame)
			{
				break;
			}
		}

		ViaPoint v;
		v.frame = frame;
		v.rc = rc;
		//if the frame is already a via point replace it
		if(i != mLabels[row].viaPoints.end() && i->frame == frame)
		{
			i->frame = v.frame;
			i->rc = v.rc;
		}
		else
		{
			//insert new via point
			mLabels[row].viaPoints.insert(i, v);
		}
		rebuildLabelBoxes();
	}
}

//add new point in the right place in the label structure
void SimpleLabel::addViaPoint(int frame, ViaPoint v)
{
	bool ok = true;
	int row = ui.listLabels->currentRow();
	if(row < 0)
	{
		QMessageBox::information(NULL,"Label", "Select a label.");
		ok = false;
	}

	if(ok)
	{	
		QList<ViaPoint>::iterator i;
		//find the place where to put new point
		for(i = mLabels[row].viaPoints.begin(); i != mLabels[row].viaPoints.end(); i++)
		{
			if(i->frame > frame || i->frame == frame)
			{
				break;
			}
		}

		//if the frame is already a via point replace it
		if(i != mLabels[row].viaPoints.end() && i->frame == frame)
		{
			*i = v;
			i->frame = frame;
		}
		else
		{
			//insert new via point
			ViaPoint nv = v;
			nv.frame = frame;
			mLabels[row].viaPoints.insert(i, nv);
		}
		rebuildLabelBoxes();

		//add polygon viaPoint too
		int numVerticies = 4;
		if( mLabels[row].viaPointsPoly.count() > 0)
		{
			numVerticies = mLabels[row].viaPointsPoly[0].pl.count();
		}

		QPolygon newPolyg;
		newPolyg << v.rc.topLeft();
		newPolyg << v.rc.topRight();
		newPolyg << v.rc.bottomRight();

		for(int ver = 3; ver < numVerticies; ver++)
		{
			newPolyg << v.rc.bottomLeft();
		}

		//we don't want to add new rectangle 
		//we just added it
		addViaPointPoly(v.frame, newPolyg, false);
		
	}
}

//recalculate the label polygons
void SimpleLabel::rebuildLabelPolygons()
{
	int row = ui.listLabels->currentRow();
	double df = 0.0;
	QPointF dp;
	ViaPointPolygon b1, b2;
	ViaPointPolygon newPolyg;
	QPoint np;
	int t;

	if(row >= 0)
	{
		QList<ViaPointPolygon>::iterator i;
		mLabels[row].polygons.clear();


		if(mLabels[row].viaPointsPoly.count() == 1)
		{
			mLabels[row].polygons << mLabels[row].viaPointsPoly[0];
		}

		for(i = mLabels[row].viaPointsPoly.begin(); i != mLabels[row].viaPointsPoly.end() - 1; ++i)
		{
			b1 = *i;
			b2 = *(i + 1);
			if(mIntrMethod == LinearIntr)
			{
				df = b2.frame - b1.frame;
			}

			for(int k = b1.frame; k <= b2.frame; k++)
			{
				t = k - b1.frame;
				newPolyg.frame = k;
				newPolyg.pl = QPolygon();
				
				for(int j = 0; j < b1.pl.count(); j++)
				{
					if(mIntrMethod == LinearIntr)
					{
						QPointF p1 = b2.pl.point(j) - b1.pl.point(j);
						dp = p1/df;
						np = linearInterpolation(b1.pl.point(j), dp, t);
					}
					newPolyg.pl << np;
				}

				//the first frame of the new segment is the same as the last frame of the 
				//previous segment, so we rewrite it
				if(k == b1.frame && mLabels[row].polygons.count() > 0)
				{
					mLabels[row].polygons[mLabels[row].polygons.count() - 1] = newPolyg;
				}
				else
				{
					mLabels[row].polygons << newPolyg;
				}
			}
		}
	}
}


//recalculate the label boxes
void SimpleLabel::rebuildLabelBoxes()
{
	int row = ui.listLabels->currentRow();

	if(row >= 0)
	{
		QList<ViaPoint>::iterator i;
		mLabels[row].boxes.clear();
		ViaPoint b1, b2;
		ViaPoint newbox;
		double dx = 0, dy = 0;
		double dw = 0, dh = 0;
		double df;
                double da = 0;
		int t;

		//special case
		if(mLabels[row].viaPoints.count() == 1)
		{
			mLabels[row].boxes << mLabels[row].viaPoints[0];
		}

		for(i = mLabels[row].viaPoints.begin(); i != mLabels[row].viaPoints.end() - 1; ++i)
		{
			b1 = *i;
			b2 = *(i+1);
			
			if(mIntrMethod == LinearIntr)
			{
				df = b2.frame - b1.frame;
				dx = (b2.rc.left() - b1.rc.left())/df;
				dy = (b2.rc.top() - b1.rc.top())/df;
				dw = (b2.rc.width() - b1.rc.width())/df;
				dh = (b2.rc.height() - b1.rc.height())/df;
				da = (b2.angle - b1.angle)/df;
			}
			for(int k = b1.frame; k <= b2.frame; k++)
			{
				t = k - b1.frame;
				if(mIntrMethod == LinearIntr)
				{
					newbox.rc = linearInterpolation(b1.rc, dx, dy, dw, dh, t);
					newbox.frame = k;
					newbox.angle = b1.angle + da*t;
				}
				else if(mIntrMethod == Motion)
				{
					//DO THE MOTION ALGORITHM HERE
				}

				//the first frame of the new segment is the same as the last frame of the 
				//previous segment, so we rewrite it
				if(k == b1.frame && mLabels[row].boxes.count() > 0)
				{
					mLabels[row].boxes[mLabels[row].boxes.count() - 1] = newbox;
				}
				else
				{
					mLabels[row].boxes.append(newbox);
				}
			}
		}
	}
}

QPoint SimpleLabel::linearInterpolation(QPoint src, QPointF dt, int t)
{
	return src + (t*dt).toPoint();
}

QRect SimpleLabel::linearInterpolation(QRect initPoint, double dx, double dy, double dw, double dh, int t)
{
	QRect res;
	res.setLeft(ROUND(initPoint.left() + t*dx));
	res.setTop(ROUND(initPoint.top() + t*dy));
	res.setWidth(ROUND(initPoint.width() + t*dw));
	res.setHeight(ROUND(initPoint.height() + t*dh));

	return res;
}

void SimpleLabel::on_btnToBeginning_pressed()
{
	int row = ui.listLabels->currentRow();
	if(mShapeMode == Rect)
	{
		if( row >= 0 && mLabels[row].viaPoints.count() > 0)
		{
			ui.hSliderFrames->setValue(mLabels[row].viaPoints[0].frame);
		}
	}
	else if(mShapeMode == Polyg)
	{
		if( row >= 0 && mLabels[row].viaPointsPoly.count() > 0)
		{
			ui.hSliderFrames->setValue(mLabels[row].viaPointsPoly[0].frame);
		}
	}
}

void SimpleLabel::on_btnPrevViaPoint_pressed()
{
	int row = ui.listLabels->currentRow();
	if(mShapeMode == Rect)
	{
		if( row >= 0 && mLabels[row].viaPoints.count() > 0)
		{
			int fr = ui.hSliderFrames->value();
			int i = mLabels[row].viaPoints.count() - 1;
			while(mLabels[row].viaPoints[i].frame >= fr && i > 0)
			{
				i--;
			}

			ui.hSliderFrames->setValue(mLabels[row].viaPoints[i].frame);
		}
	}
	else if(mShapeMode == Polyg)
	{
		if( row >= 0 && mLabels[row].viaPointsPoly.count() > 0)
		{
			int fr = ui.hSliderFrames->value();
			int i = mLabels[row].viaPointsPoly.count() - 1;
			while(mLabels[row].viaPointsPoly[i].frame >= fr && i > 0)
			{
				i--;
			}

			ui.hSliderFrames->setValue(mLabels[row].viaPointsPoly[i].frame);
		}
	}
}

void SimpleLabel::on_btnStop_pressed()
{
	mMonitor->stop();
}

void SimpleLabel::on_btnRemoveViaPoint_pressed()
{
	int row = ui.listLabels->currentRow();
	if(mShapeMode == Rect)
	{
		if( row >= 0 && mLabels[row].viaPoints.count() > 0)
		{
			int fr = ui.hSliderFrames->value();
			for(int i = 0; i < mLabels[row].viaPoints.count(); i++)
			{
				if(mLabels[row].viaPoints[i].frame == fr)
				{
					mLabels[row].viaPoints.removeAt(i);
					if(mLabels[row].viaPoints.count() > 0)
					{
						rebuildLabelBoxes();
					}
					break;
				}
			}
		}
	}
	else if(mShapeMode == Polyg)
	{
		if( row >= 0 && mLabels[row].viaPointsPoly.count() > 0)
		{
			int fr = ui.hSliderFrames->value();
			for(int i = 0; i < mLabels[row].viaPointsPoly.count(); i++)
			{
				if(mLabels[row].viaPointsPoly[i].frame == fr)
				{
					mLabels[row].viaPointsPoly.removeAt(i);
					if(mLabels[row].viaPointsPoly.count() > 0)
					{
						rebuildLabelPolygons();
					}
					break;
				}
			}
		}
	}
}

void SimpleLabel::on_btnPlay_pressed()
{
	mMonitor->start();
}


void SimpleLabel::on_btnNextViaPoint_pressed()
{
	int row = ui.listLabels->currentRow();
	if(mShapeMode == Rect)
	{
		if( row >= 0 && mLabels[row].viaPoints.count() > 0)
		{
			int fr = ui.hSliderFrames->value();
			int i = 0;
			while(mLabels[row].viaPoints[i].frame <= fr && i < mLabels[row].viaPoints.count() - 1)
			{
				i++;
			}

			ui.hSliderFrames->setValue(mLabels[row].viaPoints[i].frame);
		}
	}
	else if(mShapeMode == Polyg)
	{
		if( row >= 0 && mLabels[row].viaPointsPoly.count() > 0)
		{
			int fr = ui.hSliderFrames->value();
			int i = 0;
			while(mLabels[row].viaPointsPoly[i].frame <= fr && i < mLabels[row].viaPointsPoly.count() - 1)
			{
				i++;
			}

			ui.hSliderFrames->setValue(mLabels[row].viaPointsPoly[i].frame);
		}
	}
}

void SimpleLabel::on_btnToEnd_pressed()
{
	int row = ui.listLabels->currentRow();
	if(mShapeMode == Rect)
	{
		if( row >= 0 && mLabels[row].viaPoints.count() > 0)
		{
			ui.hSliderFrames->setValue(mLabels[row].viaPoints.last().frame);
		}
	}
	else if(mShapeMode == Polyg)
	{
		if( row >= 0 && mLabels[row].viaPointsPoly.count() > 0)
		{
			ui.hSliderFrames->setValue(mLabels[row].viaPointsPoly.last().frame);
		}
	}
}

void SimpleLabel::exportToMovie(bool origBkgrd)
{
	int nLbls = ui.listLabels->count();
	int startF, endF;
	int i, t;
	QSize origSz = mMonitor->getImageSize();

	if(nLbls > 0)
	{
		//find the earliest frame in all the labels
		startF = qMax( mSaveDgl->ui.edtFirstImageIndex->text().toInt(), mFirstFrameNumber);
		endF = qMin(mSaveDgl->ui.edtLastImageIndex->text().toInt(), mFirstFrameNumber + mMonitor->getFrameCount() - 1);
		if(startF > mFirstFrameNumber + mMonitor->getFrameCount() - 1)
		{
			QMessageBox::information(this, "Index is out of bounds", "Starting frame number is larger than image sequence length.");
			return;
		}

		CvVideoWriter* aviW;
		QString saveFile = mSaveDgl->ui.edtSavePath->text();
		QString path, prefix, fname, ext;
		int index;

		CommonFunctions::splitPath(saveFile, path, fname, prefix, (uint)5, index, ext);

		if(mSaveDgl->ui.rbtnSaveAsAVI->isChecked())
		{
			int fps = mMonitor->getFPS();
			if(fps < 1)
				fps = 30;

			try
			{
				//CV_FOURCC('M', 'P', '4', '2') CV_FOURCC('M','J','P','G')
				aviW = cvCreateVideoWriter(saveFile.toAscii(), CV_FOURCC_PROMPT, fps, cvSize(origSz.width(), origSz.height()));
			//		CvVideoWriter* aviW = cvCreateAVIWriter((mFileName.left(mFileName.length()-4) + "_bm.avi").toAscii(), 0, fps, cvSize(origSz.width(), origSz.height()));
			}
			catch(...)
			{
				aviW = cvCreateVideoWriter(saveFile.toAscii(), CV_FOURCC_DEFAULT, fps, cvSize(origSz.width(), origSz.height()));
			}
		}

		QPainter pt;
		int h;
		QColor cl;
		QBrush br(QColor(0,0,0,255));
		QImage *im = NULL;
		if(!origBkgrd)
		{
			im = new QImage(origSz, QImage::Format_ARGB32);
		}
		ViaPoint *bx;
		ViaPointPolygon *polyg;
		IplImage* ipl = cvCreateImage(cvSize(origSz.width(), origSz.height()), IPL_DEPTH_8U, 3);
		for(t = startF; t <= endF; t++)
		{
			if(origBkgrd)
			{
				mMonitor->moveToFrame(t);
				im = mMonitor->getImage();
			}

			pt.begin(im);
			if(!origBkgrd)
			{
				pt.fillRect(im->rect(), Qt::black);
			}
			for(i = 0; i < nLbls; i++)
			{
				if(mLabels[i].shape == Rect)
				{
					if(t >= mLabels[i].boxes[0].frame &&
						t <= mLabels[i].boxes[mLabels[i].boxes.count() - 1].frame)
					{
						h = 200;//(int)(100 + (i + 1) * 155.0/nLbls);
						cl = QColor::fromRgb(h,h,h,128);
						br.setColor(cl);
						bx = mLabels[i].findBoxByFrame(t);
						QRect rt = imageToImage(mDisplayImage->width(), mDisplayImage->height(), bx->rc, origSz.width(), origSz.height());
						pt.fillRect(rt, br);
					}
				}
				else if(mLabels[i].shape == Polyg)
				{
					if(t >= mLabels[i].polygons[0].frame &&
						t <= mLabels[i].polygons[mLabels[i].polygons.count() - 1].frame)
					{
						h = 200;//(int)(100 + (i + 1) * 155.0/nLbls);
						cl = QColor::fromRgb(h,h,h,128);
						br.setColor(cl);
						pt.setBrush(br);
						polyg = mLabels[i].findPolygonByFrame(t);
						QPolygon pl = imageToImage(mDisplayImage->width(), mDisplayImage->height(), polyg->pl, origSz.width(), origSz.height());
						pt.drawPolygon(pl);
					}
				}
			}

			if(mSaveDgl->ui.rbtnSaveAsAVI->isChecked())
			{
				mMonitor->convertARGB2RGB(im, ipl);
				cvWriteFrame(aviW, ipl);
			}
			else
			{
				QString sv;
				sv = path + prefix + sv.sprintf("%05d", t) + ext;
				im->save(sv, "PNG");
			}

			pt.end();
		}
		if(mSaveDgl->ui.rbtnSaveAsAVI->isChecked())
		{
			cvReleaseVideoWriter(&aviW);
		}

		if(!origBkgrd)
		{
			delete im;
		}
	}
}


void SimpleLabel::exportToMatlabStruct()
{
	QSize origSz = mMonitor->getImageSize();
	int i, k, j;
	QString saveFile = mSaveDgl->ui.edtSavePath->text();
	QString filename = mSaveDgl->ui.edtFileNamePrefix->text();

	QFile fd(saveFile);
	if(fd.open(QFile::WriteOnly | QFile::Truncate))
	{
		QTextStream out(&fd);
		out << "function " << filename << " = " << filename + "_lbl" << endl;
		for(i = 0; i < mLabels.count(); i++)
		{
			out << filename << "(" << i +1 << ").number = " << mLabels[i].number << ";" << endl;
			out << filename << "(" << i +1 << ").desc = '" << mLabels[i].desc.replace("\n", "") << "';" << endl;
			out << filename << "(" << i +1 << ").name = '" << mLabels[i].name << "';" << endl;
			out << filename << "(" << i +1 << ").startFrame = " << mLabels[i].viaPoints[0].frame << ";" << endl;
			out << filename << "(" << i +1 << ").endFrame = " << mLabels[i].viaPoints[mLabels[i].viaPoints.count() - 1].frame << ";" << endl;

			out << filename << "(" << i +1 << ").boxes = [";
			for(k = 0; k < mLabels[i].boxes.count(); k++)
			{
				QRect rc = imageToImage(mDisplayImage->width(), mDisplayImage->height(), mLabels[i].boxes[k].rc, origSz.width(), origSz.height());
				out << rc.left()<< "," << rc.top() << "," << rc.width() << "," << rc.height();
				if(k != mLabels[i].boxes.count() - 1)
					out << ";";
			}
			out << "];" << endl;

			for(k = 0; k < mLabels[i].viaPoints.count(); k++)
			{
				QRect rc = imageToImage(mDisplayImage->width(), mDisplayImage->height(), mLabels[i].viaPoints[k].rc, origSz.width(), origSz.height());
					
				out << filename << "(" << i +1 << ").pivots(" << k + 1 << ").frame = " <<  mLabels[i].viaPoints[k].frame << ";" << endl;
				out << filename << "(" << i +1 << ").pivots(" << k + 1 << ").box = [";
				out << rc.left() << "," << rc.top() << "," << rc.width() << "," << rc.height() << "];" << endl;
			}


			
			for(k = 0; k < mLabels[i].polygons.count(); k++)
			{
				out << filename << "(" << i +1 << ").polygons(" << k + 1 << ").polygon=[";
				QPolygon pl = imageToImage(mDisplayImage->width(), mDisplayImage->height(), mLabels[i].polygons[k].pl, origSz.width(), origSz.height());
				for(j = 0; j < pl.count(); j++)
				{
					QPoint pt = pl.point(j);
					out << pt.x() << "," << pt.y();
					if(j != pl.count() - 1)
						out << ";";
				}
				out << "];" << endl;
			}

			for(k = 0; k < mLabels[i].viaPointsPoly.count(); k++)
			{
				QPolygon pl = imageToImage(mDisplayImage->width(), mDisplayImage->height(), mLabels[i].viaPointsPoly[k].pl, origSz.width(), origSz.height());
					
				out << filename << "(" << i +1 << ").pivotsPolyg(" << k + 1 << ").frame = " <<  mLabels[i].viaPoints[k].frame << ";" << endl;
				out << filename << "(" << i +1 << ").pivotsPolyg(" << k + 1 << ").polygon = [";
				for(j = 0; j < pl.count(); j++)
				{
					QPoint pt = pl.point(j);
					out << pt.x() << "," << pt.y();
					if(j != pl.count() - 1)
						out << ";";
				}
			}
		}
		out << "];" << endl;
		fd.close();
	}
}

Label* SimpleLabel::findLabel(int number)
{
	for(int i = 0; i < mLabels.count(); i++)
	{
		if(mLabels[i].number == number)
		{
			return &mLabels[i];
		}
	}

	return NULL;
}



void SimpleLabel::exportToLabelMeXML()
{
	QString saveFile = mSaveDgl->ui.edtSavePath->text();
	QString path, fname, ext;
	CommonFunctions::splitPath(saveFile, path, fname, ext);

	int startF = qMax( mSaveDgl->ui.edtFirstImageIndex->text().toInt(), mFirstFrameNumber);
	int endF = qMin(mSaveDgl->ui.edtLastImageIndex->text().toInt(), mFirstFrameNumber + mMonitor->getFrameCount() - 1);
	if(startF > mFirstFrameNumber + mMonitor->getFrameCount() - 1)
	{
		QMessageBox::information(this, "Index is out of bounds", "Starting frame number is larger than image sequence length.");
		return;
	}

	for(int i = startF; i <= endF; i++)
	{
		exportFrameToLabelMeXML(path, i);
	}
}


void SimpleLabel::on_actionLoad_LabelMe_XML_triggered()
{
	int curframe = -1, totframes = 0;
	int w = 0, h = 0;
	bool err = false;
	QString s = QFileDialog::getOpenFileName(this, tr("Open LabelMe XML"), ".", tr("XML Files (*.xml)"));
	QSize sz = mMonitor->getImageSize();

	if(!s.isEmpty())
	{
		try
		{
			//scanning first file for global settings
			QFile f(s);
			if(f.open(QIODevice::ReadOnly))
			{
				QDomDocument doc("");
				if(doc.setContent(&f))
				{
					resetLabels();

					QDomNode anot = doc.documentElement();

					QDomNode n = anot.firstChild();
					while(!n.isNull())	//going through all the elements
					{
						QDomElement e = n.toElement();
						if(!e.isNull() && e.tagName() == "frame")
						{
							curframe = e.text().trimmed().toInt();
						}
						if(!e.isNull() && e.tagName() == "source")
						{

							QDomNode n1 = n.firstChild();
							while(!n1.isNull())	//going through all the children
							{
								QDomElement e1 = n1.toElement();
								if(!e1.isNull() && e1.tagName() == "numberFrames")
								{
									totframes = e1.text().trimmed().toInt();
								}
								n1 = n1.nextSibling();
							}
						}
						if(!e.isNull() && e.tagName() == "imagesize")
						{
							QDomNode n1 = n.firstChild();
							while(!n1.isNull())	//going through all the children
							{
								QDomElement e1 = n1.toElement();
								if(!e1.isNull() && e1.tagName() == "rows")
								{
									h = e1.text().trimmed().toInt();
								}
								if(!e1.isNull() && e1.tagName() == "columns")
								{
									w = e1.text().trimmed().toInt();
								}
								n1 = n1.nextSibling();
							}
						}

						n = n.nextSibling();
					}
					
				}
				else
				{
					QMessageBox::information(this, "Invalid XML file", "Failed to parse XML file " + s);
				}

				f.close();
			}

			QRect rc;
			int x, y, id;
			QString idstr;
			QString path;
			QString ext;
			QString filename;
			QString prefix;
			int firstframe;
			QProgressBar pBar(statusBar());
			pBar.setRange(curframe, totframes);
			pBar.setVisible(true);

			//splitting the full path into parts
			CommonFunctions::splitPath(s, path, filename, prefix, (uint)5, firstframe, ext);

			while(!err && curframe < totframes && QFile::exists(s))
			{
				pBar.setValue(curframe);
				rc.setCoords(0,0,0,0);
				QFile fd(s);
				if(fd.open(QIODevice::ReadOnly))
				{
					QDomDocument doc("");
					if(doc.setContent(&fd))
					{	
						QDomNode anot = doc.documentElement();

						curframe = anot.namedItem("frame").toElement().text().trimmed().toInt();

						QDomNode n = anot.firstChild();
						while(!n.isNull())	//going through all the elements
						{
							QDomElement e = n.toElement();
							if(!e.isNull() && e.tagName() == "object")
							{
								idstr = e.namedItem("tgtID").toElement().text().trimmed();
								if(idstr == "")
								{
									n = n.nextSibling();
									continue;
								}
								id = idstr.toInt();
								Label *lbl = findLabel(id);
								if(lbl == NULL)
								{
									//create a new label and added it to the list
									//need to retrieve it from the list in order to
									//get a pointer to the to the actual object in the list
									lbl = new Label();
									lbl->number = id;
									lbl->name = e.namedItem("name").toElement().text();
									mLabels.append(*lbl);
									delete lbl;
									lbl = findLabel(id);

									//adding ui item to the list
									QListWidgetItem *it = new QListWidgetItem();
									it->setText(lbl->name);
									it->setFlags(it->flags() | Qt::ItemIsEditable);
									ui.listLabels->addItem(it);
								}

								
								QDomNode bbox = e.namedItem("bbox");
								if(!bbox.isNull())
								{
									QDomNode pt = bbox.namedItem("pt");
									if(!pt.isNull())
									{
										x = qRound(pt.namedItem("x").toElement().text().trimmed().toFloat());
										y = qRound(pt.namedItem("y").toElement().text().trimmed().toFloat());
										rc.setTopLeft(QPoint(x,y));

										pt = pt.nextSibling();
										x = qRound(pt.namedItem("x").toElement().text().trimmed().toFloat());
										y = qRound(pt.namedItem("y").toElement().text().trimmed().toFloat());
										rc.setTopRight(QPoint(x,y));

										pt = pt.nextSibling();
										x = qRound(pt.namedItem("x").toElement().text().trimmed().toFloat());
										y = qRound(pt.namedItem("y").toElement().text().trimmed().toFloat());
										rc.setBottomRight(QPoint(x,y));

										pt = pt.nextSibling();
										x = qRound(pt.namedItem("x").toElement().text().trimmed().toFloat());
										y = qRound(pt.namedItem("y").toElement().text().trimmed().toFloat());
										rc.setBottomLeft(QPoint(x,y));

										rc = imageToImage(w, h, rc, mDisplayImage->width(), mDisplayImage->height());

										ViaPoint vp;
										vp.frame = curframe;
										vp.rc = rc;
										lbl->boxes.append(vp);
										lbl->viaPoints.append(vp);
									}
								}

								QDomNode polygon = e.namedItem("polygon");
								if(!polygon.isNull())
								{
									ViaPointPolygon polyg;
									polyg.frame = curframe;
									QDomNode pt = polygon.namedItem("pt");
									while(!pt.isNull())
									{
										x = qRound(pt.namedItem("x").toElement().text().trimmed().toFloat());
										y = qRound(pt.namedItem("y").toElement().text().trimmed().toFloat());
										
										QPoint pl_pt(x,y);

										pl_pt = imageToImage(w, h, pl_pt, mDisplayImage->width(), mDisplayImage->height());
										polyg.pl << pl_pt;
										pt = pt.nextSibling();
									}
									//only adding polygon if it is at least a triangle
									if(polyg.pl.count() > 2)
									{
										lbl->shape = Polyg;
										lbl->polygons << polyg;
										lbl->viaPointsPoly << polyg;
									}
								}
							}
							n = n.nextSibling();
						}
					}
					else
					{
						QMessageBox::information(this, "Invalid XML file", "Failed to parse XML file " + s);
						err = true;
					}

					fd.close();
				}

				firstframe++;
				QString num;
				num.sprintf("%05d",firstframe);
				s = path + prefix + num + ext;
			}
			pBar.setVisible(false);
		}
		catch(...)
		{
			QMessageBox::information(this, "Unrecoverable error", "Error while processing file: " + s);
		}
	}
}

void SimpleLabel::on_actionLoad_XML_triggered()
{
	int w, h;
	QSize origSz = mMonitor->getImageSize();

	w = origSz.width();
	h = origSz.height();

	QString s = QFileDialog::getOpenFileName(this, tr("Open Label XML"), ".", tr("XML Files (*.xml)"));
	if(!s.isEmpty())
	{
		QFile fd(s);
		if(fd.open(QIODevice::ReadOnly))
		{
			QDomDocument doc("Labels");
			if(doc.setContent(&fd))
			{
				resetLabels();

				QDomNode root = doc.documentElement();
				QDomNode n = root.firstChild();
				while(!n.isNull())	//going through all the labels
				{
					QDomElement e = n.toElement();
					if(!e.isNull() && e.tagName() == "image")
					{
						w = e.attribute("width", "-1").toInt();
						h = e.attribute("height", "-1").toInt();
					}
					else if(!e.isNull() && e.tagName() == "label")
					{
						Label lb;
						lb.name = e.attribute("name", "-1");
						lb.desc = e.attribute("desc", "-1");
						lb.number = e.attribute("number", "-1").toInt();
						lb.shape = (LabelShape)e.attribute("shape", "-1").toInt();
						
						QDomNode t = e.firstChild();
						while(!t.isNull())
						{
							QDomElement el = t.toElement();
							qDebug() << el.tagName();
							if(!el.isNull())
							{
								QDomNode p = el.firstChild();
								while(!p.isNull())
								{
									QDomElement point = p.toElement();
									qDebug() << point.tagName();
									if(!point.isNull())
									{
										if(el.tagName() == "boxes" || el.tagName() == "pivots")
										{
											ViaPoint pt;

											pt.frame = point.attribute("frame", "-1").toInt();
											pt.rc.setLeft(point.attribute("left", "-1").toInt());
											pt.rc.setTop(point.attribute("top", "-1").toInt());
											pt.rc.setRight(point.attribute("right", "-1").toInt());
											pt.rc.setBottom(point.attribute("bottom", "-1").toInt());
											pt.angle = (point.attribute("angle", "0.0").toFloat());
											if(point.tagName() == "box")
											{
												lb.boxes.append(pt);
											}
											else if(point.tagName() == "pivot")
											{
												lb.viaPoints.append(pt);
											}
										}
										else if(el.tagName() == "polygons" || el.tagName() == "polygonPivots")
										{
											ViaPointPolygon pl;
											pl.frame = point.attribute("frame", "-1").toInt();
											
											QDomNode vertex = point.firstChild();
											while(!vertex.isNull())
											{
												QDomElement ver = vertex.toElement();
												QPoint v;
												v.setX(ver.attribute("x", "-1").toInt());
												v.setY(ver.attribute("y", "-1").toInt());

												pl.pl << v;

												vertex = vertex.nextSibling();
											}

											if(point.tagName() == "polygon")
											{
												lb.polygons.append(pl);
											}
											else if(point.tagName() == "polygonPivot")
											{
												lb.viaPointsPoly.append(pl);
											}
										}
									}
									p = p.nextSibling();
								}
							}
							t = t.nextSibling();
						}
						mLabels.append(lb);
						ui.listLabels->addItem(lb.name);
					}
					n = n.nextSibling();
				}

				//converting coordinates to the screen image size
				QList<Label>::iterator i;
				for(i = mLabels.begin(); i != mLabels.end(); i++)
				{
					if(i->shape == Rect)
					{
						QList<ViaPoint>::iterator k;
						for(k = i->boxes.begin(); k != i->boxes.end(); k++)
						{
							k->rc = imageToImage(w, h, k->rc, mDisplayImage->width(), mDisplayImage->height());
						}

						QList<ViaPoint>::iterator j;
						for(j = i->viaPoints.begin(); j != i->viaPoints.end(); j++)
						{
							j->rc = imageToImage(w, h, j->rc, mDisplayImage->width(), mDisplayImage->height());

							ViaPoint* bx = i->findBoxByFrame(j->frame);

	//						qDebug() << "box:" << bx->rc << "   viapoint:" << j->rc;
							if(bx != NULL && bx->rc != j->rc)
								j->rc = bx->rc;
						}
					}
					else if(i->shape == Polyg)
					{
						QList<ViaPointPolygon>::iterator k;
						for(k = i->polygons.begin(); k != i->polygons.end(); k++)
						{
							k->pl = imageToImage(w, h, k->pl, mDisplayImage->width(), mDisplayImage->height());
						}

						QList<ViaPointPolygon>::iterator j;
						for(j = i->viaPointsPoly.begin(); j != i->viaPointsPoly.end(); j++)
						{
							j->pl = imageToImage(w, h, j->pl, mDisplayImage->width(), mDisplayImage->height());

							ViaPointPolygon* pl = i->findPolygonByFrame(j->frame);

							if(pl != NULL && pl->pl != j->pl)
								j->pl = pl->pl;
						}
					}
				}
		}	
			else
			{
				QMessageBox::information(this, "Invalid XML file", "Failed to parse XML file " + s);
			}

			fd.close();
		}
		else
		{
			QMessageBox::information(this, "Open file failed", "Cannot open file " + s);
		}
	}
}

void SimpleLabel::exportFrameToLabelMeXMLWebTool(QString path, int frame)
{
	QSize origSz = mMonitor->getImageSize();

	QDomDocument doc("");
	QDomElement root = doc.createElement("annotation");
	doc.appendChild(root);

	//filename
	QDomElement tmp = doc.createElement("filename");
	QDomText txt = doc.createTextNode(mFileName);
	tmp.appendChild(txt);
	root.appendChild(tmp);

	//folder
	tmp = doc.createElement("folder");
	txt = doc.createTextNode(mPath);
	tmp.appendChild(txt);
	root.appendChild(tmp);

	//frame
	tmp = doc.createElement("frame");
	txt = doc.createTextNode(QString("%1").arg(frame));
	tmp.appendChild(txt);
	root.appendChild(tmp);

	//source
	QDomElement src = doc.createElement("source");
	//sourceImage
	tmp = doc.createElement("sourceImage");
	txt = doc.createTextNode("");
	tmp.appendChild(txt);
	src.appendChild(tmp);
	//sourceAnnotation
	tmp = doc.createElement("sourceAnnotation");
	txt = doc.createTextNode("SimpleLabel");
	tmp.appendChild(txt);
	src.appendChild(tmp);
	//numberFrames
	tmp = doc.createElement("numberFrames");
	txt = doc.createTextNode(QString::number(mMonitor->getFrameCount()));
	tmp.appendChild(txt);
	src.appendChild(tmp);

	root.appendChild(src);

	//imagesize
	src = doc.createElement("imagesize");
	//rows
	tmp = doc.createElement("rows");
	txt = doc.createTextNode(QString::number(origSz.height()));
	tmp.appendChild(txt);
	src.appendChild(tmp);
	//columns
	tmp = doc.createElement("columns");
	txt = doc.createTextNode(QString::number(origSz.width()));
	tmp.appendChild(txt);
	src.appendChild(tmp);
	//channels
	tmp = doc.createElement("channels");
	txt = doc.createTextNode(QString::number(3));
	tmp.appendChild(txt);
	src.appendChild(tmp);
	root.appendChild(src);


	int fr;
	QDomElement obj;
	QList<Label>::iterator lb;
	
	for(lb = mLabels.begin(); lb != mLabels.end(); lb++)
	{
		if(lb->boxes.count() < 1)
			continue;

		QDomElement pt;
		QDomElement x;
		QDomElement y;

		//object
		obj = doc.createElement("object");

		//name
		QDomElement name = doc.createElement("name");
		//deleted
		QDomElement deleted = doc.createElement("deleted");
		//verified
		QDomElement verified = doc.createElement("verified");
		//date
		QDomElement date = doc.createElement("date");
		//id
		QDomElement tgtID = doc.createElement("id");

		//bbox
		QDomElement bbox = doc.createElement("bbox");
		//polygon
		QDomElement polygon = doc.createElement("polygon");

		fr = frame - lb->boxes[0].frame;
		if(fr >= 0 && fr < lb->boxes.count())
		{
			QRect b = imageToImage(mDisplayImage->width(), mDisplayImage->height(), lb->boxes[fr].rc, origSz.width(), origSz.height());
			
			txt = doc.createTextNode(lb->name);
			name.appendChild(txt);

			txt = doc.createTextNode(QDateTime::currentDateTime().toString("d-MMM-yyy h:mm:ss"));
			date.appendChild(txt);

			txt = doc.createTextNode(QString::number(lb->number));
			tgtID.appendChild(txt);

			//polygon
			if(lb->shape == Rect)
			{
				pt = doc.createElement("pt");
				x = doc.createElement("x");
				y = doc.createElement("y");

				txt = doc.createTextNode(QString::number(b.left()));
				x.appendChild(txt);
				txt = doc.createTextNode(QString::number(b.top()));
				y.appendChild(txt);
				pt.appendChild(x);
				pt.appendChild(y);
				polygon.appendChild(pt);
			}
			else if(lb->shape == Polyg)
			{
				QPolygon pl = imageToImage(mDisplayImage->width(), mDisplayImage->height(), lb->viaPointsPoly[fr].pl, origSz.width(), origSz.height());
				for(int pi = 0; pi < pl.count(); pi++)
				{
					pt = doc.createElement("pt");
					x = doc.createElement("x");
					y = doc.createElement("y");

					QPoint pl_pt = pl.point(pi);
					txt = doc.createTextNode(QString::number(pl_pt.x()));
					x.appendChild(txt);
					txt = doc.createTextNode(QString::number(pl_pt.y()));
					y.appendChild(txt);
					pt.appendChild(x);
					pt.appendChild(y);
					polygon.appendChild(pt);
				}
			}
		}
		else
		{
			txt = doc.createTextNode("");
			bbox.appendChild(txt);
			txt = doc.createTextNode("");
			name.appendChild(txt);
			txt = doc.createTextNode("");
			tgtID.appendChild(txt);
			txt = doc.createTextNode("");
			polygon.appendChild(txt);
		}

		//deleted
		txt = doc.createTextNode("0");
		deleted.appendChild(txt);

		//verified
		txt = doc.createTextNode("1");
		verified.appendChild(txt);

		obj.appendChild(bbox);
		obj.appendChild(name);
		obj.appendChild(tgtID);
		obj.appendChild(polygon);
		obj.appendChild(deleted);
		obj.appendChild(verified);

		root.appendChild(obj);
	}



	QString num;
	num.sprintf("%05d",frame);
	QString fname = path + "/" + mFileNamePrefix + num + ".xml";
	QFile fd(fname);
	if(fd.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		QTextStream out(&fd);
		out << doc;

		fd.close();
	}
}


void SimpleLabel::exportFrameToLabelMeXML(QString path, int frame)
{
	QSize origSz = mMonitor->getImageSize();

	QDomDocument doc("");
	QDomElement root = doc.createElement("annotation");
	doc.appendChild(root);

	//filename
	QDomElement tmp = doc.createElement("filename");
	QDomText txt = doc.createTextNode(mFileName);
	tmp.appendChild(txt);
	root.appendChild(tmp);

	//folder
	tmp = doc.createElement("folder");
	txt = doc.createTextNode(mPath);
	tmp.appendChild(txt);
	root.appendChild(tmp);

	//frame
	tmp = doc.createElement("frame");
	txt = doc.createTextNode(QString("%1").arg(frame));
	tmp.appendChild(txt);
	root.appendChild(tmp);

	//source
	QDomElement src = doc.createElement("source");
	//sourceImage
	tmp = doc.createElement("sourceImage");
	txt = doc.createTextNode("");
	tmp.appendChild(txt);
	src.appendChild(tmp);
	//sourceAnnotation
	tmp = doc.createElement("sourceAnnotation");
	txt = doc.createTextNode("");
	tmp.appendChild(txt);
	src.appendChild(tmp);
	//sensor
	tmp = doc.createElement("sensor");
	txt = doc.createTextNode("");
	tmp.appendChild(txt);
	src.appendChild(tmp);
	//sensorType
	tmp = doc.createElement("sensorType");
	txt = doc.createTextNode("");
	tmp.appendChild(txt);
	src.appendChild(tmp);
	//scenario
	tmp = doc.createElement("scenario");
	txt = doc.createTextNode("");
	tmp.appendChild(txt);
	src.appendChild(tmp);
	//day
	tmp = doc.createElement("day");
	txt = doc.createTextNode("");
	tmp.appendChild(txt);
	src.appendChild(tmp);
	//clip
	tmp = doc.createElement("clip");
	txt = doc.createTextNode("");
	tmp.appendChild(txt);
	src.appendChild(tmp);
	//sampleSet
	tmp = doc.createElement("sampleSet");
	txt = doc.createTextNode("");
	tmp.appendChild(txt);
	src.appendChild(tmp);
	//numberFrames
	tmp = doc.createElement("numberFrames");
	txt = doc.createTextNode(QString::number(mMonitor->getFrameCount()));
	tmp.appendChild(txt);
	src.appendChild(tmp);
	//duration
	tmp = doc.createElement("duration");
	txt = doc.createTextNode("");
	tmp.appendChild(txt);
	src.appendChild(tmp);
	//FrameRate
	tmp = doc.createElement("FrameRate");
	txt = doc.createTextNode("");
	tmp.appendChild(txt);
	src.appendChild(tmp);

	root.appendChild(src);


	//imagesize
	src = doc.createElement("imagesize");
	//rows
	tmp = doc.createElement("rows");
	txt = doc.createTextNode(QString::number(origSz.height()));
	tmp.appendChild(txt);
	src.appendChild(tmp);
	//columns
	tmp = doc.createElement("columns");
	txt = doc.createTextNode(QString::number(origSz.width()));
	tmp.appendChild(txt);
	src.appendChild(tmp);
	//channels
	tmp = doc.createElement("channels");
	txt = doc.createTextNode(QString::number(3));
	tmp.appendChild(txt);
	src.appendChild(tmp);
	root.appendChild(src);


	int fr;
	QDomElement obj;
	QList<Label>::iterator lb;
	for(lb = mLabels.begin(); lb != mLabels.end(); lb++)
	{
		if(lb->boxes.count() < 1)
			continue;

		//object
		obj = doc.createElement("object");

		//bbox
		QDomElement bbox = doc.createElement("bbox");
		//center
		QDomElement center = doc.createElement("center");
		//size
		QDomElement size = doc.createElement("size");
		//obscuration
		QDomElement obscuration = doc.createElement("obscuration");
		//view
		QDomElement view = doc.createElement("view");
		//name
		QDomElement name = doc.createElement("name");
		//tgtID
		QDomElement tgtID = doc.createElement("tgtID");
		//polygon
		QDomElement polygon = doc.createElement("polygon");
		//eccentricity
		QDomElement eccentricity = doc.createElement("eccentricity");
		//deleted
		QDomElement deleted = doc.createElement("deleted");
		//verified
		QDomElement verified = doc.createElement("verified");

		fr = frame - lb->boxes[0].frame;
		if(fr >= 0 && fr < lb->boxes.count())
		{
			QRect b = imageToImage(mDisplayImage->width(), mDisplayImage->height(), lb->boxes[fr].rc, origSz.width(), origSz.height());
			
			QDomElement pt = doc.createElement("pt");
			QDomElement x = doc.createElement("x");
			QDomElement y = doc.createElement("y");

			//top left
			txt = doc.createTextNode(QString::number(b.left()));
			x.appendChild(txt);
			txt = doc.createTextNode(QString::number(b.top()));
			y.appendChild(txt);
			pt.appendChild(x);
			pt.appendChild(y);
			bbox.appendChild(pt);

			//top right
			pt = doc.createElement("pt");
			x = doc.createElement("x");
			y = doc.createElement("y");

			txt = doc.createTextNode(QString::number(b.right()));
			x.appendChild(txt);
			txt = doc.createTextNode(QString::number(b.top()));
			y.appendChild(txt);
			pt.appendChild(x);
			pt.appendChild(y);
			bbox.appendChild(pt);

			//bottom right
			pt = doc.createElement("pt");
			x = doc.createElement("x");
			y = doc.createElement("y");

			txt = doc.createTextNode(QString::number(b.right()));
			x.appendChild(txt);
			txt = doc.createTextNode(QString::number(b.bottom()));
			y.appendChild(txt);
			pt.appendChild(x);
			pt.appendChild(y);
			bbox.appendChild(pt);

			//bottom left
			pt = doc.createElement("pt");
			x = doc.createElement("x");
			y = doc.createElement("y");

			txt = doc.createTextNode(QString::number(b.left()));
			x.appendChild(txt);
			txt = doc.createTextNode(QString::number(b.bottom()));
			y.appendChild(txt);
			pt.appendChild(x);
			pt.appendChild(y);
			bbox.appendChild(pt);

			//top left
			pt = doc.createElement("pt");
			x = doc.createElement("x");
			y = doc.createElement("y");

			txt = doc.createTextNode(QString::number(b.left()));
			x.appendChild(txt);
			txt = doc.createTextNode(QString::number(b.top()));
			y.appendChild(txt);
			pt.appendChild(x);
			pt.appendChild(y);
			bbox.appendChild(pt);

			//center
			x = doc.createElement("x");
			y = doc.createElement("y");

			txt = doc.createTextNode(QString::number((b.left() + b.right())/2));
			x.appendChild(txt);
			txt = doc.createTextNode(QString::number((b.top() + b.bottom())/2));
			y.appendChild(txt);

			center.appendChild(x);
			center.appendChild(y);

			//size
			x = doc.createElement("x");
			y = doc.createElement("y");

			txt = doc.createTextNode(QString::number(b.width()));
			x.appendChild(txt);
			txt = doc.createTextNode(QString::number(b.height()));
			y.appendChild(txt);

			size.appendChild(x);
			size.appendChild(y);

			//obscuration
			txt = doc.createTextNode("unobscured");
			obscuration.appendChild(txt);
			
			//view
			txt = doc.createTextNode("");
			view.appendChild(txt);

			//name
			txt = doc.createTextNode(lb->name);
			name.appendChild(txt);

			//tgtID
			txt = doc.createTextNode(QString::number(lb->number));
			tgtID.appendChild(txt);

			//polygon
			if(lb->shape == Rect)
			{
				pt = doc.createElement("pt");
				x = doc.createElement("x");
				y = doc.createElement("y");

				txt = doc.createTextNode(QString::number(b.left()));
				x.appendChild(txt);
				txt = doc.createTextNode(QString::number(b.top()));
				y.appendChild(txt);
				pt.appendChild(x);
				pt.appendChild(y);
				polygon.appendChild(pt);
			}
			else if(lb->shape == Polyg)
			{
				QPolygon pl = imageToImage(mDisplayImage->width(), mDisplayImage->height(), lb->viaPointsPoly[fr].pl, origSz.width(), origSz.height());
				for(int pi = 0; pi < pl.count(); pi++)
				{
					pt = doc.createElement("pt");
					x = doc.createElement("x");
					y = doc.createElement("y");

					QPoint pl_pt = pl.point(pi);
					txt = doc.createTextNode(QString::number(pl_pt.x()));
					x.appendChild(txt);
					txt = doc.createTextNode(QString::number(pl_pt.y()));
					y.appendChild(txt);
					pt.appendChild(x);
					pt.appendChild(y);
					polygon.appendChild(pt);
				}
			}
			

			//eccentricity
			float sm = qMin(b.width(), b.height());
			float bg = qMax(b.width(), b.height());
			txt = doc.createTextNode(QString::number(bg/sm));
			eccentricity.appendChild(txt);
		}
		else
		{
			txt = doc.createTextNode("");
			bbox.appendChild(txt);
			txt = doc.createTextNode("");
			center.appendChild(txt);
			txt = doc.createTextNode("");
			size.appendChild(txt);
			txt = doc.createTextNode("");
			obscuration.appendChild(txt);
			txt = doc.createTextNode("");
			name.appendChild(txt);
			txt = doc.createTextNode("");
			tgtID.appendChild(txt);
			txt = doc.createTextNode("");
			polygon.appendChild(txt);
			txt = doc.createTextNode("");
			eccentricity.appendChild(txt);
		}

		//deleted
		txt = doc.createTextNode("0");
		deleted.appendChild(txt);

		//verified
		txt = doc.createTextNode("1");
		verified.appendChild(txt);

		obj.appendChild(bbox);
		obj.appendChild(center);
		obj.appendChild(size);
		obj.appendChild(obscuration);
		obj.appendChild(view);
		obj.appendChild(name);
		obj.appendChild(tgtID);
		obj.appendChild(polygon);
		obj.appendChild(eccentricity);
		obj.appendChild(deleted);
		obj.appendChild(verified);

		root.appendChild(obj);
	}



	QString num;
	num.sprintf("%05d",frame);
	QString fname = path + "/" + mFileNamePrefix + num + ".xml";
	QFile fd(fname);
	if(fd.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		QTextStream out(&fd);
		out << doc;

		fd.close();
	}

}

void SimpleLabel::exportToSimpleLabelXML()
{
	int i, k, j;

	QDomDocument doc("Labels");
	QDomElement root = doc.createElement("root");
	doc.appendChild(root);
	QSize origSz = mMonitor->getImageSize();

	//save image properties
	QDomElement imTag = doc.createElement("image");
	imTag.setAttribute("name", mFileName);
	imTag.setAttribute("width", origSz.width());
	imTag.setAttribute("height", origSz.height());
	root.appendChild(imTag);

	//save labels
	for(i = 0; i < mLabels.count(); i++)
	{
		QDomElement tag = doc.createElement("label");
		tag.setAttribute("name", mLabels[i].name);
		tag.setAttribute("number", mLabels[i].number);
		tag.setAttribute("desc", mLabels[i].desc);
		tag.setAttribute("shape", mLabels[i].shape);

		if(mLabels[i].shape == Rect)
		{
			//bounding rectangles
			QDomElement boxes = doc.createElement("boxes");
			for(k = 0; k < mLabels[i].boxes.count(); k++)
			{
				QRect p = imageToImage(mDisplayImage->width(), mDisplayImage->height(), mLabels[i].boxes[k].rc, origSz.width(), origSz.height());
				QDomElement b = doc.createElement("box");
				b.setAttribute("frame", mLabels[i].boxes[k].frame);
				b.setAttribute("left", p.left());
				b.setAttribute("top", p.top());
				b.setAttribute("right", p.right());
				b.setAttribute("bottom", p.bottom());
				b.setAttribute("angle", mLabels[i].boxes[k].angle);

				boxes.appendChild(b);
			}
			tag.appendChild(boxes);

			//bounding rectangle via points
			QDomElement pivots = doc.createElement("pivots");
			for(k = 0; k < mLabels[i].viaPoints.count(); k++)
			{
				QRect p = imageToImage(mDisplayImage->width(), mDisplayImage->height(), mLabels[i].viaPoints[k].rc, origSz.width(), origSz.height());
				QDomElement b = doc.createElement("pivot");
				b.setAttribute("frame", mLabels[i].viaPoints[k].frame);
				b.setAttribute("left", p.left());
				b.setAttribute("top", p.top());
				b.setAttribute("right", p.right());
				b.setAttribute("bottom", p.bottom());
				b.setAttribute("angle", mLabels[i].viaPoints[k].angle);

				pivots.appendChild(b);
			}
			tag.appendChild(pivots);
		}
		else if(mLabels[i].shape == Polyg)
		{
			//polygons
			QDomElement polygons = doc.createElement("polygons");
			for(k = 0; k < mLabels[i].polygons.count(); k++)
			{
				QDomElement polyg = doc.createElement("polygon");
				polyg.setAttribute("frame", mLabels[i].polygons[k].frame);
				for(j = 0; j < mLabels[i].polygons[k].pl.count(); j++)
				{
					QPoint p = imageToImage(mDisplayImage->width(), mDisplayImage->height(), mLabels[i].polygons[k].pl.point(j), origSz.width(), origSz.height());
					QDomElement vertex = doc.createElement("vertex");
					vertex.setAttribute("x", p.x());
					vertex.setAttribute("y", p.y());
					polyg.appendChild(vertex);
				
				}
				polygons.appendChild(polyg);
			}
			tag.appendChild(polygons);

			//polygon via points
			QDomElement polygonPivots = doc.createElement("polygonPivots");
			for(k = 0; k < mLabels[i].viaPointsPoly.count(); k++)
			{
				QDomElement polygPivot = doc.createElement("polygonPivot");
				polygPivot.setAttribute("frame", mLabels[i].viaPointsPoly[k].frame);
				for(j = 0; j < mLabels[i].viaPointsPoly[k].pl.count(); j++)
				{
					QPoint p = imageToImage(mDisplayImage->width(), mDisplayImage->height(), mLabels[i].viaPointsPoly[k].pl.point(j), origSz.width(), origSz.height());
					QDomElement vertex = doc.createElement("vertex");
					vertex.setAttribute("x", p.x());
					vertex.setAttribute("y", p.y());
					polygPivot.appendChild(vertex);
				}
				polygonPivots.appendChild(polygPivot);
			}
			tag.appendChild(polygonPivots);
		}
		root.appendChild(tag);
	}

	QString saveFile = mSaveDgl->ui.edtSavePath->text();

	QFile fd(saveFile);
	if(fd.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		QTextStream out(&fd);
		out << doc;

		fd.close();
	}
}


void SimpleLabel::on_actionExport_triggered()
{
	if(mSaveDgl->mPath == "")
		mSaveDgl->mPath = mPath;
	
	if(mSaveDgl->ui.edtFileNamePrefix->text().trimmed() == "")
	{
		if(mFirstFrameNumber < 0)
		{
			mSaveDgl->ui.edtFileNamePrefix->setText(mFileName);
			mSaveDgl->mPrefix = mFileName;
		}
		else
		{
			mSaveDgl->ui.edtFileNamePrefix->setText(mFileNamePrefix);
			mSaveDgl->mPrefix = mFileNamePrefix;
		}
		
	}

	//only set the max numbers in the first time
	if(mSaveDgl->ui.edtFirstImageIndex->text().toInt() < 0)
	{
		mSaveDgl->ui.edtFirstImageIndex->setText(QString::number(mFirstFrameNumber));
		mSaveDgl->ui.edtLastImageIndex->setText(QString::number( mFirstFrameNumber + mMonitor->getFrameCount() - 1));
	}
	
	mSaveDgl->show();
}

void SimpleLabel::on_SaveDialog_accept()
{
	if(mSaveDgl->ui.rbtnBlackBgrd->isChecked())
	{
		exportToMovie(false);
	}
	else if(mSaveDgl->ui.rbtnOrigImage->isChecked())
	{
		exportToMovie(true);
	}
	else if(mSaveDgl->ui.rbtnSimpleLabelXML->isChecked())
	{
		exportToSimpleLabelXML();
	}
	else if(mSaveDgl->ui.rbtLabelMeXML->isChecked())
	{
		exportToLabelMeXML();
	}
	else if(mSaveDgl->ui.rbtnMatlabStruct->isChecked())
	{
		exportToMatlabStruct();
	}

}

void SimpleLabel::on_cmbBoxLabelShape_currentIndexChanged ( int index )
{
	mShapeMode = (LabelShape)index;
	int row = ui.listLabels->currentRow();
	if(row >= 0 && mMonitor->isInitialized())
	{
		switch(mShapeMode)
		{
		case Rect:
			mStatus_Mode.setText("Rect");
			setDrawRectToFrame(mMonitor->getCurrentFrameNumber());
			ui.actionNewPolygon->setDisabled(true);
			break;
		case Polyg:
			mStatus_Mode.setText("Polygon");
			setDrawPolygonToFrame(mMonitor->getCurrentFrameNumber());
			ui.actionNewPolygon->setEnabled(true);
			break;
		default:
			break;
		}
		update();
	}
}

void SimpleLabel::on_actionNewPolygon_triggered()
{
	int row = ui.listLabels->currentRow();
	if(row >= 0)
	{
		if((mLabels[row].boxes.count() == 0 && mLabels[row].polygons.count() == 0) ||
			QMessageBox::question(this, "Reset the Label",
			"Creating a new polygon will remove all previously selected regions of interest for this label.\nDo you want to proceed?",
			QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Ok)
		{
			//clear all points in the current label
			mLabels[row].boxes.clear();
			mLabels[row].viaPoints.clear();
			mLabels[row].viaPointsPoly.clear();
			mLabels[row].polygons.clear();
			mLabels[row].shape = Polyg;

			mFirstPolygon = QPolygon();
			mNewPolygon = true;

			ui.centralWidget->setDisabled(true);
			ui.menuFile->setDisabled(true);
			ui.actionNewPolygon->setDisabled(true);
			ui.menuHelp->setDisabled(true);

			ui.actionFinish->setEnabled(true);
		}
	}
}

void SimpleLabel::on_actionFinish_triggered()
{
	//int row = ui.listLabels->currentRow();

	//not enough points to build polygon
	if(mFirstPolygon.count() < 3)
	{
		//remove everything
		mFirstPolygon = QPolygon();
		//mLabels[row].viaPointsPoly.clear();
	}
	else
	{
		addViaPointPoly(mMonitor->getCurrentFrameNumber(), mFirstPolygon);
		mDrawPolygon = mFirstPolygon;
	}
	mNewPolygon = false;

	ui.centralWidget->setEnabled(true);
	ui.menuFile->setEnabled(true);
	ui.actionNewPolygon->setEnabled(true);
	ui.menuHelp->setEnabled(true);

	ui.actionFinish->setDisabled(true);
	update();
}
