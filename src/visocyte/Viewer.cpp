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

#include <QTimer>
#include <QIcon>
#include <QFileDialog>
#include <QFileInfo>
#include <vtkElevationFilter.h>
#include <vtkNew.h>
#include <vtkQtTableView.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkVectorText.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkPolyDataStreamer.h>
#include <vtkDoubleArray.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkLookupTable.h>
#include <vtkPNGWriter.h>
#include <vtkWindowToImageFilter.h>
#include <vtkTextProperty.h>
#include <vtkCamera.h>
#include "vtkAutoInit.h" 
#include "ui_Viewer.h"
#include <Viewer.hpp>
#include <ImarisReader.hpp>
#include <SpatiocyteReader.hpp>
#include <limits>
#include <QStyle>


VTK_MODULE_INIT(vtkRenderingOpenGL2)
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);

Viewer::Viewer():
  is_forward_(true),
  is_playing_(false), 
  is_active_timeout_(false),
  is_initialized_(false),
  timer_interval_(10),
  slider_value_(0),
  timer_id_(-1),
  rng_(rd_()),
  uni_(-5, 5),
  points_(vtkPoints::New()),
  polydata_(vtkPolyData::New()),
  reader_(Reader::New()),
  renderer_(vtkRenderer::New()),
  glyph_filter_(vtkVertexGlyphFilter::New()),
  mapper_(vtkPolyDataMapper::New()),
  actor_(vtkActor::New()),
  render_window_(vtkGenericOpenGLRenderWindow::New()),
  to_table_(vtkDataObjectToTable::New()),
  color_table_(vtkLookupTable::New()),
  colors_(vtkUnsignedCharArray::New()),
  time_text_(vtkTextActor::New()),
  text_prop_(vtkTextProperty::New()),
  axes_(vtkCubeAxesActor2D::New()),
  axes_actor_(vtkAxesActor::New()),
  axes_widget_(vtkOrientationMarkerWidget::New()),
  timer_(new QTimer(this)) {
    this->ui_ = new Ui_Viewer;
    this->ui_->setupUi(this);
    this->ui_->progressSlider->setFocusPolicy(Qt::StrongFocus);
    this->ui_->progressSlider->setSingleStep(1);
    this->ui_->progressSlider->setMaximum(1);

    open_action_ = new QAction(style()->standardIcon(QStyle::SP_FileIcon),
                               "open", this);
    open_button_ = new QToolButton;
    open_button_->setDefaultAction(open_action_);
    this->ui_->toolBar->addWidget(open_button_);
    connect(open_action_, SIGNAL(triggered()), this, SLOT(open_file()));

    play_action_ = new QAction(style()->standardIcon(QStyle::SP_MediaPlay),
                               "play", this);
    pause_action_ = new QAction(style()->standardIcon(QStyle::SP_MediaPause),
                                "pause", this);
    play_button_ = new QToolButton;
    play_button_->setDefaultAction(play_action_);
    this->ui_->toolBar->addWidget(play_button_);
    connect(play_action_, SIGNAL(triggered()), this, SLOT(play()));
    connect(pause_action_, SIGNAL(triggered()), this, SLOT(pause()));

    connect(this->ui_->actionExit, SIGNAL(triggered()), this, SLOT(exit()));
    connect(timer_, SIGNAL(timeout()), this, SLOT(on_timeout()));
    connect(this->ui_->progressSlider, SIGNAL(valueChanged(int)), this,
            SLOT(progress_slider_value_changed(int)));
    timer_->setInterval(1);
  }

void Viewer::read_file(std::string input_file_name) {
  reader_->initialize(this, input_file_name);
  initialize();
}

void Viewer::write_png() {
  vtkWindowToImageFilter* windowToImage = vtkWindowToImageFilter::New();
  windowToImage->SetInput(this->ui_->qvtkWidget->renderWindow());
  vtkPNGWriter* writer = vtkPNGWriter::New();
  std::ostringstream png_file;
  png_file << "image" << std::setw(5) << std::setfill('0') << image_cnt_++
    << ".png"; 
  writer->SetFileName(png_file.str().c_str());
  writer->SetInputConnection(windowToImage->GetOutputPort());
  writer->Write();
}

