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


#ifndef __Reader_hpp
#define __Reader_hpp

#include <vtkDelimitedTextReader.h>

class Viewer;

class Reader : public vtkDelimitedTextReader {
public:
  static Reader* New();
  Reader();
  ~Reader();
  virtual void initialize(Viewer* viewer, std::string input_file_name) {};
  virtual std::vector<float> initialize_points() {
    return std::vector<float>({0, 0, 0, 1, 1, 1});
  };
  virtual void update_points(int current_frame) {};
  virtual void reset() {};
  virtual int get_offset();
protected:
  Viewer* viewer_;
  vtkTable* table_;
};

#endif /* __Reader_hpp */
