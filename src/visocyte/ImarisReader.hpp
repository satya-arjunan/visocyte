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


#ifndef __ImarisReader_hpp
#define __ImarisReader_hpp

#include <map>
#include <random>
#include <vtkDelimitedTextReader.h>
#include <vtkSmartPointer.h>
#include <vtkPoints.h>
#include <vtkUnsignedCharArray.h> 

class Viewer;

class ImarisReader : public vtkDelimitedTextReader {
public:
  static ImarisReader* New();
  ImarisReader();
  ~ImarisReader();
  void initialize(Viewer* viewer, std::string input_file_name);
  void initialize_points(std::vector<int>& frames, std::vector<int>& ids,
                         std::map<int, int>& ids_map);
  void update_points(std::vector<int>& frames, int current_frame, 
                     vtkSmartPointer<vtkPoints> points, 
                     vtkSmartPointer<vtkUnsignedCharArray> colors,
                     std::mt19937_64& rng);
private:
  Viewer* viewer_;
  vtkTable* table_;
  int id_column_;
};

#endif /* __ImarisReader_hpp */