void Viewer::initialize() {
  std::vector<float> min_max(initialize_points());
  polydata_->SetPoints(points_);
  glyph_filter_->SetInputData(polydata_);
  glyph_filter_->Update();
  mapper_->SetInputConnection(glyph_filter_->GetOutputPort());
  actor_->SetMapper(mapper_);
  //renderer_->AddActor(actor_);

  superquadricSource = vtkSmartPointer<vtkSuperquadricSource>::New();
  superquadricSource->SetPhiRoundness(3.1);
  superquadricSource->SetThetaRoundness(1.0);
  superquadricSource->Update(); // needed to GetBounds later

  //mapper_->SetInputConnection(superquadricSource->GetOutputPort());

  superquadricActor = vtkSmartPointer<vtkActor>::New();
  //superquadricActor->SetMapper(mapper_);
  
  cubeAxesActor = vtkSmartPointer<vtkCubeAxesActor>::New();

  cubeAxesActor->SetBounds(min_max[0], min_max[3], min_max[1], min_max[4],
                           min_max[2], min_max[5]);
  /*
  std::cout << superquadricSource->GetOutput()->GetBounds()[0] <<  " " <<
    superquadricSource->GetOutput()->GetBounds()[1] <<  " " <<
    superquadricSource->GetOutput()->GetBounds()[2] <<  " " <<
    superquadricSource->GetOutput()->GetBounds()[3] <<  " " <<
    superquadricSource->GetOutput()->GetBounds()[4] <<  " " <<
    superquadricSource->GetOutput()->GetBounds()[5] << std::endl;
    */
  //cubeAxesActor->SetBounds(superquadricSource->GetOutput()->GetBounds());
  cubeAxesActor->SetCamera(renderer_->GetActiveCamera());
  cubeAxesActor->GetTitleTextProperty(0)->SetColor(1.0, 0.0, 0.0);
  cubeAxesActor->GetLabelTextProperty(0)->SetColor(1.0, 0.0, 0.0);

  cubeAxesActor->GetTitleTextProperty(1)->SetColor(0.0, 1.0, 0.0);
  cubeAxesActor->GetLabelTextProperty(1)->SetColor(0.0, 1.0, 0.0);

  cubeAxesActor->GetTitleTextProperty(2)->SetColor(0.0, 0.5, 1.0);
  cubeAxesActor->GetLabelTextProperty(2)->SetColor(0.0, 0.5, 1.0);

  cubeAxesActor->DrawXGridlinesOn();
  cubeAxesActor->DrawYGridlinesOn();
  cubeAxesActor->DrawZGridlinesOn();

  cubeAxesActor->SetGridLineLocation(cubeAxesActor->VTK_GRID_LINES_FURTHEST);
  
  cubeAxesActor->XAxisMinorTickVisibilityOff();
  cubeAxesActor->YAxisMinorTickVisibilityOff();
  cubeAxesActor->ZAxisMinorTickVisibilityOff();
  cubeAxesActor->GetXAxesGridlinesProperty()->SetColor(0.3, 0.3, 0.3);
  cubeAxesActor->GetYAxesGridlinesProperty()->SetColor(0.3, 0.3, 0.3);
  cubeAxesActor->GetZAxesGridlinesProperty()->SetColor(0.3, 0.3, 0.3);

  renderer_->AddActor(cubeAxesActor);
  renderer_->AddActor(actor_);
  renderer_->GetActiveCamera()->Azimuth(30);
  renderer_->GetActiveCamera()->Elevation(30);


  time_text_->SetInput("Hello world");
  time_text_->SetPosition2 (500, 500);
  time_text_->GetTextProperty()->SetFontSize (24);
  time_text_->GetTextProperty()->SetColor (1.0, 1.0, 1.0);
  renderer_->AddActor2D(time_text_); 

  ////vtkSmartPointer<vtkCubeAxesActor> p_cubeAxesActor = 
  ////  vtkSmartPointer<vtkCubeAxesActor>::New();
  ////p_cubeAxesActor->SetBounds(renderer_->ComputeVisiblePropBounds());
  ////p_cubeAxesActor->SetCamera(renderer_->GetActiveCamera());
  ////p_cubeAxesActor->GetTitleTextProperty(0)->SetColor(1.0, 0.0, 0.0);
  ////p_cubeAxesActor->GetLabelTextProperty(0)->SetColor(1.0, 0.0, 0.0);
  ////p_cubeAxesActor->GetTitleTextProperty(1)->SetColor(1.0, 0.0, 0.0);
  ////p_cubeAxesActor->GetLabelTextProperty(1)->SetColor(1.0, 0.0, 0.0);
  ////p_cubeAxesActor->GetTitleTextProperty(2)->SetColor(1.0, 0.0, 0.0);
  ////p_cubeAxesActor->GetLabelTextProperty(2)->SetColor(1.0, 0.0, 0.0);
  ////p_cubeAxesActor->GetXAxesLinesProperty()->SetColor(1.0, 0.0, 0.0);
  ////p_cubeAxesActor->GetYAxesLinesProperty()->SetColor(1.0, 0.0, 0.0);
  ////p_cubeAxesActor->GetZAxesLinesProperty()->SetColor(1.0, 0.0, 0.0);
  ////p_cubeAxesActor->DrawXGridlinesOff();
  ////p_cubeAxesActor->DrawYGridlinesOff();
  ////p_cubeAxesActor->DrawZGridlinesOff();
  ////p_cubeAxesActor->XAxisMinorTickVisibilityOff();
  ////p_cubeAxesActor->YAxisMinorTickVisibilityOff();
  ////p_cubeAxesActor->ZAxisMinorTickVisibilityOff();
  ////renderer_->AddActor(p_cubeAxesActor);
  ////
  //text_prop_->SetColor(1, 1, 1);
  //text_prop_->ShadowOn();
  //text_prop_->SetFontSize(20);

  ////axes->SetInputConnection(normals->GetOutputPort());
  ////axes->SetCamera(ren->GetActiveCamera());
  //axes_->SetBounds(min_max[0], min_max[1], min_max[2], min_max[3], min_max[4],
  //                 min_max[5]);
  //axes_->SetCamera(renderer_->GetActiveCamera());
  //axes_->SetLabelFormat("%6.4g");
  //axes_->SetFlyModeToOuterEdges();
  //axes_->SetAxisTitleTextProperty(text_prop_);
  //axes_->SetAxisLabelTextProperty(text_prop_);
  ////renderer_->AddViewProp(axes_);
  //renderer_->AddActor(axes_);

  //// axes
  //axes_widget_->SetDefaultRenderer(renderer_);  
  //axes_widget_->SetOrientationMarker(axes_actor_);
  //axes_widget_->SetInteractor(this->ui_->qvtkWidget->interactor());
  //axes_widget_->SetViewport( 0.0, 0.0, 0.2, 0.2 );
  //renderer_->AddActor(axes_actor_);
  //axes_widget_->SetEnabled(1);
  //axes_widget_->InteractiveOn();

  //axes_->SetBounds(min_max[0], min_max[1], min_max[2], min_max[3],
  //                         min_max[4], min_max[5]);
  //axes_->SetCamera(renderer_->GetActiveCamera());
  //renderer_->AddActor(axes_);
  //renderer_->AddActor(actor_);

  this->ui_->qvtkWidget->setRenderWindow(render_window_);
  this->ui_->qvtkWidget->renderWindow()->AddRenderer(renderer_);
  this->ui_->qvtkWidget->interactor()->Initialize();

  //to_table_->SetInputConnection(glyph_filter_->GetOutputPort());
  //to_table_->SetFieldType(vtkDataObjectToTable::POINT_DATA);
  is_initialized_ = true;
  on_timeout();
  renderer_->ResetCamera();
  this->ui_->qvtkWidget->interactor()->Render();
};

