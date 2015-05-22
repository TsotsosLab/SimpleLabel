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
#include "SimpleLabel.h"
#include <QtGui/QApplication>


int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setApplicationVersion(APP_VERSION);
	SimpleLabel w;
	w.show();
	return a.exec();
}
