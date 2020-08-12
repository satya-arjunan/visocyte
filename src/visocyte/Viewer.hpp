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


#ifndef __Viewer_hpp
#define __Viewer_hpp

#include <random>
#include <map>
#include <Reader.hpp>
#include <vtkSmartPointer.h>
#include <QMainWindow>
#include <QKeyEvent>
#include <QAction>
#include <QToolButton>
#include <vtkPoints.h>
#include <vtkPolyDataMapper.h>
#include <vtkTable.h>
#include <vtkDataObjectToTable.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkCubeAxesActor2D.h>
#include <vtkAxesActor.h>
#include <vtkCubeAxesActor.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkSuperquadricSource.h>
#include "vtkGenericOpenGLRenderWindow.h"

class Ui_Viewer;
class vtkQtTableView;

class Viewer : public QMainWindow
{
  Q_OBJECT
public:
  Viewer();
  ~Viewer() override;

public slots:
  virtual void open_file();
  virtual void exit();
  virtual void progress_slider_value_changed(int);
  virtual std::vector<float> initialize_points();
  virtual void init_colors();
  virtual void insert_color(const unsigned color_index,
                            const unsigned agent_index);
  virtual void update_points();
  virtual void init_random_points();
  virtual void update_random_points();
  virtual void inc_dec_frame();
  virtual void timeout_add();
  virtual void timeout_remove();
  virtual void on_timeout();
  virtual void set_frame(int value);
  virtual void set_progress_frame(int current_frame);
  virtual void keyPressEvent(QKeyEvent *event);
  virtual void play();
  virtual void pause();
  virtual void play_or_pause();
  std::vector<int>& get_frames();
  std::vector<int>& get_ids();
  std::vector<double>& get_times();
  vtkPoints* get_points();
  vtkUnsignedCharArray* get_colors();
  std::mt19937_64& get_rng();


protected:
  void write_png();
  void step();
  void read_file(std::string input_file_name);
  void initialize();
  void reset();
  void set_unset_record();
  void write_time();
  void record_frame();

protected slots:

private:
  bool is_forward_;
  bool is_playing_;
  bool is_active_timeout_;
  bool is_initialized_;
  bool is_record_ = false;
  unsigned image_cnt_ = 0;
  double timer_interval_;
  double frame_interval_ = 20.63;
  int slider_value_;
  std::random_device rd_;
  std::mt19937_64 rng_;
  std::uniform_real_distribution<> uni_;
  vtkQtTableView* tableview_;
  Ui_Viewer *ui_;
  int timer_id_;
  int current_frame_;
  std::vector<int> frames_;
  std::vector<int> ids_;
  std::vector<double> times_;
  vtkPoints* points_;
  vtkPolyData* polydata_;
  Reader* reader_;
  vtkUnsignedCharArray* colors_;
  vtkLookupTable* color_table_;
  vtkRenderer* renderer_;
  vtkVertexGlyphFilter* glyph_filter_;
  vtkPolyDataMapper* mapper_;
  vtkActor* actor_;
  vtkGenericOpenGLRenderWindow* render_window_;
  vtkDataObjectToTable* to_table_;
  vtkTextActor* time_text_;
  vtkTextProperty* text_prop_;
  vtkCubeAxesActor2D* axes_;
  vtkAxesActor* axes_actor_;
  vtkOrientationMarkerWidget* axes_widget_;
  vtkSmartPointer<vtkSuperquadricSource> superquadricSource;
  vtkSmartPointer<vtkActor> superquadricActor;
  vtkSmartPointer<vtkCubeAxesActor> cubeAxesActor;
  QTimer* timer_;
  std::string filename_;
  QAction* play_action_;
  QAction* pause_action_;
  QAction* open_action_;
  QToolButton* play_button_;
  QToolButton* open_button_;
};

#endif /* __Viewer_hpp */
