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

//replace frames with times

std::vector<float> SpatiocyteReader::initialize_points() {
  std::vector<int>& frames(viewer_->get_frames());
  std::vector<int>& ids(viewer_->get_ids()); //species id
  std::vector<double>& times(viewer_->get_times());

  const unsigned ncolumns(table_->GetNumberOfColumns());

  std::string radius_str((table_->GetValue(0, ncolumns-1)).ToString());
  voxel_radius_ = ::atof(split(radius_str, "=")[1].c_str());
  for (unsigned i(1); i < ncolumns-1; ++i) { 
    species_list_.push_back((table_->GetValue(0, i)).ToString());
    //push species id
    ids.push_back(i-1);
  }
  const int skip_rows(1); //skip first header row
  const int time_column(0); //look for time column
  for (unsigned i(skip_rows); i < table_->GetNumberOfRows(); ++i) {
    double time((table_->GetValue(i, time_column)).ToDouble());
    times.push_back(time);
    frames.push_back(i);
  }
  return Reader::initialize_points();
}



void SpatiocyteReader::update_points(int current_frame) {
  std::vector<int>& frames(viewer_->get_frames()); 
  vtkSmartPointer<vtkPoints> points(viewer_->get_points()); 
  vtkSmartPointer<vtkUnsignedCharArray> colors(viewer_->get_colors());
  std::mt19937_64& rng(viewer_->get_rng());
  const int n_surface(100); //number of points on molecule surface
  const double radius(voxel_radius_);
  const unsigned row(frames[current_frame]);
  if (molecule_sizes_.size() <= current_frame) {
    unsigned molecule_size(0);
    for (unsigned i(1); i < species_list_.size()+1; ++i) { 
      std::string coord_str((table_->GetValue(row, i)).ToString());
      if (coord_str.size()) {
        std::vector<std::string> coords_str(split(coord_str, " "));
        molecule_size += coords_str.size()/4; //3D coordinates + ID
      }
    }
    molecule_sizes_.push_back(molecule_size);
  }
  const unsigned molecule_size(molecule_sizes_[current_frame]);
  points->SetNumberOfPoints(molecule_size+molecule_size*n_surface);
  colors->SetNumberOfValues(molecule_size+molecule_size*n_surface);
  std::uniform_real_distribution<> uni_z(-1, 1);
  std::uniform_real_distribution<> uni_t(0, 2*M_PI);
  unsigned cnt(0);
  for (unsigned i(1); i < species_list_.size()+1; ++i) { 
    std::string coord_str((table_->GetValue(row, i)).ToString());
    std::vector<std::string> coords_str(split(coord_str, " "));
    if (coords_str.size() >= 3) {
      for (unsigned j(0); j < coords_str.size(); ) {
        const float x(::atof(coords_str[j].c_str()));
        const float y(::atof(coords_str[j+1].c_str()));
        const float z(::atof(coords_str[j+2].c_str()));
        j += 4;
        points->SetPoint(cnt, x, y, z);
        const unsigned species_index(i-1);
        viewer_->insert_color(species_index, cnt);
        for (unsigned k(0); k < n_surface; ++k) {
          float zi(uni_z(rng));
          float t(uni_t(rng));
          points->SetPoint(molecule_size+cnt*n_surface+k, 
                            x+sqrt(1-pow(zi,2))*cos(t)*radius,
                            y+sqrt(1-pow(zi,2))*sin(t)*radius,
                            z+zi*radius);
          viewer_->insert_color(species_index, molecule_size+cnt*n_surface+k);
        }
        ++cnt;
      }
    }
  }
}

