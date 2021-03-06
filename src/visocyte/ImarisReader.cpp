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


#include <ImarisReader.hpp>
#include <Viewer.hpp>

vtkStandardNewMacro(ImarisReader);

ImarisReader::ImarisReader() {}

ImarisReader::~ImarisReader() {}

void ImarisReader::initialize(Viewer* viewer, std::string input_file_name) {
  viewer_ = viewer;
  SetFileName(input_file_name.c_str());
  DetectNumericColumnsOn();
  SetFieldDelimiterCharacters(",");
  Update();
  table_ = GetOutput();
}

std::vector<float> ImarisReader::initialize_points() {
  std::vector<int>& frames(viewer_->get_frames());
  std::vector<int>& ids(viewer_->get_ids());
  const int skip_rows(1); //skip first three header rows
  int time_column(-1); //look for time column
  std::cout << "number of cols:" << table_->GetNumberOfColumns() << std::endl;
  for (int i(0); i < table_->GetNumberOfColumns(); ++i) {
    std::cout << "i:" << i << " " << (table_->GetValue(skip_rows-1, i)).ToString() << std::endl; 
    if ((table_->GetValue(skip_rows-1, i)).ToString() == "Time" ||
        (table_->GetValue(skip_rows-1, i)).ToString() == "TimePointID") {
      time_column = i;
    }
    if ((table_->GetValue(skip_rows-1, i)).ToString() == "Parent" ||
        (table_->GetValue(skip_rows-1, i)).ToString() == "TrackID") {
      id_column_ = i;
    }
  }
  if (time_column == -1) {
    std::cout << "time column not found" << std::endl;
  }
  if (id_column_ == -1) {
    std::cout << "id column not found" << std::endl;
  }
  int time(0);
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
    minx_ = std::min(minx_, (table_->GetValue(cnt, 0)).ToFloat());
    maxx_ = std::max(maxx_, (table_->GetValue(cnt, 0)).ToFloat());
    miny_ = std::min(miny_, (table_->GetValue(cnt, 1)).ToFloat());
    maxy_ = std::max(maxy_, (table_->GetValue(cnt, 1)).ToFloat());
    minz_ = std::min(minz_, (table_->GetValue(cnt, 2)).ToFloat());
    maxz_ = std::max(maxz_, (table_->GetValue(cnt, 2)).ToFloat());
    int id((table_->GetValue(cnt, id_column_)).ToInt());
    if (std::find(ids.begin(), ids.end(), id) == ids.end()) {
      ids.push_back(id);
      ids_map_[id] = ids.size()-1;
    }
    ++cnt;
  }
  frames.push_back(cnt-1);
  double ave(0);
  for (unsigned i(0); i != frames.size(); ++i) {
    if (i > 0) {
      ave += frames[i] - frames[i-1];
    }
  }
  std::vector<float> min_max({minx_, miny_, minz_, maxx_, maxy_, maxz_});
  std::cout << "average cells per frame:" << ave/(frames.size()-1) <<
    std::endl;
  std::cout << "dims:" << maxx_-minx_ << " " << maxy_-miny_ << " " << maxz_-minz_ <<
    std::endl;
  std::cout << "min:" << minx_ << " " << miny_ << " " << minz_ << std::endl;
  std::cout << "max:" << maxx_ << " " << maxy_ << " " << maxz_ << std::endl;
  return min_max;
}

void ImarisReader::update_points(int current_frame) {
  std::cout << "current_frame:" << current_frame << std::endl;
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
    unsigned color_index(ids_map_[
                               (table_->GetValue(row,id_column_)).ToInt()]);
    /*
    if (x < minx_+200) {
      color_index = ids_map_[0];
    }
    if (y < miny_+200) {
      color_index = ids_map_[0];
    }
    */
    viewer_->insert_color(color_index, i);
    for (unsigned j(0); j != n_surface; ++j) {
      float zi(uni_z(rng));
      float t(uni_t(rng));
      points->SetPoint(n+i*n_surface+j, 
                        x+sqrt(1-pow(zi,2))*cos(t)*radius,
                        y+sqrt(1-pow(zi,2))*sin(t)*radius,
                        z+zi*radius);
      viewer_->insert_color(color_index, n+i*n_surface+j);
    }
  }
}

void ImarisReader::reset() {
  ids_map_.clear();
}

int ImarisReader::get_offset() {
  return 2;
}
