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


#ifndef __SpatiocyteReader_hpp
#define __SpatiocyteReader_hpp

#include <vtkSmartPointer.h>
#include <Util.hpp>
#include <Reader.hpp>

class SpatiocyteReader : public Reader {
public:
  static SpatiocyteReader* New();
  SpatiocyteReader();
  ~SpatiocyteReader();
  virtual void initialize(Viewer* viewer, std::string input_file_name);
  virtual std::vector<float> initialize_points();
  virtual void update_points(int current_frame);
private:
  int id_column_;
  double voxel_radius_;
  std::vector<std::string> species_list_;
  std::vector<unsigned> molecule_sizes_;
};

#endif /* __SpatiocyteReader_hpp */
