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

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>
#include <qmath.h>
#include <QDebug>
#include <QRect>
#include <QPolygon>
#include <QTransform>

#define W_DISPLAYIMAGE	800
#define H_DISPLAYIMAGE	600

#define WINDOW_OFFSET_X	5
#define WINDOW_OFFSET_Y 25
#define CORNER_CIRCLE_RAD	5
#define LENGTH_OF_ROTATION_LINE	20

#define INVALID_ANGLE	0xffffffff

#define ROUND(X) (abs(X -floor(X))<0.5?floor(X):floor(X)+1)
#define sqr(X)	((X)*(X))

//#define APP_VERSION "1.0.1"
enum InputType
{
	AviFile,
	ImageSequence,
	None
};

enum LabelShape
{
	Rect,
	Polyg,
	Other
};

enum InterpolationMethod
{
	LinearIntr,
	Motion
};

struct  ViaPoint
{
public:
	ViaPoint()
	{
		Init(0, 0.0, QRect(0,0,0,0));
	}

	ViaPoint(int frame , float angle, QRect rc)
	{
		Init(frame, angle, rc);
	}

	float angle;
	int frame;
	QRect rc;

        bool operator == (const ViaPoint& b)
	{
		return this->angle == b.angle &&
			this->frame == b.frame &&
			this->rc == b.rc;
	}

        bool operator != (const ViaPoint& b)
	{
		return !(*this == b);
	}

        ViaPoint& operator=(const ViaPoint& b)
	{
		if(this != &b)
		{
			this->angle = b.angle;
			this->frame = b.frame;
			this->rc = b.rc;
		}
		return *this;
	}

private:
	void Init(int fr , float a, QRect r)
	{
		frame = fr;
		angle = a;
		rc = r;
	}
};

struct ViaPointPolygon
{
	int frame;
	QPolygon pl;
};

struct Label
{
	int number;
	QString name;
	QString desc;
	LabelShape shape;
	QList<ViaPoint> boxes;
	QList<ViaPoint> viaPoints;
	QList<ViaPointPolygon> polygons;
	QList<ViaPointPolygon> viaPointsPoly;

	ViaPoint* findBoxByFrame(int frame)
	{
		ViaPoint *res = NULL;
		int fr;
		if(boxes.count() > 0)
		{
			fr = frame - boxes[0].frame;
			if(fr >= 0 && fr < boxes.count())
				res = &(boxes[fr]);
		}

		return res;
	}

	ViaPointPolygon* findPolygonByFrame(int frame)
	{
		ViaPointPolygon *res = NULL;
		int fr;
		if(polygons.count() > 0)
		{
			fr = frame - polygons[0].frame;
			if(fr >= 0 && fr < polygons.count())
				res = &(polygons[fr]);
		}

		return res;
	}
};

class CommonFunctions
{
public:
	static const QPoint NULL_POINT;
	static const ViaPoint NULL_RECT;
	static const QPolygon NULL_POLYGON;

	static void splitPath(QString fullpath, QString &path, QString &filename, QString &fnamePrefix, uint n, int &index, QString &ext)
	{
		bool ok;
		QString fpath = fullpath;
		fpath.replace("\\", "/");
		int dot = fpath.lastIndexOf(".");
		int slash = fpath.lastIndexOf("/");

		path = fullpath.left(slash + 1);
		ext = fullpath.right(fullpath.length() - dot);
		filename = fullpath.mid(slash + 1, dot - slash - 1);
		fnamePrefix = filename.left(filename.length() - n);
		index = filename.right(n).toInt(&ok);
		if(!ok)
			index = -1;
	}

	static void splitPath(QString fullpath, QString &path, QString &filename, QString &ext)
	{
		QString fpath = fullpath;
		fpath.replace("\\", "/");
		int dot = fpath.lastIndexOf(".");
		int slash = fpath.lastIndexOf("/");

		path = fullpath.left(slash + 1);
		ext = fullpath.right(fullpath.length() - dot);
		filename = fullpath.mid(slash + 1, dot - slash - 1);
	}

