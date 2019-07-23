/*
 * Copyright 2007 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include <QTimer>
#include <QIcon>
#include <QFileDialog>
#include <QFileInfo>
#include <vtkElevationFilter.h>
#include <vtkNew.h>
#include <vtkQtTableView.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include "vtkSmartPointer.h"
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
#include "vtkAutoInit.h" 
#include "ui_SimpleView.h"
#include "SimpleView.h"
#include <limits>
#include <QStyle>


VTK_MODULE_INIT(vtkRenderingOpenGL2)
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);

SimpleView::SimpleView():
  is_forward_(true),
  is_playing_(false), 
  is_active_timeout_(false),
  is_initialized_(false),
  timer_interval_(10),
  slider_value_(0),
  timer_id_(-1),
  rng_(rd_()),
  uni_(-5, 5),
  points_(vtkSmartPointer<vtkPoints>::New()),
  polydata_(vtkSmartPointer<vtkPolyData>::New()),
  renderer_(vtkSmartPointer<vtkRenderer>::New()),
  reader_(vtkSmartPointer<vtkDelimitedTextReader>::New()),
  glyphFilter_(vtkSmartPointer<vtkVertexGlyphFilter>::New()),
  mapper_(vtkSmartPointer<vtkPolyDataMapper>::New()),
  actor_(vtkSmartPointer<vtkActor>::New()),
  render_window_(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()),
  to_table_(vtkSmartPointer<vtkDataObjectToTable>::New()),
  color_table_(vtkSmartPointer<vtkLookupTable>::New()),
  colors_(vtkSmartPointer<vtkUnsignedCharArray>::New()),
  time_text_(vtkSmartPointer<vtkTextActor>::New()),
  timer_(new QTimer(this)) {
    this->ui_ = new Ui_SimpleView;
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

void SimpleView::read_file(std::string input_file_name) {
  reader_->SetFileName(input_file_name.c_str());
  reader_->DetectNumericColumnsOn();
  reader_->SetFieldDelimiterCharacters(",");
  reader_->Update();
  table_ = reader_->GetOutput();
  initialize();
}

void SimpleView::write_png() {
  vtkSmartPointer<vtkWindowToImageFilter> windowToImage = 
    vtkSmartPointer<vtkWindowToImageFilter>::New();
  windowToImage->SetInput(this->ui_->qvtkWidget->renderWindow());
  vtkSmartPointer<vtkPNGWriter> writer =
    vtkSmartPointer<vtkPNGWriter>::New();
  std::ostringstream png_file;
  png_file << "image" << std::setw(5) << std::setfill('0') << image_cnt_++
    << ".png"; 
  writer->SetFileName(png_file.str().c_str());
  writer->SetInputConnection(windowToImage->GetOutputPort());
  writer->Write();
}

void SimpleView::initialize() {
  init_points();
  polydata_->SetPoints(points_);
  glyphFilter_->SetInputData(polydata_);
  glyphFilter_->Update();
  mapper_->SetInputConnection(glyphFilter_->GetOutputPort());
  actor_->SetMapper(mapper_);
  renderer_->AddActor(actor_);

  time_text_->SetInput ("Hello world");
  time_text_->SetPosition2 (500, 500);
  time_text_->GetTextProperty()->SetFontSize (24);
  time_text_->GetTextProperty()->SetColor (1.0, 1.0, 1.0);
  renderer_->AddActor2D(time_text_);

  this->ui_->qvtkWidget->setRenderWindow(render_window_);
  this->ui_->qvtkWidget->renderWindow()->AddRenderer(renderer_);
  this->ui_->qvtkWidget->interactor()->Initialize();
  to_table_->SetInputConnection(glyphFilter_->GetOutputPort());
  to_table_->SetFieldType(vtkDataObjectToTable::POINT_DATA);
  is_initialized_ = true;
  on_timeout();
};

void SimpleView::init_random_points() {
  const unsigned n(300000);
  points_->SetNumberOfPoints(n);
  for (unsigned i(0); i < n; ++i) {
    points_->SetPoint(i, uni_(rng_), uni_(rng_), uni_(rng_)); 
  }
}

void SimpleView::update_random_points() {
  const unsigned n(300000);
  for (unsigned i(0); i < n; ++i) {
    points_->SetPoint(i, uni_(rng_), uni_(rng_), uni_(rng_)); 
  }
}

void SimpleView::init_points() {
  const int skip_rows(2); //skip first two header rows
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
    frames_.push_back(skip_rows);
  }
  int cnt(skip_rows);
  while(cnt < table_->GetNumberOfRows()) {
    if ((table_->GetValue(cnt, time_column)).ToDouble() != time) {
        frames_.push_back(cnt);
        time = (table_->GetValue(cnt, time_column)).ToDouble();
    }
    minx = std::min(minx, (table_->GetValue(cnt, 0)).ToDouble());
    maxx = std::max(maxx, (table_->GetValue(cnt, 0)).ToDouble());
    miny = std::min(miny, (table_->GetValue(cnt, 1)).ToDouble());
    maxy = std::max(maxy, (table_->GetValue(cnt, 1)).ToDouble());
    minz = std::min(minz, (table_->GetValue(cnt, 2)).ToDouble());
    maxz = std::max(maxz, (table_->GetValue(cnt, 2)).ToDouble());
    int id((table_->GetValue(cnt, id_column_)).ToInt());
    if (std::find(ids_.begin(), ids_.end(), id) == ids_.end()) {
      ids_.push_back(id);
      ids_map_[id] = ids_.size()-1;
    }
    ++cnt;
  }
  frames_.push_back(cnt-1);
  current_frame_ = 0;
  init_colors();
  update_points();

  double ave(0);
  for (unsigned i(0); i != frames_.size(); ++i) {
    std::cout << "i:" << i << " " << frames_[i] << std::endl;
    if (i > 0) {
      ave += frames_[i] - frames_[i-1];
    }
  }
  std::cout << "average cells per frame:" << ave/(frames_.size()-1) <<
    std::endl;
  std::cout << "dims:" << maxx-minx << " " << maxy-miny << " " << maxz-minz <<
    std::endl;
}

void SimpleView::inc_dec_frame() {
  if (is_initialized_) {
    if (is_forward_) {
      current_frame_ = std::min(current_frame_+1, int(frames_.size()-2));
    }
    else {
      current_frame_ = std::max(current_frame_-1, 0);
    }
  }
}

void SimpleView::update_points() {
  //std::cout << "frame:" << current_frame_ << std::endl;
  const int n_surface(300);
  const double radius(5);
  int n(frames_[current_frame_+1]-frames_[current_frame_]);
  points_->SetNumberOfPoints(n+n*n_surface);
  colors_->SetNumberOfValues(n+n*n_surface);
  std::uniform_real_distribution<> uni_z(-1, 1);
  std::uniform_real_distribution<> uni_t(0, 2*M_PI);
  for (unsigned i(0); i < n; ++i) {
    int row(i+frames_[current_frame_]);
    const float x((table_->GetValue(row,0)).ToDouble());
    const float y((table_->GetValue(row,1)).ToDouble());
    const float z((table_->GetValue(row,2)).ToDouble());
    points_->SetPoint(i, x, y, z);
    insert_color((table_->GetValue(row,id_column_)).ToInt(), i);
    for (unsigned j(0); j != n_surface; ++j) {
      float zi(uni_z(rng_));
      float t(uni_t(rng_));
      points_->SetPoint(n+i*n_surface+j, 
                        x+sqrt(1-pow(zi,2))*cos(t)*radius,
                        y+sqrt(1-pow(zi,2))*sin(t)*radius,
                        z+zi*radius);
      insert_color((table_->GetValue(row,id_column_)).ToInt(), n+i*n_surface+j);
    }
  }
  polydata_->GetPointData()->SetScalars(colors_);
}

void SimpleView::insert_color(int id, int index) {
  double dcolor[3];
  color_table_->GetColor(ids_map_[id], dcolor);
  /*
  dcolor[0] = 1;
  dcolor[1] = 0;
  dcolor[2] = 0;
  */
  unsigned char color[3];
  for(unsigned int j = 0; j < 3; j++) {
    color[j] = static_cast<unsigned char>(255.0 * dcolor[j]);
  }
  colors_->InsertTypedTuple(index, color);
}

