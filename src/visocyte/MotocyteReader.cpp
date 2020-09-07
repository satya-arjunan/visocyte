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


#include <MotocyteReader.hpp>
#include <Viewer.hpp>

vtkStandardNewMacro(MotocyteReader);

MotocyteReader::MotocyteReader() {}

MotocyteReader::~MotocyteReader() {}

void MotocyteReader::initialize(Viewer* viewer, std::string input_file_name) {
  viewer_ = viewer;
  SetFileName(input_file_name.c_str());
  DetectNumericColumnsOn();
  SetFieldDelimiterCharacters(",");
  Update();
  table_ = GetOutput();
}

std::vector<float> MotocyteReader::initialize_points() {
  std::vector<int>& frames(viewer_->get_frames());
  std::vector<int>& ids(viewer_->get_ids());
  const int skip_rows(1); //skip first three header rows
  int time_column(-1); //look for time column
  std::cout << "number of cols:" << table_->GetNumberOfColumns() << std::endl;
  for (int i(0); i < table_->GetNumberOfColumns(); ++i) {
    std::cout << "i:" << i << " " << (table_->GetValue(skip_rows-1, i)).ToString() << std::endl; 
    if ((table_->GetValue(skip_rows-1, i)).ToString() == "time_id") { 
      time_column = i;
    }
    else if ((table_->GetValue(skip_rows-1, i)).ToString() == "track_id") {
      id_col_ = i;
    }
    else if ((table_->GetValue(skip_rows-1, i)).ToString() == "position_x") {
      px_col_ = i;
    }
    else if ((table_->GetValue(skip_rows-1, i)).ToString() == "position_y") {
      py_col_ = i;
    }
    else if ((table_->GetValue(skip_rows-1, i)).ToString() == "position_z") {
      pz_col_ = i;
    }
    else if ((table_->GetValue(skip_rows-1, i)).ToString() == "instant_speed") {
      instant_speed_col_ = i;
    }
    else if ((table_->GetValue(skip_rows-1, i)).ToString() == "instant_turn") {
      instant_turn_col_ = i;
    }
    else if ((table_->GetValue(skip_rows-1, i)).ToString() ==
             "previous_speed") {
      previous_speed_col_ = i;
    }
    else if ((table_->GetValue(skip_rows-1, i)).ToString() == "previous_turn") {
      previous_turn_col_ = i;
    }
  }
  if (time_column == -1) {
    std::cout << "time column not found" << std::endl;
  }
  if (id_col_ == -1) {
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
    minx_ = std::min(minx_, (table_->GetValue(cnt, px_col_)).ToFloat());
    maxx_ = std::max(maxx_, (table_->GetValue(cnt, px_col_)).ToFloat());
    miny_ = std::min(miny_, (table_->GetValue(cnt, py_col_)).ToFloat());
    maxy_ = std::max(maxy_, (table_->GetValue(cnt, py_col_)).ToFloat());
    minz_ = std::min(minz_, (table_->GetValue(cnt, pz_col_)).ToFloat());
    maxz_ = std::max(maxz_, (table_->GetValue(cnt, pz_col_)).ToFloat());
    float instant_speed((table_->GetValue(cnt, instant_speed_col_)).ToFloat());
    float instant_turn((table_->GetValue(cnt, instant_turn_col_)).ToFloat()*
                       M_PI/180);
    float previous_speed((table_->GetValue(cnt,
                                           previous_speed_col_)).ToFloat());
    float previous_turn((table_->GetValue(cnt, previous_turn_col_)).ToFloat()*
                       M_PI/180);
    float mean_speed((instant_speed+previous_speed)/2);
    float mean_turn((instant_turn+previous_turn)/2);
    min_speed_ = std::min(min_speed_, mean_speed/mean_turn);
    max_speed_ = std::max(max_speed_, mean_speed/mean_turn);
    int id((table_->GetValue(cnt, id_col_)).ToInt());
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
  max_speed_ = 80;
  std::vector<float> min_max({minx_, miny_, minz_, maxx_, maxy_, maxz_,
    min_speed_, max_speed_});
  std::cout << "average cells per frame:" << ave/(frames.size()-1) <<
    std::endl;
  std::cout << "dims:" << maxx_-minx_ << " " << maxy_-miny_ << " " << maxz_-minz_ <<
    std::endl;
  std::cout << "min:" << minx_ << " " << miny_ << " " << minz_ << std::endl;
  std::cout << "max:" << maxx_ << " " << maxy_ << " " << maxz_ << std::endl;
  std::cout << "speed:" << max_speed_ << " " << min_speed_ << std::endl;
  return min_max;
}

void MotocyteReader::update_points(int current_frame) {
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
    const float x((table_->GetValue(row,px_col_)).ToDouble());
    const float y((table_->GetValue(row,py_col_)).ToDouble());
    const float z((table_->GetValue(row,pz_col_)).ToDouble());
    points->SetPoint(i, x, y, z);
    /*
    unsigned color_index(ids_map_[
                               (table_->GetValue(row,id_col_)).ToInt()]);
                               */

    float instant_speed((table_->GetValue(row, instant_speed_col_)).ToFloat());
    float instant_turn((table_->GetValue(row, instant_turn_col_)).ToFloat()*
                       M_PI/180);
    float previous_speed((table_->GetValue(row,
                                           previous_speed_col_)).ToFloat());
    float previous_turn((table_->GetValue(row, previous_turn_col_)).ToFloat()*
                       M_PI/180);
    float mean_speed((instant_speed+previous_speed)/2);
    float mean_turn((instant_turn+previous_turn)/2);
    unsigned color_index(int(std::min(mean_speed/mean_turn, max_speed_)));
    //std::cout << "index:" << color_index << std::endl;
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

void MotocyteReader::reset() {
  ids_map_.clear();
}

int MotocyteReader::get_offset() {
  return 2;
}