	static int checkForVertices(QPoint p, QRect rc)
	{
		int res = -1;
		if(isCloseToEachOther(rc.topLeft(), p))
			res = 0;
		else if(isCloseToEachOther(rc.topRight(), p))
			res = 1;
		else if(isCloseToEachOther(rc.bottomRight(), p))
			res = 2;
		else if(isCloseToEachOther(rc.bottomLeft(), p))
			res = 3;

		return res;
	}


	static int checkForVerticesWithRotation(QPoint p, ViaPoint vp)
	{
		QPoint np = rotatePointAboutPoint(p, vp.angle, vp.rc.center());
		QRect rc = vp.rc;

		int res = -1;
		if(isCloseToEachOther(rc.topLeft(), np))
			res = 0;
		else if(isCloseToEachOther(rc.topRight(), np))
			res = 1;
		else if(isCloseToEachOther(rc.bottomRight(), np))
			res = 2;
		else if(isCloseToEachOther(rc.bottomLeft(), np))
			res = 3;

		return res;
	}

	static bool checkForRotation(QPoint p, ViaPoint vp)
	{
		QPoint c = vp.rc.center();
		QPoint pt(vp.rc.center().x(), vp.rc.top() - LENGTH_OF_ROTATION_LINE);

		QPoint new_pt = rotatePointAboutPoint(p, vp.angle, c);
		
		return isCloseToEachOther(pt, new_pt);
	}

	static QPoint rotatePointAboutPoint(QPoint p, double ang, QPoint o)
	{
		QTransform tr;
		QPointF p1(p);
		QPointF o1(o);

		tr.translate(o1.x(), o1.y());
		tr.rotate(ang);
		tr.translate(-o1.x(), -o1.y());

		QPointF res = tr.map(p1);

		double l1 = vectorLength(p1 - o1);
		double l2 = vectorLength(res - o1);
		if(l1 - l2 > 0.00001)
			qDebug() << "Something is off";

		return QPoint(qRound(res.x()), qRound(res.y()));
	}

	static int checkForVertices(QPoint p, QPolygon pl)
	{
		int res = -1;
		for(int i = 0; i < pl.count(); i++)
		{
			if(isCloseToEachOther(pl.point(i), p))
			{
				res = i;
				break;
			}
		}

		return res;
	}

	static double vectorLength(QPoint p)
	{
		return vectorLength(QPointF(p));
	}

	static double vectorLength(QPointF p)
	{
		if(p.manhattanLength() == 0) return 0.0;

		return qSqrt( sqr(p.x()) + sqr(p.y()) );
	}

	static double findAngleBetweenVectors2(QPoint v1, QPoint v2)
	{
		double ang = qAtan2(v1.x(), v1.y()) - qAtan2(v2.x(), v2.y());
		
		//converting to degrees
		return 180*ang/M_PI;
	}

	static QPoint getVertex(QRect rc, int index)
	{
		QPoint pt = NULL_POINT;

		switch(index)
		{
		case 0:
			pt = rc.topLeft();
			break;
		case 1:
			pt = rc.topRight();
			break;
		case 2:
			pt = rc.bottomRight();
			break;
		case 3:
			pt = rc.bottomLeft();
			break;
		default:
			break;
		}

		return pt;
	}

	static QPoint getOpposingVertex(QRect rc, int index)
	{
		QPoint pt = NULL_POINT;

		switch(index)
		{
		case 0:
			pt = rc.bottomRight();
			break;
		case 1:
			pt = rc.bottomLeft();
			break;
		case 2:
			pt = rc.topLeft();
			break;
		case 3:
			pt = rc.topRight();
			break;
		default:
			break;
		}

		return pt;
	}

	private:
		static bool isCloseToEachOther(QPoint pl, QPoint p2)
		{
			bool res = false;
			int rad = CORNER_CIRCLE_RAD;
			QPoint rt = pl - p2;
			float dist = sqr(rt.x()) + sqr(rt.y());
			
			if(dist == 0)
			{
				res = true;
			}
			else
			{
				res = sqrt(dist) < rad;
			}

			return res;
		}
};


#endif