void SimpleView::init_colors() {
  //color_table_->SetNumberOfColors(ids_.size());
  //color_table_->SetHueRange(0.0,0.667);
  color_table_->SetTableRange(0, ids_.size());
  color_table_->Build();
  colors_->SetNumberOfComponents(3);
  colors_->SetName("Colors");
}


SimpleView::~SimpleView() {}

void SimpleView::timeout_add() {
  if (is_initialized_ && !is_active_timeout_) {
    timer_->start();
    is_active_timeout_ = true;
    std::cout << "added timer_id:" << timer_id_ << std::endl;
  }
}

void SimpleView::timeout_remove() {
  if (is_initialized_ && is_active_timeout_) {
    timer_->stop();
    is_active_timeout_ = false;
    std::cout << "removing timer_id:" << timer_id_ << std::endl;
  }
}

void SimpleView::set_frame(int value) {
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

void SimpleView::set_progress_frame(int current_frame) {
  if(current_frame > this->ui_->progressSlider->maximum()) {
    this->ui_->progressSlider->setMaximum(current_frame);
  }
  if(current_frame != this->ui_->progressSlider->value()) {
    this->ui_->progressSlider->setValue(current_frame);
  }
}

void SimpleView::on_timeout() {
  inc_dec_frame(); 
  update_points(); 
  points_->Modified();
  polydata_->Modified();
  set_progress_frame(current_frame_);
  write_time();
  this->ui_->qvtkWidget->interactor()->Render();
  record_frame();
}

void SimpleView::write_time() {
  double time((current_frame_-1)*frame_interval_);
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

void SimpleView::record_frame() {
  if (is_record_) {
    timeout_remove();
    write_png();
    std::cout << "wrote png:" << image_cnt_ << std::endl; 
    timeout_add();
  }
}

void SimpleView::set_unset_record() {
  is_record_ = !is_record_;
  if (is_record_) {
    std::cout << "starting to save png frames..." << std::endl;
  } else {
    std::cout << "stopped saving png frames." << std::endl;
  }
}

void SimpleView::progress_slider_value_changed(int value) {
  if (is_initialized_ && current_frame_ != value) {
    set_frame(value);
  }
}

void SimpleView::open_file() { 
  QString filename =  QFileDialog::getOpenFileName(
                 this,
                 "Open file",
                 QDir::currentPath(),
                 "CSV files (*.csv);; Binary files (*.dat)");
  if (!filename.isNull()) {
    QFileInfo fi(filename);
    std::string csv("csv");
    if (fi.suffix().toUtf8().constData() == csv) {
      reset();
      std::cout << "extension:" << fi.suffix().toUtf8().constData() <<
        std::endl;
      read_file(filename.toUtf8().constData());
    }
  }
}

void SimpleView::reset() {
  timeout_remove();
  frames_.resize(0);
  ids_.resize(0);
  ids_map_.clear();
  is_forward_ = true;
  is_playing_ = false;
  is_initialized_ = false;
  current_frame_ = 1;
}

void SimpleView::exit() {
  qApp->exit();
}

void SimpleView::pause() {
  if(is_initialized_) {
    play_button_->setDefaultAction(play_action_);
    is_playing_ = false;
    timeout_remove();
  }
}

void SimpleView::play() {
  if(is_initialized_) {
    play_button_->setDefaultAction(pause_action_);
    is_playing_ = true;
    timeout_add();
  }
}

void SimpleView::step() {
  if(is_initialized_) {
    if(is_playing_) {
      pause();
    }
    on_timeout();
  }
}

void SimpleView::play_or_pause() {
  if(is_initialized_) {
    if(is_playing_) {
      pause();
    }
    else {
      play();
    }
  }
}

void SimpleView::keyPressEvent(QKeyEvent *event)
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