void Viewer::init_random_points() {
  const unsigned n(300000);
  points_->SetNumberOfPoints(n);
  for (unsigned i(0); i < n; ++i) {
    points_->SetPoint(i, uni_(rng_), uni_(rng_), uni_(rng_)); 
  }
}

void Viewer::update_random_points() {
  const unsigned n(300000);
  for (unsigned i(0); i < n; ++i) {
    points_->SetPoint(i, uni_(rng_), uni_(rng_), uni_(rng_)); 
  }
}

std::vector<float> Viewer::initialize_points() {
  std::vector<float> min_max(reader_->initialize_points());
  current_frame_ = -1;
  init_colors();
  return min_max;
}

void Viewer::inc_dec_frame() {
  if (is_initialized_) {
    if (is_forward_) {
      current_frame_ = std::min(current_frame_+1, int(frames_.size()-
                                                      reader_->get_offset()));
    }
    else {
      current_frame_ = std::max(current_frame_-1, 0);
    }
  }
}

void Viewer::update_points() {
  reader_->update_points(current_frame_);
  polydata_->GetPointData()->SetScalars(colors_);
}

void Viewer::insert_color(const unsigned color_index,
                          const unsigned agent_index) {
  double dcolor[3];
  color_table_->GetColor(color_index, dcolor);
  /*
  dcolor[0] = 1;
  dcolor[1] = 0;
  dcolor[2] = 0;
  */
  unsigned char color[3];
  for(unsigned int j = 0; j < 3; j++) {
    color[j] = static_cast<unsigned char>(255.0 * dcolor[j]);
  }
  colors_->InsertTypedTuple(agent_index, color);
}

