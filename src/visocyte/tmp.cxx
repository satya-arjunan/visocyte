/*
 * Copyright 2007 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */


#include <vtkDataObjectToTable.h>
#include <vtkElevationFilter.h>
#include "vtkGenericOpenGLRenderWindow.h"
#include <vtkNew.h>
#include <vtkQtTableView.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include "vtkSmartPointer.h"
#include <vtkVectorText.h>
#include <vtkActor.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkProperty.h>
#include <vtkCallbackCommand.h>
#include <vtkPolyDataStreamer.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkDoubleArray.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkLookupTable.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include "vtkAutoInit.h" 
#include "ui_SimpleView.h"
#include "SimpleView.h"


VTK_MODULE_INIT(vtkRenderingOpenGL2)
VTK_MODULE_INIT(vtkInteractionStyle);

void vtkTimerCallback::Execute(vtkObject *caller, unsigned long eventId,
                     void * vtkNotUsed(callData)) {
  if (vtkCommand::TimerEvent == eventId) {
    ++this->TimerCount;
  }
  view_->on_timeout(); 
} 

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
    if ((table_->GetValue(skip_rows-1, i)).ToString() == "Parent") {
      id_column_ = i;
    }
  }
  int time(0);
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
}

void SimpleView::inc_dec_frame() {
  if(is_forward_) {
    current_frame_ = std::min(current_frame_+1, int(frames_.size()-2));
  }
  else {
    current_frame_ = std::max(current_frame_-1, 0);
  }
}

// Define interaction style
class KeyPressInteractorStyle : public vtkInteractorStyleTrackballCamera
{
public:
  static KeyPressInteractorStyle* New();
  vtkTypeMacro(KeyPressInteractorStyle, vtkInteractorStyleTrackballCamera);

  virtual void OnKeyPress() override
  {
    // Get the keypress
    vtkRenderWindowInteractor *rwi = this->Interactor;
    std::string key = rwi->GetKeySym();

    // Output the key that was pressed
    std::cout << "Pressed " << key << std::endl;

    // Handle an arrow key
    if(key == "Up")
    {
      std::cout << "The up arrow was pressed." << std::endl;
    }

    // Handle a "normal" key
    if(key == "a")
    {
      std::cout << "The a key was pressed." << std::endl;
    }

    // Forward events
    vtkInteractorStyleTrackballCamera::OnKeyPress();
  }
};

vtkStandardNewMacro(KeyPressInteractorStyle);


// Constructor
SimpleView::SimpleView():
  is_forward_(true),
  is_playing_(false), 
  timer_interval_(100),
  slider_value_(0),
  rng_(rd_()),
  uni_(-5, 5),
  callback_(vtkSmartPointer<vtkTimerCallback>::New()),
  points_(vtkSmartPointer<vtkPoints>::New()),
  polydata_(vtkSmartPointer<vtkPolyData>::New()),
  renderer_(vtkSmartPointer<vtkRenderer>::New()) {

  callback_->view_ = this;
  callback_->points_ = points_;
  callback_->polydata_ = polydata_;

  read_file();
  init_points();
  this->ui_ = new Ui_SimpleView;
  this->ui_->setupUi(this);
  this->ui_->progressSlider->setFocusPolicy(Qt::StrongFocus);
  this->ui_->progressSlider->setTickPosition(QSlider::TicksBothSides);
  this->ui_->progressSlider->setMinimum(1);
  this->ui_->progressSlider->setMaximum(frames_.size()-2);
  this->ui_->progressSlider->setTickInterval(frames_.size()/10);
  this->ui_->progressSlider->setSingleStep(1);

  polydata_->SetPoints(points_);
  vtkSmartPointer<vtkVertexGlyphFilter> glyphFilter =
    vtkSmartPointer<vtkVertexGlyphFilter>::New();
  glyphFilter->SetInputData(polydata_);
  glyphFilter->Update();

  vtkSmartPointer<KeyPressInteractorStyle> style =
    vtkSmartPointer<KeyPressInteractorStyle>::New();
  this->ui_->qvtkWidget->interactor()->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer_);


  vtkSmartPointer<vtkPolyDataMapper> mapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(glyphFilter->GetOutputPort());
  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  renderer_->AddActor(actor);

  vtkNew<vtkGenericOpenGLRenderWindow> renderWindow;
  this->ui_->qvtkWidget->setRenderWindow(renderWindow);
  this->ui_->qvtkWidget->renderWindow()->AddRenderer(renderer_);

  this->ui_->qvtkWidget->interactor()->Initialize();
  //play();

  // Just a bit of Qt interest: Culling off the
  // point data and handing it to a vtkQtTableView
  vtkNew<vtkDataObjectToTable> toTable;
  toTable->SetInputConnection(glyphFilter->GetOutputPort());
  toTable->SetFieldType(vtkDataObjectToTable::POINT_DATA);

  // Set up action signals and slots
  connect(this->ui_->actionOpenFile, SIGNAL(triggered()), this,
          SLOT(slotOpenFile()));
  connect(this->ui_->actionExit, SIGNAL(triggered()), this, SLOT(slotExit()));
  connect(this->ui_->progressSlider, SIGNAL(valueChanged(int)), this,
          SLOT(progress_slider_value_changed(int)));
  //renderWindowInteractor->Start();
};

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

