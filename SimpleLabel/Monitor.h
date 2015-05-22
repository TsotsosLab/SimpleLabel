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
#ifndef MONITOR_H
#define MONITOR_H

#include <QThread>
#include <QImage>
#include <QMutex>
#include <opencv\cv.h>
#include <opencv\highgui.h>
#include "Constants.h"

class Monitor : public QThread
{
	Q_OBJECT

public:
	Monitor(QObject *parent = NULL);
	virtual ~Monitor();

	void setCvCapture(CvCapture* c);
	CvCapture* getCvCapture(){return mCvCapture;}
	void setFisrtFilenameOfSequence(QString fname);

	int getFrameCount();
	IplImage* getFrame(int f);
	int getCurrentFrameNumber() { return mCurrentFrameNumber; };
	QImage* getImage() { return mCurrImage;}
	QSize getImageSize();
	int getFPS();

	void lockImages();
	void releaseImages();

	void stop();
	void moveToFrame(int f);
	
	void convertARGB2RGB(QImage *dataIn, IplImage *dataOut);

	bool isInitialized() {return mInitialized;}

protected:
	virtual void run();

private:
	void reset();
	void convertRGB2ARGB(IplImage *dataIn, QImage *dataOut);
	void findLastImageSequenceFrameNumber();

private:
	bool mInitialized;
	bool stopExec;
	QMutex mImMutex;
	CvCapture *mCvCapture;
	QString mFileNamePrefix;
	QString mFileName;
	QString mPath;
	QString mExtention;
	int mFirstFrameNumber;
	int mCurrentFrameNumber;
	int mLastFrameNumber;
	InputType mInputType;
	QImage *mCurrImage;

signals:
	void imageChanged();
};


#endif // MONITOR_H