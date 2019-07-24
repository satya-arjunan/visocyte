//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//
//        This file is part of Visocyte
//
//        Copyright (C) 2019 Satya N.V. Arjunan
//
//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//
//
// Motocyte is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// Motocyte is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public
// License along with Motocyte -- see the file COPYING.
// If not, write to the Free Software Foundation, Inc.,
// 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
// 
//END_HEADER
//
// written by Satya N. V. Arjunan <satya.arjunan@gmail.com>
//


#include <SpatiocyteReader.hpp>
#include <Viewer.hpp>

vtkStandardNewMacro(SpatiocyteReader);

SpatiocyteReader::SpatiocyteReader() {}

SpatiocyteReader::~SpatiocyteReader() {}

void SpatiocyteReader::initialize(Viewer* viewer, std::string input_file_name) {
  viewer_ = viewer;
  SetFileName(input_file_name.c_str());
  DetectNumericColumnsOn();
  SetFieldDelimiterCharacters(",");
  Update();
  table_ = GetOutput();
}

void SpatiocyteReader::initialize_points() {
  std::vector<int>& frames(viewer_->get_frames());
  std::vector<int>& ids(viewer_->get_ids());
  std::map<int, int>& ids_map(viewer_->get_ids_map());

  const unsigned ncolumns(table_->GetNumberOfColumns());

  std::string radius_str((table_->GetValue(0, ncolumns-1)).ToString());
  voxel_radius_ = ::atof(split(radius_str, "=")[1].c_str());
  for (unsigned i(1); i < ncolumns-1; ++i) { 
    species_list_.push_back((table_->GetValue(0, i)).ToString());
  }

  const int skip_rows(1); //skip first two header rows
  int time_column(0); //look for time column
  for (int i(0); i != table_->GetNumberOfColumns(); ++i) {
    if ((table_->GetValue(skip_rows-1, i)).ToString() == "Time") {
      time_column = i;
    }
    if ((table_->GetValue(skip_rows-1, i)).ToString() == "Parent" ||
        (table_->GetValue(skip_rows-1, i)).ToString() == "TrackID") {
      id_column_ = i;
    }
  }
  int time(0);
  double minx(1e+10);
  double miny(1e+10);
  double minz(1e+10);
  double maxx(-1e+10);
  double maxy(-1e+10);
  double maxz(-1e+10);
  if (skip_rows < table_->GetNumberOfRows()) { 
    time = (table_->GetValue(skip_rows, time_column)).ToDouble();
    frames.push_back(skip_rows);
  }
  int cnt(skip_rows);
  while(cnt < table_->GetNumberOfRows()) {
    if ((table_->GetValue(cnt, time_column)).ToDouble() != time) {
        frames.push_back(cnt);
        time = (table_->GetValue(cnt, time_column)).ToDouble();
    }
    minx = std::min(minx, (table_->GetValue(cnt, 0)).ToDouble());
    maxx = std::max(maxx, (table_->GetValue(cnt, 0)).ToDouble());
    miny = std::min(miny, (table_->GetValue(cnt, 1)).ToDouble());
    maxy = std::max(maxy, (table_->GetValue(cnt, 1)).ToDouble());
    minz = std::min(minz, (table_->GetValue(cnt, 2)).ToDouble());
    maxz = std::max(maxz, (table_->GetValue(cnt, 2)).ToDouble());
    int id((table_->GetValue(cnt, id_column_)).ToInt());
    if (std::find(ids.begin(), ids.end(), id) == ids.end()) {
      ids.push_back(id);
      ids_map[id] = ids.size()-1;
    }
    ++cnt;
  }
  frames.push_back(cnt-1);
  double ave(0);
  for (unsigned i(0); i != frames.size(); ++i) {
    std::cout << "i:" << i << " " << frames[i] << std::endl;
    if (i > 0) {
      ave += frames[i] - frames[i-1];
    }
  }
  std::cout << "average cells per frame:" << ave/(frames.size()-1) <<
    std::endl;
  std::cout << "dims:" << maxx-minx << " " << maxy-miny << " " << maxz-minz <<
    std::endl;
}

void SpatiocyteReader::update_points(int current_frame) {
  std::vector<int>& frames(viewer_->get_frames()); 
  vtkSmartPointer<vtkPoints> points(viewer_->get_points()); 
  vtkSmartPointer<vtkUnsignedCharArray> colors(viewer_->get_colors());
  std::mt19937_64& rng(viewer_->get_rng());
  const int n_surface(300);
  const double radius(5);
  int n(frames[current_frame+1]-frames[current_frame]);
  points->SetNumberOfPoints(n+n*n_surface);
  colors->SetNumberOfValues(n+n*n_surface);
  std::uniform_real_distribution<> uni_z(-1, 1);
  std::uniform_real_distribution<> uni_t(0, 2*M_PI);
  for (unsigned i(0); i < n; ++i) {
    int row(i+frames[current_frame]);
    const float x((table_->GetValue(row,0)).ToDouble());
    const float y((table_->GetValue(row,1)).ToDouble());
    const float z((table_->GetValue(row,2)).ToDouble());
    points->SetPoint(i, x, y, z);
    viewer_->insert_color((table_->GetValue(row,id_column_)).ToInt(), i);
    for (unsigned j(0); j != n_surface; ++j) {
      float zi(uni_z(rng));
      float t(uni_t(rng));
      points->SetPoint(n+i*n_surface+j, 
                        x+sqrt(1-pow(zi,2))*cos(t)*radius,
                        y+sqrt(1-pow(zi,2))*sin(t)*radius,
                        z+zi*radius);
      viewer_->insert_color((table_->GetValue(row,id_column_)).ToInt(),
                           n+i*n_surface+j);
    }
  }
}