void Viewer::init_colors() {
  //color_table_->SetNumberOfColors(ids_.size());
  //color_table_->SetHueRange(0.0,0.667);
  color_table_->SetTableRange(0, ids_.size());
  color_table_->Build();
  colors_->SetNumberOfComponents(3);
  colors_->SetName("Colors");
}


Viewer::~Viewer() {}

void Viewer::timeout_add() {
  if (is_initialized_ && !is_active_timeout_) {
    timer_->start();
    is_active_timeout_ = true;
    std::cout << "added timer_id:" << timer_id_ << std::endl;
  }
}

void Viewer::timeout_remove() {
  if (is_initialized_ && is_active_timeout_) {
    timer_->stop();
    is_active_timeout_ = false;
    std::cout << "removing timer_id:" << timer_id_ << std::endl;
  }
}

void Viewer::set_frame(int value) {
  if (is_initialized_) {
    if(is_playing_) {
      pause();
    }
    if(is_forward_) {
      current_frame_ = std::max(value-1, 0);
    }
    else {
      current_frame_ = std::min(value+1, int(frames_.size())+1);
    }
    on_timeout();
  }
}

void Viewer::set_progress_frame(int current_frame) {
  if(current_frame > this->ui_->progressSlider->maximum()) {
    this->ui_->progressSlider->setMaximum(current_frame);
  }
  if(current_frame != this->ui_->progressSlider->value()) {
    this->ui_->progressSlider->setValue(current_frame);
  }
}

void Viewer::on_timeout() {
  inc_dec_frame(); 
  update_points(); 
  points_->Modified();
  polydata_->Modified();
  set_progress_frame(current_frame_);
  write_time();
  this->ui_->qvtkWidget->interactor()->Render();
  record_frame();
}

void Viewer::write_time() {
  double time((current_frame_)*frame_interval_);
  if (times_.size()) {
    time = times_[current_frame_];
  }
  std::stringstream ss;
  if(fabs(time) < 1e-3)
    {
      ss << time*1e+6 << " us";
    }
  else if(fabs(time) < 1)
    {
      ss << time*1e+3 << " ms";
    }
  else if(fabs(time) < 60)
    {
      ss << time << " s";
    }
  else if(fabs(time) < 3600)
    {
      ss << int(time/60) << "m " << int(time)%60  << "s";
    }
  else if(fabs(time) < 86400)
    {
      ss << int(time/3600) << "h " << int(time)%3600/60 << "m";
    }
  else
    {
      ss << int(time/86400) << "d " << int(time)%86400/3600 << "h " 
        << int(time)%86400%3600/60 << "m";
    }

  time_text_->SetInput(ss.str().c_str());
}

