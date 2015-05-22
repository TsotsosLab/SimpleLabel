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
#include "SaveDialog.h"
#include <QDebug>
#include <QFileDialog>


SaveDialog::SaveDialog(QWidget *parent, Qt::WindowFlags flags)
		: QDialog(parent, flags)
{
	ui.setupUi(this);
	connect(ui.rbtnBlackBgrd, SIGNAL(toggled(bool)), this, SLOT(rbtnFormat_toggled(bool)));
	connect(ui.rbtnOrigImage, SIGNAL(toggled(bool)), this, SLOT(rbtnFormat_toggled(bool)));
	connect(ui.rbtnMatlabStruct, SIGNAL(toggled(bool)), this, SLOT(rbtnFormat_toggled(bool)));
	connect(ui.rbtnSimpleLabelXML, SIGNAL(toggled(bool)), this, SLOT(rbtnFormat_toggled(bool)));
	connect(ui.rbtLabelMeXML, SIGNAL(toggled(bool)), this, SLOT(rbtnFormat_toggled(bool)));
	connect(ui.rbtnSaveAsAVI, SIGNAL(toggled(bool)), this, SLOT(rbtnFormat_toggled(bool)));
	connect(ui.rbtnSaveImgSeq, SIGNAL(toggled(bool)), this, SLOT(rbtnFormat_toggled(bool)));
	connect(ui.edtFirstImageIndex, SIGNAL(textChanged(const QString &)), this, SLOT(on_edtFileNamePrefix_textChanged(const QString &)));

	QValidator *v = new QIntValidator(0, 99999, this);
	ui.edtFirstImageIndex->setValidator(v);
	ui.edtLastImageIndex->setValidator(v);
	rbtnFormat_toggled(true);
}

SaveDialog::~SaveDialog(void)
{
}


void SaveDialog::rbtnFormat_toggled(bool b)
{
	if(b)
	{
		if(ui.rbtnBlackBgrd->isChecked() || ui.rbtnOrigImage->isChecked())
		{
			ui.rbtnSaveAsAVI->setEnabled(true);
			ui.rbtnSaveImgSeq->setEnabled(true);			
		}
		else
		{
			ui.rbtnSaveAsAVI->setDisabled(true);
			ui.rbtnSaveImgSeq->setDisabled(true);
		}

		if(ui.rbtnBlackBgrd->isChecked() || ui.rbtnOrigImage->isChecked())
		{
			if(ui.rbtnSaveAsAVI->isChecked())
			{
				mBrowseSettings.title = tr("Save As avi");
				mBrowseSettings.ext = tr(".avi");
				mBrowseSettings.filter = tr("AVI Files (*.avi)");
				mBrowseSettings.index = "";
			}
			else
			{
				mBrowseSettings.title = tr("Save As Image Sequense");
				mBrowseSettings.ext = tr(".png");
				mBrowseSettings.filter = tr("Image Files (*.png)");
				mBrowseSettings.index = mBrowseSettings.index.sprintf("%05d",ui.edtFirstImageIndex->text().toInt());
				
			}

		}
		else if(ui.rbtnMatlabStruct->isChecked())
		{
			mBrowseSettings.title = tr("Save As Matlab structure");
			mBrowseSettings.ext = tr(".m");
			mBrowseSettings.filter = tr("Matlab Files (*.m)");
			mBrowseSettings.index = "";
			
		}
		else if(ui.rbtnSimpleLabelXML->isChecked())
		{
			mBrowseSettings.title = tr("Save As SimpleLabel XML");
			mBrowseSettings.ext = tr(".xml");
			mBrowseSettings.filter = tr("XML Files (*.xml)");
			mBrowseSettings.index = "";
		}
		else if(ui.rbtLabelMeXML->isChecked())
		{
			mBrowseSettings.title = tr("Save As LabelMe XML");
			mBrowseSettings.ext = tr(".xml");
			mBrowseSettings.filter = tr("XML Files (*.xml)");
			mBrowseSettings.index = mBrowseSettings.index.sprintf("%05d",ui.edtFirstImageIndex->text().toInt());
		}

		ui.edtSavePath->setText(mPath + ui.edtFileNamePrefix->text() + mBrowseSettings.index + mBrowseSettings.ext);
	}
}


void SaveDialog::on_btnBrowse_clicked()
{
	QString saveFile = QFileDialog::getExistingDirectory(this, mBrowseSettings.title,
								mPath,
								QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);


	if(saveFile != "")
	{
		mPath = saveFile + "/";
	}

	rbtnFormat_toggled(true);
}

void SaveDialog::on_edtFileNamePrefix_textChanged(const QString &text)
{
		ui.edtSavePath->setText(mPath + text + mBrowseSettings.index + mBrowseSettings.ext);
	//rbtnFormat_toggled(true);
}

QString SaveDialog::fixFileNameForMatlab(QString str)
{
	QString tmp = str.replace("-", "_");
	tmp = tmp.replace("+", "_");
	return tmp;
}