void SimpleView::read_file() {
  std::string input_file_name = "position.csv";
  reader_ = vtkSmartPointer<vtkDelimitedTextReader>::New();
  reader_->SetFileName(input_file_name.c_str());
  reader_->DetectNumericColumnsOn();
  reader_->SetFieldDelimiterCharacters(",");
  reader_->Update();
  table_ = reader_->GetOutput();
}

void SimpleView::insert_color(int id, int index) {
  double dcolor[3];
  color_table_->GetColor(ids_map_[id], dcolor);
  unsigned char color[3];
  for(unsigned int j = 0; j < 3; j++) {
    color[j] = static_cast<unsigned char>(255.0 * dcolor[j]);
  }
  colors_->InsertTypedTuple(index, color);
}

void SimpleView::init_colors() {
  color_table_ = vtkSmartPointer<vtkLookupTable>::New();
  //color_table_->SetNumberOfColors(ids_.size());
  //color_table_->SetHueRange(0.0,0.667);
  color_table_->SetTableRange(0, ids_.size());
  color_table_->Build();
  colors_ = vtkSmartPointer<vtkUnsignedCharArray>::New();
  colors_->SetNumberOfComponents(3);
  colors_->SetName("Colors");
}


SimpleView::~SimpleView()
{
  // The smart pointers should clean up for up

}

void SimpleView::pause() {
  if (is_playing_) {
    timeout_remove();
    is_playing_ = false;
  }
}

void SimpleView::play() {
  if (!is_playing_) {
    timeout_add();
    is_playing_ = true;
  }
}

void SimpleView::timeout_add() {
  timer_id_ = this->ui_->qvtkWidget->interactor()->CreateRepeatingTimer(
                                                      timer_interval_);
  this->ui_->qvtkWidget->interactor()->AddObserver(vtkCommand::TimerEvent,
                                                   callback_);;
}

void SimpleView::timeout_remove() {
  this->ui_->qvtkWidget->interactor()->RemoveObserver(vtkCommand::TimerEvent);
  this->ui_->qvtkWidget->interactor()->DestroyTimer(timer_id_);
}

void SimpleView::set_frame(int value) {
  if(is_playing_) {
    is_playing_ = false;
    timeout_remove();
  }
  if(is_forward_) {
    current_frame_ = std::max(value-1, 0);
  }
  else {
    current_frame_ = std::min(value+1, int(frames_.size())+1);
  }
  on_timeout();
}

void SimpleView::set_progress_frame(int current_frame) {
  std::cout << "set progress:" << current_frame << " " << this->ui_->progressSlider->value() << std::endl;
  if(current_frame > this->ui_->progressSlider->maximum()) {
    this->ui_->progressSlider->setMaximum(current_frame);
  }
  if(current_frame != this->ui_->progressSlider->value()) {
    this->ui_->progressSlider->setValue(current_frame);
  }
  std::cout << "after set progress:" << current_frame << " " << this->ui_->progressSlider->value() << std::endl;
}


void SimpleView::on_timeout() {
  inc_dec_frame(); 
  update_points(); 
  points_->Modified();
  polydata_->Modified();
  set_progress_frame(current_frame_);
  this->ui_->qvtkWidget->interactor()->Render();
}

void SimpleView::progress_slider_value_changed(int value) {
  std::cout << "value changed:" << value << " curr frame:" << current_frame_ << std::endl;
  if (current_frame_ != value) {
    set_frame(value);
  }
  std::cout << "after value changed:" << value << " curr frame:" << current_frame_ << std::endl;
}

// Action to be taken upon file open
void SimpleView::slotOpenFile() { 
}

void SimpleView::slotExit() {
  qApp->exit();
}