void Viewer::record_frame() {
  if (is_record_) {
    timeout_remove();
    write_png();
    std::cout << "wrote png:" << image_cnt_ << std::endl; 
    timeout_add();
  }
}

void Viewer::set_unset_record() {
  is_record_ = !is_record_;
  if (is_record_) {
    std::cout << "starting to save png frames..." << std::endl;
  } else {
    std::cout << "stopped saving png frames." << std::endl;
  }
}

void Viewer::progress_slider_value_changed(int value) {
  if (is_initialized_ && current_frame_ != value) {
    set_frame(value);
  }
}

void Viewer::open_file() { 
  QString filename =  QFileDialog::getOpenFileName(
                 this,
                 "Open file",
                 QDir::currentPath(),
                 "CSV files (*.csv);; Spatiocyte files (*.spa)");
  if (!filename.isNull()) {
    QFileInfo fi(filename);
    std::string csv("csv");
    if (fi.suffix().toUtf8().constData() == csv) {
      std::stringstream filename_ss(fi.fileName().toUtf8().constData());
      std::vector<std::string> split_strings;
      std::string str; 
      while (std::getline(filename_ss, str, '_')) {
        split_strings.push_back(str);
      }
      frame_interval_ = std::stof(split_strings[split_strings.size()-2]);
      std::cout << "interval:" << frame_interval_ << std::endl;
      std::cout << "extension:" << fi.suffix().toUtf8().constData() <<
        std::endl;
      reset();
      reader_ = ImarisReader::New();
      read_file(filename.toUtf8().constData());
    }
    else if (fi.suffix().toUtf8().constData() == std::string("spa")) {
      reset();
      reader_ = SpatiocyteReader::New();
      read_file(filename.toUtf8().constData());
    }
  }
}

void Viewer::reset() {
  timeout_remove();
  frames_.resize(0);
  ids_.resize(0);
  reader_->reset();
  is_forward_ = true;
  is_playing_ = false;
  is_initialized_ = false;
  current_frame_ = 1;
}

void Viewer::exit() {
  qApp->exit();
}

void Viewer::pause() {
  if(is_initialized_) {
    play_button_->setDefaultAction(play_action_);
    is_playing_ = false;
    timeout_remove();
  }
}

void Viewer::play() {
  if(is_initialized_) {
    play_button_->setDefaultAction(pause_action_);
    is_playing_ = true;
    timeout_add();
  }
}

void Viewer::step() {
  if(is_initialized_) {
    if(is_playing_) {
      pause();
    }
    on_timeout();
  }
}

void Viewer::play_or_pause() {
  if(is_initialized_) {
    if(is_playing_) {
      pause();
    }
    else {
      play();
    }
  }
}

void Viewer::keyPressEvent(QKeyEvent *event)
{
  switch (event->key()) {
  case Qt::Key_Pause:
  case Qt::Key_P:
    pause();
    break;
  case Qt::Key_Left:
    is_forward_ = false;
    play();
    break;
  case Qt::Key_Right:
    is_forward_ = true;
    play();
    break;
  case Qt::Key_Enter:
    is_forward_ = true;
    step();
    break;
  case Qt::Key_Space:
    play_or_pause();
    break;
  case Qt::Key_Down:
    is_forward_ = false;
    step();
    break;
  case Qt::Key_Up:
    write_png();
    is_forward_ = true;
    step();
    break;
  case Qt::Key_S:
    set_unset_record();
    break;
  default:
    QWidget::keyPressEvent(event);
  }
}

std::vector<int>& Viewer::get_frames() {
  return frames_;
}

std::vector<int>& Viewer::get_ids() {
  return ids_;
}

std::vector<double>& Viewer::get_times() {
  return times_;
}

vtkPoints* Viewer::get_points() {
  return points_;
}

vtkUnsignedCharArray* Viewer::get_colors() {
  return colors_;
}

std::mt19937_64& Viewer::get_rng() {
  return rng_;
}

