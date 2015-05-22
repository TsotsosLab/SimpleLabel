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
#include "Monitor.h"
#include <QMessageBox>
#include <QFile>

Monitor::Monitor(QObject *parent)
	: QThread(parent)
{
	mCurrImage = NULL;
	mCvCapture = NULL;
	mCurrentFrameNumber = 0;
	stopExec = false;
	mInputType = None;
	mInitialized = false;
}

Monitor::~Monitor()
{
	reset();
}

void Monitor::reset()
{
	stopExec = false;
	mInputType = None;
	mFileNamePrefix = "";
	mFileName = "";
	mPath = "";
	mExtention = "";
	mFirstFrameNumber = -1;
	mCurrentFrameNumber = -1;
	mLastFrameNumber = -1;
	mInitialized = false;

	if(mCurrImage)
	{
		delete mCurrImage;
		mCurrImage = NULL;
	}
}

void Monitor::run()
{
	stopExec = false;
	int fps = getFPS();
	if(fps < 1)
		fps = 30;

	int delay = 1000/fps;
	while(mInitialized && !stopExec && mCurrentFrameNumber < mLastFrameNumber)
	{
		moveToFrame(mCurrentFrameNumber + 1);
		emit imageChanged();
		msleep(delay);
	}
	stopExec = true;
}

void Monitor::stop()
{
	stopExec = true;
}

void Monitor::lockImages()
{
	mImMutex.lock();
}

void Monitor::releaseImages()
{
	mImMutex.unlock();
}


void Monitor::setCvCapture(CvCapture* c)
{
	int w, h, bpp;

	if(c)
		reset();

	mCvCapture = c;

	if(c)
	{
		
		cvSetCaptureProperty(mCvCapture, CV_CAP_PROP_POS_FRAMES, mCurrentFrameNumber);
		IplImage *im = cvQueryFrame(mCvCapture);
		w = im->width;
		h = im->height;
		bpp = im->nChannels;

		lockImages();
		mCurrImage = new QImage(w, h, QImage::Format_ARGB32);
		convertRGB2ARGB(im, mCurrImage);
		releaseImages();
		mInputType = AviFile;

		mFirstFrameNumber = 0;
		mLastFrameNumber = cvGetCaptureProperty(mCvCapture, CV_CAP_PROP_FRAME_COUNT) - 1;
		mInitialized = true;
		emit imageChanged();
	}
}

int Monitor::getFPS()
{
	int res = 0;
	if(mInitialized)
	{
		if(mInputType == AviFile)
			res = cvGetCaptureProperty(mCvCapture, CV_CAP_PROP_FPS);
	}

	return res;
}

void Monitor::setFisrtFilenameOfSequence(QString fname)
{
	int dot = fname.lastIndexOf(".");
	int slash = fname.lastIndexOf("/");
	bool ok;


	QImage im(fname);

	if(!im.isNull())
	{
		if(mCurrImage != NULL)
			reset();

		mPath = fname.left(slash + 1);
		mExtention = fname.right(fname.length() - dot);
		mFileName = fname.mid(slash + 1, dot - slash - 1);
		mFileNamePrefix = mFileName.left(mFileName.length() - 5);
		mFirstFrameNumber = mFileName.right(5).toInt(&ok);
		mCurrentFrameNumber = mFirstFrameNumber;
		mInputType = ImageSequence;

		mCurrImage = new QImage(im);

		if(ok)
		{
			findLastImageSequenceFrameNumber();
			moveToFrame(mFirstFrameNumber);
		}
		else
		{
			mFirstFrameNumber = -1;
			mLastFrameNumber = -1;
		}
		

		mInitialized = true;
		emit imageChanged();

	}
	else
	{
		QMessageBox::information(NULL, "Image Error", "File " + fname + " is not an image or cannot be read.");
	}
}


int Monitor::getFrameCount()
{
	int f = -1;
	if(mInputType == AviFile)
	{
		if(mCvCapture)
			f = (int)cvGetCaptureProperty(mCvCapture, CV_CAP_PROP_FRAME_COUNT);
	}
	else if(mInputType == ImageSequence)
	{
		f = mLastFrameNumber - mFirstFrameNumber + 1;
	}

	return f;
}

void Monitor::findLastImageSequenceFrameNumber()
{
	int i = 0;
	if(mInputType == ImageSequence)
	{
		i = mFirstFrameNumber;
		//QString s = mPath + mFileNamePrefix + QString("%1").arg(i,5,10,QChar('0')) + mExtention;
		
		while(QFile::exists(mPath + mFileNamePrefix + QString("%1").arg(i,5,10,QChar('0')) + mExtention))
		{
			i++;
		}
	}

	mLastFrameNumber = i - 1; //-1 because current image does not exist
}

void Monitor::moveToFrame(int f)
{
	if(mInputType == AviFile)
	{
		if(mCvCapture)
		{
			cvSetCaptureProperty(mCvCapture, CV_CAP_PROP_POS_FRAMES, f);
			IplImage *im = cvQueryFrame(mCvCapture);
			mCurrentFrameNumber = f;
			lockImages();
			convertRGB2ARGB(im, mCurrImage);
			releaseImages();
		}
	}
	else if( mInputType == ImageSequence)
	{
		if(f >= mFirstFrameNumber && f <= mLastFrameNumber)
		{
			QString fname = mPath + mFileNamePrefix + QString("%1").arg(f, 5, 10, QChar('0')) + mExtention;

			lockImages();
			mCurrImage->load(fname);
			mCurrentFrameNumber = f;
			releaseImages();
		}
	}
}

void Monitor::convertRGB2ARGB(IplImage *dataIn, QImage *dataOut)
{
	int w = dataIn->width;
	int h = dataIn->height;
//	int step = dataIn->widthStep;
	int c = dataIn->nChannels;
	uchar* data = (unsigned char*)dataIn->imageData;
	uchar* res = dataOut->bits();

	int i;
	for(i = 0; i < w*h; i++)
	{
		memcpy(res, data, 3*sizeof(uchar));
		res[3] = 255;

		data += c;	//next line in the IplImage
		res += 4;
	}
}

void Monitor::convertARGB2RGB(QImage *dataIn, IplImage *dataOut)
{
	int w = dataIn->width();
	int h = dataIn->height();
//	int step = dataIn->widthStep;
	int c = dataOut->nChannels;
	uchar* res = (unsigned char*)dataOut->imageData;
	uchar* data = dataIn->bits();

	int i;
	for(i = 0; i < w*h; i++)
	{
		memcpy(res, data, 3*sizeof(uchar));

		data += 4;	
		res += c;	//next line in the IplImage
	}
}


IplImage* Monitor::getFrame(int f)
{
	cvSetCaptureProperty(mCvCapture, CV_CAP_PROP_POS_FRAMES, f);
	return cvQueryFrame(mCvCapture);
}

QSize Monitor::getImageSize()
{
	if(mInitialized)
		return mCurrImage->size();
	else
		return QSize(0,0);
}